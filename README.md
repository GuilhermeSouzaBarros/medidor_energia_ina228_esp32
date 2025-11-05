# Medidor de Energia INA228 com ESP32

Projeto para medição de tensão, corrente e potência utilizando ESP32 e sensor INA228. As medições são realizadas de forma contínua enquanto o pino D32 estiver aterrado, com atualizações no terminal a cada 0.5 segundos.

## 📋 Características

- Medição de tensão (mV), corrente (mA) e potência (mW)
- Amostragem em alta velocidade (máximo de medições possível)
- Atualização de valores no terminal a cada 0.5 segundos
- Estatísticas ao finalizar: total de medições, tempo e taxa de amostragem
- Controle por trigger: inicia quando D32 está em terra (GND)

## 🛠️ Hardware Necessário

- **ESP32 DOIT DEVKIT V1**
- **Módulo INA228** (sensor de corrente e potência)
- **Resistor shunt**: 0.015Ω
- **Cabo USB** para programação e alimentação
- **Protoboard e jumpers** para conexões

## 📦 Instalação e Configuração

### 1. Instalação do `uv`

O `uv` é uma ferramenta rápida para gerenciamento de dependências Python e criação de ambientes virtuais isolados.

**Instalação via pip:**
```bash
pip install uv
```

**Ou via curl (recomendado):**
```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

### 2. Criação do Ambiente Virtual

Navegue até o diretório do projeto e crie o ambiente virtual:

```bash
cd "Medidor Energia"
uv venv
```

**Ativar o ambiente virtual:**

- **Linux/Mac:**
  ```bash
  source .venv/bin/activate
  ```

- **Windows:**
  ```bash
  .venv\Scripts\activate
  ```

### 3. Instalação do PlatformIO

Com o ambiente virtual ativado, instale o PlatformIO:

```bash
uv pip install platformio
```

**Alternativa:** Se preferir instalar o PlatformIO como extensão do VS Code:

1. Abra o Visual Studio Code
2. Pressione `Ctrl + Shift + X` (ou `Cmd + Shift + X` no Mac) para abrir a aba de extensões
3. Pesquise por "PlatformIO IDE"
4. Clique em "Instalar"

### 4. Verificação da Instalação

Verifique se o PlatformIO está instalado corretamente:

```bash
pio --version
```

## 🚀 Compilação e Upload

### Compilar o projeto:

```bash
pio run
```

### Fazer upload para o ESP32:

```bash
pio run --target upload
```

### Compilar e fazer upload em um único comando:

```bash
pio run --target upload
```

**Nota:** Certifique-se de que o ESP32 está conectado via USB e a porta serial está disponível.

## 📺 Monitor Serial

Para visualizar as medições e mensagens do ESP32:

### Método 1: Via Paleta de Comandos (Recomendado)

1. Pressione `Ctrl + Shift + P` (ou `Cmd + Shift + P` no Mac)
2. Digite "Serial Monitor" ou "PlatformIO: Serial Monitor"
3. Selecione a opção correspondente

### Método 2: Via Terminal

```bash
pio device monitor
```

### Método 3: Via Barra de Status

Clique no ícone de "plug" na barra de status do PlatformIO e selecione "Serial Monitor"

**Configurações do Monitor Serial:**
- Velocidade: 115200 baud
- Formato: NL & CR (New Line e Carriage Return)

## 🔌 Conexões

### ESP32 → INA228

| ESP32 | INA228 | Descrição |
|-------|--------|-----------|
| 3.3V  | VCC    | Alimentação |
| GND   | GND    | Terra comum |
| GPIO21| SDA    | Dados I2C |
| GPIO22| SCL    | Clock I2C |

### ESP32 - Pinos Utilizados

| Pino | Função | Descrição |
|------|--------|-----------|
| GPIO 2 | LED onboard | LED indicador (pisca durante medição) |
| GPIO 21 | SDA (I2C) | Dados do barramento I2C |
| GPIO 22 | SCL (I2C) | Clock do barramento I2C |
| GPIO 32 | Trigger | Entrada de trigger (LOW = inicia medição) |

### INA228 - Conexão de Medição

O INA228 deve ser conectado em série com a carga a ser medida:

```
FONTE (+) ────[Vin+] INA228 [Vin-]──── CARGA (+)
                │                         │
                │ Shunt (0.015Ω)          │
                │                         │
