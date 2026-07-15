# Sala Controlada - Automação com ESP32

[![Relatório Completo](https://img.shields.io/badge/📖_Ver_Relatório_Completo-blue?style=for-the-badge)](docs/Documentacao_do_projeto.pdf)
[![Vídeo de Demonstração](https://img.shields.io/badge/▶️_Assistir_Vídeo_no_YouTube-red?style=for-the-badge)](https://www.youtube.com/watch?v=5RqwbafUiyc)
[![Fluxograma ao Vivo](https://img.shields.io/badge/🔄_Acessar_Fluxograma_ao_Vivo-orange?style=for-the-badge)](https://abre.ai/m5MO)

> ⚠️ **Aviso sobre o Relatório:** Devido à alta resolução e formatação do PDF, o visualizador do GitHub pode exibir o erro `Invalid PDF`. **Para ler o documento, faça o download clicando no botão "Download raw file" no canto superior direito da página do arquivo.**


Este projeto consiste em um sistema de automação residencial inteligente que integra **controle de acesso físico**, **monitoramento ambiental (temperatura e umidade)** e **detecção de presença**. Ele utiliza o microcontrolador ESP32 como unidade central e permite o gerenciamento e visualização local e remota (via Web).

O sistema foi desenvolvido como Projeto Final da disciplina ECAE00 (Introdução à Engenharia e ao Método Científico) no curso de Engenharia de Controle e Automação da **Universidade Federal de Itajubá (UNIFEI)**.

## 🚀 Funcionalidades

- **Controle de Acesso por RFID**: Entrada liberada via leitura de TAGs (MFRC522) previamente autorizadas, controlando a abertura e fechamento de uma cancela (servo motor).
- **Detecção de Presença**: Um sensor ultrassônico (HC-SR04) monitora a ocupação da sala, e com base nisso controla automaticamente a iluminação.
- **Monitoramento e Controle de Clima**: Um sensor DHT11 monitora temperatura e umidade. Ventilação pode ser acionada manualmente ou automaticamente, de acordo com limiares definidos (liga acima de 27°C e desliga abaixo de 24°C).
- **Interface Web Integrada**: O ESP32 hospeda uma página web onde é possível visualizar em tempo real temperatura, umidade, ocupação, estado da iluminação e acionar dispositivos manualmente.
- **Display Local e Avisos Sonoros**: Display LCD (I2C) apresenta dados em tempo real. Um buzzer fornece feedback sonoro imediato para acesso permitido, negado, e outros eventos.
- **Eficiência Energética**: O sistema detecta a ausência de pessoas na sala e desliga automaticamente a iluminação e ventilação após um tempo de inatividade para economizar energia.

## 🛠️ Hardware e Componentes

- **Controladora**: ESP32
- **Sensores**:
  - Leitor RFID MFRC522 (Acesso)
  - DHT11 (Temperatura e Umidade)
  - Ultrassônico HC-SR04 (Presença)
- **Atuadores / Saída**:
  - Servo Motor SG90 (Tranca/Cancela)
  - Display LCD 16x2 com Módulo I2C
  - Buzzer Passivo/Ativo
  - Transistor TIP120 (Chaveamento da Ventoinha)
  - LED e Ventoinha (Para demonstração de iluminação e refrigeração)

## 🔌 Pinout (Esquema de Ligações)

| Componente | Pino no ESP32 | Detalhes |
| :--- | :--- | :--- |
| **Servo Motor** | GPIO 4 | Controle PWM (50Hz) |
| **LCD I2C** | GPIO 21 (SDA), GPIO 22 (SCL) | Alimentação 5V |
| **RFID MFRC522** | GPIO 5 (SDA), 18 (SCK), 23 (MOSI), 19 (MISO), 0 (RST) | Alimentação 3.3V |
| **HC-SR04** | GPIO 16 (TRIG), GPIO 17 (ECHO) | Alimentação 5V |
| **Buzzer** | GPIO 32 | - |
| **DHT11** | GPIO 15 | Requer resistor Pull-up 1kΩ no pino de sinal |
| **LED (Luz)** | GPIO 14 | Via resistor 330Ω |
| **Ventoinha (Auto)** | GPIO 2 | Base do Transistor TIP120 via Diodo |
| **Ventoinha (Man)** | GPIO 13 | Base do Transistor TIP120 via Diodo |

> **Nota**: As portas da ESP32 operam em 3.3V, atente-se à tensão e corrente limite. O chaveamento da ventoinha é feito utilizando transistor TIP120 para garantir a corrente necessária.

## 📂 Estrutura do Repositório

```
SalaControlada-ESP32/
├── src/
│   └── SalaControlada/
│       └── SalaControlada.ino  # Código fonte principal (Arduino IDE)
├── docs/
│   ├── Documentacao_do_projeto.pdf # Relatório completo do projeto
│   └── Diagramas de arquitetura (.mmd, .svg)
├── README.md                   # Este arquivo
└── .gitignore                  # Arquivos ignorados pelo Git
```

## ⚙️ Como Executar

1. **Dependências (Bibliotecas da Arduino IDE):**
   - `MFRC522` por GithubCommunity
   - `LiquidCrystal I2C` por Frank de Brabander
   - `DHT sensor library` por Adafruit
   - `Ultrasonic` por Erick Simões
   - `ESP32Servo` por Kevin Harrington

2. **Configuração Wi-Fi:**
   No arquivo `SalaControlada.ino`, altere as variáveis de rede para se conectar ao seu ambiente:
   ```cpp
   const char *ssid1 = "SEU_WIFI";
   const char *password1 = "SUA_SENHA";
   ```

3. **Gravação:**
   - Conecte a placa ESP32.
   - Configure a IDE na placa correta ("DOIT ESP32 DEVKIT V1" ou semelhante).
   - Compile e efetue o Upload.

4. **Uso:**
   - Abra o Monitor Serial (`115200` baud) para acompanhar o status e descobrir o IP atribuído à placa.
   - Acesse o IP gerado através de qualquer navegador em um dispositivo conectado na mesma rede Wi-Fi.

## 👨‍💻 Autores

**Desenvolvedor do Código e Relatório:**
- Victor Augusto de Aquino Silvério (2025016677)

**Outros Membros do Projeto:**
- Arthur Moraes Marques dos Santos (2025001389)
- Breno Gabriel Alves de Souza (2025013746)
- Gustavo Fernandes Gonçalves de Lima (2025002985)
- Marco Antônio Carneiro Vilela (2025005299)

*Projeto desenvolvido na **UNIFEI - Universidade Federal de Itajubá** (2025.1).*
