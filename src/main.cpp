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

void setup() {
  // Inicializa Serial
  Serial.setTxBufferSize(1024);
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
  Serial.println(lastTriggerState ? "HIGH (desconectado)" : "LOW (em terra)\n");
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

#pragma pack(push, 1)
typedef struct outputData {
  unsigned long time;
  float voltage_mV;
  float current_mA;
  float power_mW;
} outputData;
#pragma pack(pop)

outputData output;

#define POS_TIMESTAMP 0
#define POS_MEASUREMENTS sizeof(unsigned long)

void loop() {
  // Lê o estado atual do trigger
  if (current_state >= 2) {
    delay(1000);
    return;
  }

  currentTriggerState = digitalRead(TRIGGER_PIN);
  triggerActive = !currentTriggerState; // LOW = ativo (aterrado)
  
  // Detecta troca de estado de medição (borda de descida: HIGH -> LOW)
  if (lastTriggerState && !currentTriggerState) {
    Serial.write("state swap", 20);
    current_state++;
    if (current_state == 1) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      Serial.flush();
      digitalWrite(LED_PIN, LOW);
    }
  }
  
  
  // Atualiza estado anterior
  lastTriggerState = currentTriggerState;
  if (current_state == 1) {
    // Se está medindo, faz leituras contínuas (máxima velocidade possível)
    output.time = micros();
    busVoltage = ina.getBusVoltage();      // Tensão da fonte (V)
    shuntVoltage = ina.getShuntVoltage();   // Queda de tensão no shunt (V)
    output.current_mA = ina.getMilliAmpere();      // Corrente em mA

    loadVoltage = busVoltage - shuntVoltage; // Tensão na carga (V)
    output.voltage_mV = loadVoltage * 1000.0;       // Converte para mV
    output.power_mW = loadVoltage * output.current_mA;     // Potência em mW
    
    Serial.write((char*)&output, sizeof(outputData));
    Serial.flush();
  
  
  } else if (current_state == 0) {
    // Quando está esperando a medição,
    // pequeno delay para não sobrecarregar CPU
    delay(100);
  }
}
