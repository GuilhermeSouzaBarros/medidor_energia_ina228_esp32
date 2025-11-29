/*
 * Medidor de Energia INA228 com ESP32
 * 
 * Hardware:
 * - ESP32 (DOIT DEVKIT V1)
 * - Módulo INA228 (I2C)
 * - LED onboard no GPIO 2
 * - Pino D32 (GPIO 32) como trigger de medição
 * 
 * Ligações I2C:
 * - 3.3V (ESP32) -> VCC (INA228)
 * - GND  (ESP32) -> GND (INA228)
 * - G21  (ESP32) -> SDA (INA228)
 * - G22  (ESP32) -> SCL (INA228)
 * 
 * Funcionamento:
 * - Quando D32 está em terra (0V), inicia medições contínuas
 * - Mede tensão (mV), corrente (mA) e calcula potência (mW)
 * - Atualiza valores no terminal a cada 0.5s
 * - Faz o máximo de medições possível durante a medição
 * - Ao finalizar (D32 desconectado), mostra estatísticas
 */
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include <Arduino.h>
#include <Wire.h>
#include "INA228.h"

// Configurações de Pinos
#define LED_PIN 2
#define TRIGGER_PIN 32
#define I2C_SDA 21
#define I2C_SCL 22

// Endereço I2C do INA228
#define INA228_ADDRESS 0x40

// Intervalo para atualizar Serial (500ms)
#define SERIAL_UPDATE_INTERVAL 500

// Instância do INA228
INA228 ina(INA228_ADDRESS);

bool lastTriggerState = true;

typedef struct outputData {
  unsigned long time;
  float voltage_mV;
  float current_mA;
  float power_mW;
} outputData;

#define TX_BUFFER_SIZE 1024 * 4
#define OUTPUT_DATA_SIZE TX_BUFFER_SIZE / 16

typedef struct outputBuffer {
  outputData data[OUTPUT_DATA_SIZE];
} outputBuffer;

outputBuffer output[2];
bool output_can_write[2] = {1, 1};
bool output_can_flush[2] = {0, 0};
int   output_to_flush[2] = {0, 0};
int output_next_flush = 0;

