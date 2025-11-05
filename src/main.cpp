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

// Variáveis para controle de medição
bool isMeasuring = false;
bool lastTriggerState = true;
unsigned long measurementStartTime = 0;
unsigned long lastSerialUpdate = 0;
unsigned long measurementCount = 0;

// Variáveis para armazenar última medição
float lastVoltage_mV = 0;
float lastCurrent_mA = 0;
float lastPower_mW = 0;

void setup() {
  // Inicializa Serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("========================================");
  Serial.println("Medidor de Energia INA228 - ESP32");
  Serial.println("========================================");
  Serial.println();
  
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
  
  Serial.println("INA228 configurado para máxima velocidade");
  Serial.println();
  Serial.println("Aguardando trigger no GPIO 32 (terra/0V) para iniciar medição...");
  Serial.print("Estado inicial do GPIO 32: ");
  Serial.println(lastTriggerState ? "HIGH (desconectado)" : "LOW (em terra)");
  Serial.println();
  
  // Aguarda conversões iniciais
  delay(500);
}

void loop() {
  // Lê o estado atual do trigger
  bool currentTriggerState = digitalRead(TRIGGER_PIN);
  bool triggerActive = !currentTriggerState; // LOW = ativo (aterrado)
  
  // Detecta início de medição (borda de descida: HIGH -> LOW)
  if (lastTriggerState && !currentTriggerState) {
    Serial.println(">>> MEDIÇÃO INICIADA! GPIO 32 aterrado.");
    Serial.println(">>> Iniciando amostragem contínua...");
    Serial.println();
    
    isMeasuring = true;
    measurementStartTime = millis();
    lastSerialUpdate = millis();
    measurementCount = 0;
    
    // Acende LED para indicar medição
    digitalWrite(LED_PIN, HIGH);
  }
  
  // Detecta fim de medição (borda de subida: LOW -> HIGH)
  if (!lastTriggerState && currentTriggerState) {
    isMeasuring = false;
    digitalWrite(LED_PIN, LOW);
    
    // Calcula estatísticas
    unsigned long measurementDuration = millis() - measurementStartTime;
    float measurementsPerSecond = 0;
    if (measurementDuration > 0) {
      measurementsPerSecond = (measurementCount * 1000.0) / measurementDuration;
    }
    
    Serial.println();
    Serial.println("========================================");
    Serial.println(">>> MEDIÇÃO FINALIZADA!");
    Serial.println("========================================");
    Serial.print("Total de medições: ");
    Serial.println(measurementCount);
    Serial.print("Tempo medido: ");
    Serial.print(measurementDuration / 1000.0, 3);
    Serial.println(" segundos");
    Serial.print("Taxa de amostragem: ");
    Serial.print(measurementsPerSecond, 2);
    Serial.println(" medições/segundo");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Aguardando próximo trigger...");
    Serial.println();
  }
  
  // Atualiza estado anterior
  lastTriggerState = currentTriggerState;
  
  // Se está medindo, faz leituras contínuas
  if (isMeasuring) {
    // Faz leitura (máxima velocidade possível)
    float busVoltage = ina.getBusVoltage();      // Tensão da fonte (V)
    float shuntVoltage = ina.getShuntVoltage();   // Queda de tensão no shunt (V)
    float loadVoltage = busVoltage - shuntVoltage; // Tensão na carga (V)
    float voltage_mV = loadVoltage * 1000.0;      // Converte para mV
    float current_mA = ina.getMilliAmpere();      // Corrente em mA
    float power_mW = voltage_mV * current_mA / 1000.0; // Potência em mW
    
    // Armazena última medição
    lastVoltage_mV = voltage_mV;
    lastCurrent_mA = current_mA;
    lastPower_mW = power_mW;
    
    // Incrementa contador
    measurementCount++;
    
    // Atualiza Serial a cada 0.5s
    unsigned long currentTime = millis();
    if (currentTime - lastSerialUpdate >= SERIAL_UPDATE_INTERVAL) {
      Serial.print("Medição #");
      Serial.print(measurementCount);
      Serial.print(" | Tensão: ");
      Serial.print(voltage_mV, 3);
      Serial.print(" mV | Corrente: ");
      Serial.print(current_mA, 3);
      Serial.print(" mA | Potência: ");
      Serial.print(power_mW, 3);
      Serial.print(" mW");
      
      // Mostra taxa de amostragem atual
      unsigned long elapsed = currentTime - measurementStartTime;
      if (elapsed > 0) {
        float currentRate = (measurementCount * 1000.0) / elapsed;
        Serial.print(" | Taxa: ");
        Serial.print(currentRate, 1);
        Serial.print(" med/s");
      }
      
      Serial.println();
      lastSerialUpdate = currentTime;
    }
  } else {
    // Quando não está medindo, pequeno delay para não sobrecarregar CPU
    delay(10);
  }
}