GND ────────────┴───────────────────────── CARGA (-)
```

**Importante:**
- **Vin+**: Conectado ao positivo da fonte
- **Vin-**: Conectado ao positivo da carga
- **GND**: Conectado ao terra comum
- O resistor shunt interno de 0.015Ω fica entre Vin+ e Vin-

## 📊 ESP32 DOIT DEVKIT V1 - Especificações

### Características Gerais

- **Microcontrolador**: ESP32-D0WDQ6
- **CPU**: Dual-core Xtensa LX6 a 240 MHz
- **Memória Flash**: 4 MB (modelo padrão)
- **RAM**: 520 KB SRAM
- **Tensão de operação**: 3.0V - 3.6V (recomendado: 3.3V)
- **Corrente de consumo**: ~80 mA (ativo), ~10 μA (deep sleep)

### Interfaces e Periféricos

- **GPIOs**: 34 pinos programáveis
- **ADC**: 18 canais de 12 bits (0-3.3V)
- **DAC**: 2 canais de 8 bits
- **PWM**: 16 canais
- **UART**: 3 interfaces seriais
- **SPI**: 3 interfaces
- **I2C**: 2 interfaces (padrão: GPIO21=SDA, GPIO22=SCL)
- **I2S**: 2 interfaces
- **Wi-Fi**: 802.11 b/g/n (2.4 GHz)
- **Bluetooth**: v4.2 BR/EDR e BLE

### Pinout Relevante

| GPIO | Função | Observações |
|------|--------|-------------|
| GPIO 0 | Boot | Deve estar HIGH para boot normal |
| GPIO 2 | LED onboard | LED integrado na placa |
| GPIO 4 | ADC2_CH0 | Entrada analógica |
| GPIO 12 | Boot | Deve estar HIGH para boot normal |
| GPIO 15 | Boot | Deve estar LOW para boot normal |
| GPIO 21 | SDA (I2C) | Padrão para I2C |
| GPIO 22 | SCL (I2C) | Padrão para I2C |
| GPIO 32 | ADC1_CH4 | Entrada analógica, usado como trigger |
| GPIO 33 | ADC1_CH5 | Entrada analógica |
| GPIO 34-39 | Input only | Apenas entrada, sem pull-up/pull-down |

### Limitações

- **Corrente máxima por GPIO**: 12 mA (recomendado: 6 mA)
- **Corrente total de todos os GPIOs**: 200 mA
- **Tensão máxima de entrada**: 3.3V (GPIOs não são 5V tolerant)

## 📡 INA228 - Especificações

### Características Gerais

- **Tipo**: Sensor de corrente, tensão e potência de alta precisão
- **Interface**: I2C (até 1 MHz)
- **Resolução**: 20 bits
- **Tensão de alimentação**: 2.7V a 5.5V
- **Corrente de consumo**: ~1.5 mA (ativo)

### Especificações de Medição

- **Tensão de barramento (Bus Voltage)**:
  - Faixa: 0V a +85V
  - Resolução: 195.3125 μV/LSB
  - Precisão: ±0.1% (típico)

- **Tensão de shunt (Shunt Voltage)**:
  - Faixa: ±163.84 mV (com ADCRange = 0)
  - Faixa: ±40.96 mV (com ADCRange = 1)
  - Resolução: 2.5 nV/LSB (ADCRange = 0)
  - Resolução: 1.25 nV/LSB (ADCRange = 1)

- **Corrente**:
  - Calculada a partir da tensão de shunt
  - Faixa: Depende do resistor shunt
  - **Configuração atual**: 0 a 1A (com shunt de 0.015Ω)

- **Potência**:
  - Calculada: P = V × I
  - Atualizada a cada conversão

### Configuração Atual do Projeto

- **Corrente máxima**: 1.0 A
- **Resistor shunt**: 0.015 Ω
- **Endereço I2C**: 0x40 (padrão)
- **Velocidade I2C**: 400 kHz
- **Tempo de conversão**: 50 μs (mínimo)
- **Média**: 1 amostra (máxima velocidade)
- **ADCRange**: 0 (164 mV, resolução máxima)

### Pinout do INA228

| Pino | Nome | Descrição |
|------|------|-----------|
| 1 | VIN+ | Entrada positiva de tensão |
| 2 | VIN- | Saída positiva (após shunt) |
| 3 | GND | Terra |
| 4 | SCL | Clock I2C |
| 5 | SDA | Dados I2C |
| 6 | ALERT | Saída de alerta (opcional) |
| 7 | V+ | Alimentação (2.7V - 5.5V) |
| 8 | NC | Não conectado |

### Valores Máximos

- **Tensão de entrada (VIN+ a GND)**: 85V máximo
- **Tensão de shunt (VIN+ a VIN-)**: ±163.84 mV (ADCRange=0) ou ±40.96 mV (ADCRange=1)
- **Corrente**: Limitada pela dissipação de potência no shunt
  - Com shunt de 0.015Ω e 1A: P = I²R = 1² × 0.015 = 15 mW
- **Temperatura de operação**: -40°C a +125°C

### Endereços I2C Possíveis

O INA228 pode ter diferentes endereços I2C dependendo do modelo:
- **0x40** (padrão) - Configuração atual
- **0x41** a **0x4F** (dependendo do modelo)

## 📝 Funcionamento do Programa

1. **Inicialização**:
   - Configura I2C em 400 kHz
   - Inicializa INA228 com calibração para 1A máximo e shunt de 0.015Ω
   - Configura para máxima velocidade de amostragem

2. **Aguardando Trigger**:
   - Monitora o pino GPIO 32
   - Quando detecta terra (LOW), inicia medições

3. **Durante a Medição**:
   - Faz leituras contínuas do INA228
   - Calcula tensão (mV), corrente (mA) e potência (mW)
   - Atualiza valores no terminal a cada 0.5 segundos
   - LED onboard pisca indicando medição ativa

4. **Finalização**:
   - Quando GPIO 32 volta para HIGH, para as medições
   - Exibe estatísticas:
     - Total de medições realizadas
     - Tempo total de medição (segundos)
     - Taxa de amostragem (medições por segundo)

## 🔧 Estrutura do Projeto

```
Medidor Energia/
├── src/
│   └── main.cpp          # Código-fonte principal
├── lib/
│   └── INA228-master/    # Biblioteca INA228
├── platformio.ini        # Configuração do PlatformIO
├── .venv/                 # Ambiente virtual Python (criado pelo uv)
└── README.md             # Este arquivo
```

## 🐛 Solução de Problemas

### ESP32 não é detectado

- Verifique se o cabo USB suporta dados (não apenas carregamento)
- Instale os drivers USB apropriados (CH340 ou CP2102)
- Verifique a porta serial no gerenciador de dispositivos

### INA228 não encontrado

- Verifique as conexões I2C (SDA/SCL)
- Verifique se o endereço I2C está correto (0x40)
- Teste com multímetro se há 3.3V no VCC do INA228
- Verifique se não há curto-circuito nas conexões

### Valores de medição incorretos

- Verifique se o resistor shunt está correto (0.015Ω)
- Verifique a calibração no código (1.0A máximo)
- Certifique-se de que as conexões Vin+/Vin- estão corretas
- Verifique se a carga está conectada corretamente

### Taxa de amostragem baixa

- O código já está otimizado para máxima velocidade
- Verifique se não há outros processos consumindo CPU
- Certifique-se de que o I2C está configurado para 400 kHz

## 📚 Referências

- [Documentação do ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [Datasheet INA228](https://www.ti.com/lit/ds/symlink/ina228.pdf)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [uv Documentation](https://docs.astral.sh/uv/)

## 📄 Licença

Este projeto é de uso educacional e de pesquisa.

---

**Desenvolvido para:** UFSJ - Mestrado  
**Projeto:** Medidor de Energia