bool task_writer_ready = 0;
void taskWriter(void* parameter) {
  Serial.println("\tTarefa taskWriter iniciada");
  Serial.flush();
  task_writer_ready = 1;
  while(true) {
    if (output_can_flush[output_next_flush]) {
      Serial.write((char*)(
        &output[output_next_flush]),
        sizeof(outputData) * output_to_flush[output_next_flush]
      );
      Serial.flush();
      // For Debugging with pio device monitor
      //Serial.printf("\n\n\nFlushed buffer %d\n\n\n", output_next_flush);
      //Serial.flush();
      output_can_flush[output_next_flush] = 0;
      output_to_flush[output_next_flush] = 0;
      output_can_write[output_next_flush] = 1;
      output_next_flush = !output_next_flush;
    } //else {
      //Serial.print("\n\n\n\tWaiting to flush...\n\n\n");
    //}
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // Inicializa Serial
  Serial.setTxBufferSize(TX_BUFFER_SIZE);
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================================");
  Serial.println("Medidor de Energia INA228 - ESP32");
  Serial.println("========================================================\n");
  
  // Configura LED como saída
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Configura D32 como entrada com pull-up interno
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  lastTriggerState = digitalRead(TRIGGER_PIN);
  
  // Inicia barramento I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // 400kHz
  
  Serial.print("I2C configurado (SDA: GPIO ");
  Serial.print(I2C_SDA);
  Serial.print(", SCL: GPIO ");
  Serial.print(I2C_SCL);
  Serial.println(")");
  
  // Inicializa o INA228
  Serial.print("Inicializando INA228... ");
  if (!ina.begin()) {
    Serial.println("ERRO!");
    Serial.println("INA228 não encontrado no barramento I2C.");
    Serial.println("Verifique as ligações e o endereço I2C.");
    while (1) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }
  Serial.println("OK!");
  
  // Configura calibração para 1A máximo com shunt de 0.015 Ohms
  ina.setMaxCurrentShunt(1.0, 0.015);
  
  // Configura modo de conversão contínua de BUS e SHUNT
  ina.setMode(INA228_MODE_CONT_BUS_SHUNT);
  
  // Configura tempos de conversão para velocidade
  ina.setBusVoltageConversionTime(INA228_50_us);
  ina.setShuntVoltageConversionTime(INA228_50_us);
  
  // Configura sem média (1 amostra) para máxima velocidade
  ina.setAverage(INA228_1_SAMPLE);
  
  // Configura ADCRange para resolução máxima
  ina.setADCRange(false);
  
  Serial.println("INA228 configurado para máxima velocidade\n");
  Serial.println("Aguardando trigger no GPIO 32 (terra/0V) para iniciar medição...");
  Serial.print("Estado inicial do GPIO 32: ");
  Serial.println(lastTriggerState ? "HIGH (desconectado)" : "LOW (em terra)\n\n");
  
  Serial.println("\tIniciando taskWriter...");
  xTaskCreatePinnedToCore(
    taskWriter,
    "Task Writer",
    TX_BUFFER_SIZE*4,
    NULL,
    1,
    NULL,
    0
  );
  while (!task_writer_ready) delay(1);

  Serial.println("Setup terminou com sucesso.\n\n");
  Serial.println("finished_setup");
  Serial.flush();
  
  // Aguarda conversões iniciais
  delay(500);
}

// Variáveis para controle de medição
bool isMeasuring = false;
bool currentTriggerState, triggerActive;
unsigned int current_state = 0;
float busVoltage, shuntVoltage, loadVoltage;

#define POS_TIMESTAMP 0
#define POS_MEASUREMENTS sizeof(unsigned long)

int output_buffer_index = 0;
int output_buffer_data_index = 0;

void ouput_buffer_increment() {
  output_buffer_data_index++;
  output_to_flush[output_buffer_index]++;
  if (output_buffer_data_index < OUTPUT_DATA_SIZE) return;
  output_buffer_data_index = 0;
  output_can_write[output_buffer_index] = 0;
  output_can_flush[output_buffer_index] = 1;
  output_buffer_index = !output_buffer_index;
  while (!output_can_write[output_buffer_index]) {
    //Serial.print("\n\n\n\tWaiting to write...\n\n\n");
    delayMicroseconds(5);
  }
}

void loop() {
  if (!isMeasuring && current_state) {
    delay(1000);
    return;
  }

  // Lê o estado atual do trigger
  currentTriggerState = digitalRead(TRIGGER_PIN);
  triggerActive = !currentTriggerState; // LOW = ativo (aterrado)
  
  // Detecta troca de estado de medição (borda de descida: HIGH -> LOW)
  if (lastTriggerState && !currentTriggerState) {
    memcpy(
      &(output[output_buffer_index].data[output_buffer_data_index]),
      "state swap\0\0\0\0\0", 16
    );
    ouput_buffer_increment();

    current_state++;
    isMeasuring = current_state < 4;
    if (isMeasuring) {
      digitalWrite(LED_PIN, LOW);
    } else {
      output_can_flush[output_buffer_index] = 1;
      digitalWrite(LED_PIN, HIGH);
    }
  }
  
  // Atualiza estado anterior
  lastTriggerState = currentTriggerState;
  
  // Se está medindo, faz leituras contínuas
  if (isMeasuring) {
    // Faz leitura (máxima velocidade possível)
    outputData* output_current = &(output[output_buffer_index].data[output_buffer_data_index]);
    output_current->time = micros();
    busVoltage = ina.getBusVoltage();      // Tensão da fonte (V)
    shuntVoltage = ina.getShuntVoltage();   // Queda de tensão no shunt (V)
    output_current->current_mA = ina.getMilliAmpere();      // Corrente em mA

    loadVoltage = busVoltage - shuntVoltage; // Tensão na carga (V)
    output_current->voltage_mV = loadVoltage * 1000.0;       // Converte para mV
    output_current->power_mW = loadVoltage * output_current->current_mA;     // Potência em mW
    
    ouput_buffer_increment();
    delayMicroseconds(800);

  } else {
    // Quando não está medindo, pequeno delay para não sobrecarregar CPU
    delay(100);
  }
}
