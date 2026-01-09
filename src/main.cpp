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

// Configurações de Inferências
#define NUMBER_OF_MODELS 5
#define INFERENCES_PER_MODEL 100
#define STATES_PER_INFERENCE 3

// Configurações de Pinos
#define LED_PIN 2
#define TRIGGER_PIN_RASPBERRY 32
#define TRIGGER_PIN_POP_OS 33
#define I2C_SDA 21
#define I2C_SCL 22

// Endereço I2C do INA228
#define INA228_ADDRESS 0x40

// Intervalo para atualizar Serial (500ms)
#define SERIAL_UPDATE_INTERVAL 500

// Instância do INA228
INA228 ina(INA228_ADDRESS);

// Diretiva de compilacao para que o tamanho das struct seja a soma das suas partes
#pragma pack(push, 1)
typedef struct outputData {
  unsigned int time;
  u_short voltage_mV;
  u_short current_mA;
} outputData;
#pragma pack(pop)

#define TX_BUFFER_SIZE 1024 * 4
#define OUTPUT_DATA_SIZE TX_BUFFER_SIZE / sizeof(outputData)
// sizeof returns 8 during runtime with unsigned long as well

typedef struct outputBuffer {
  outputData data[OUTPUT_DATA_SIZE];
} outputBuffer;

outputBuffer output[2];
bool output_can_write[2] = {1, 1}; // writable buffers
bool output_can_flush[2] = {0, 0}; // flushable buffers
int   output_to_flush[2] = {0, 0}; // size to flush per buffer
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
      output_can_flush[output_next_flush] = 0;
      output_to_flush[output_next_flush] = 0;
      output_can_write[output_next_flush] = 1;
      output_next_flush = !output_next_flush;
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

bool trigger_state_pop_os_last = true, trigger_state_raspberry_last = true;
bool trigger_state_pop_os_now, trigger_state_raspberry_now;

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
  pinMode(TRIGGER_PIN_POP_OS, INPUT_PULLDOWN);
  trigger_state_pop_os_last = digitalRead(TRIGGER_PIN_POP_OS);
  
  pinMode(TRIGGER_PIN_RASPBERRY, INPUT_PULLDOWN);
  trigger_state_raspberry_last = digitalRead(TRIGGER_PIN_RASPBERRY);
  
  // Inicia barramento I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // 400kHz
  
  Serial.printf("I2C configurado (SDA: GPIO %d, SCL: GPIO %d)\n", I2C_SDA, I2C_SCL);
  
  // Inicializa o INA228
  Serial.print("Inicializando INA228... ");
  Serial.flush();
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
  
  // Configura calibração para 10A máximo com shunt de 0.015 Ohms
  ina.setMaxCurrentShunt(10.0, 0.015);
  
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
  Serial.println("Aguardando trigger no GPIO 33 (Raspberry pi) para iniciar medição...");
  
  Serial.println("\tIniciando taskWriter...");
  xTaskCreatePinnedToCore(taskWriter, "Writer", TX_BUFFER_SIZE*4, NULL, 1, NULL, 0);
  while (!task_writer_ready) delay(1);

  Serial.printf("Expecting %d models, %d inferences and %d states\n",
    NUMBER_OF_MODELS, INFERENCES_PER_MODEL, STATES_PER_INFERENCE);
  Serial.println("Setup terminou com sucesso.\n\n");
  Serial.println("setup_finished");
  Serial.flush();
  
  // Aguarda conversões iniciais
  delay(500);
}

// Variáveis para controle de medição
unsigned int current_state = 0;
float busVoltage, shuntVoltage, loadVoltage;

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
    delayMicroseconds(50);
  }
}
int model_current = 0;
int inference_count = 0;

void loop() {
  if (model_current >= NUMBER_OF_MODELS) {
    delay(1000);
    return;
  }
  // Atualiza os estados de trigger
  trigger_state_pop_os_now    = digitalRead(TRIGGER_PIN_POP_OS);
  trigger_state_raspberry_now = digitalRead(TRIGGER_PIN_RASPBERRY);

  int falling_edge_pop_os    = trigger_state_pop_os_last    && !trigger_state_pop_os_now;
  int falling_edge_raspberry = trigger_state_raspberry_last && !trigger_state_raspberry_now;

  trigger_state_pop_os_last    = trigger_state_pop_os_now;
  trigger_state_raspberry_last = trigger_state_raspberry_now;
  
  // Detecta troca de estado de medição (borda de descida: HIGH -> LOW)
  if (falling_edge_raspberry) {
    memcpy(
      &(output[output_buffer_index].data[output_buffer_data_index]),
      "staswap", 8
    );
    ouput_buffer_increment();

    current_state = (current_state + 1) % (STATES_PER_INFERENCE + 1);
    
    if (current_state) {
      digitalWrite(LED_PIN, LOW);
    } else {
      inference_count++;
      if (inference_count >= INFERENCES_PER_MODEL) {
        inference_count = 0;
        model_current++;
        if (model_current >= NUMBER_OF_MODELS) {
          output_can_flush[output_buffer_index] = 1;
        }
      }
      digitalWrite(LED_PIN, HIGH);
    }
  }
  
  // Se está medindo, faz leituras contínuas
  if (current_state) {
    // Faz leitura (máxima velocidade possível)
    outputData* output_current = &(output[output_buffer_index].data[output_buffer_data_index]);

    output_current->time = micros();

    output_current->current_mA = ina.getCurrent() * 1000; // Corrente em mA

    busVoltage = ina.getBusVoltage();                  // Tensão da fonte (V)
    shuntVoltage = ina.getShuntVoltage();              // Queda de tensão no shunt (V)
    loadVoltage = busVoltage - shuntVoltage;           // Tensão na carga (V)
    output_current->voltage_mV = loadVoltage * 1000;   // Tensao para mV

    ouput_buffer_increment();

  } else {
    // Quando não está medindo, pequeno delay para não sobrecarregar CPU
    delay(25);
  }
}
