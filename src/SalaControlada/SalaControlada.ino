/**
 * @file codigofinal.ino
 * @brief Sistema de Automação de Sala com ESP32.
 * @author Victor Augusto
 * @date 01/07/2025
 *
NEW SKETCH

 * @details
 * Este projeto controla uma sala utilizando um ESP32, integrando:
 * - Controle de acesso via RFID (MFRC522).
 * - Monitoramento de temperatura e umidade (DHT11).
 * - Detecção de presença com sensor ultrassônico.
 * - Controle de iluminação e ventilação (manual e automática).
 * - Desligamento automático de luz/ventoinha manual por ausência.
 * - Interface de controle web via Wi-Fi com atualização automática.
 * - Display LCD I2C para feedback local.
 */

// ==============================================================================
// BIBLIOTECAS
// ==============================================================================

#include <SPI.h>               // Comunicação SPI (usada pelo RFID)
#include <MFRC522.h>           // Biblioteca do leitor RFID MFRC522
#include <Wire.h>              // Comunicação I2C (usada pelo LCD)
#include <LiquidCrystal_I2C.h> // Biblioteca para LCD I2C
#include <DHT.h>               // Biblioteca para sensor DHT11 (temperatura/umidade)
#include <WiFi.h>              // Biblioteca para conexão Wi-Fi
#include <WebServer.h>         // Biblioteca para servidor web embutido
#include <Ultrasonic.h>        // Biblioteca para sensor ultrassônico
#include <ESP32Servo.h>        // Biblioteca para controle de servo motor no ESP32

// ==============================================================================
// CONFIGURAÇÕES E CONSTANTES
// ==============================================================================

const char *ssid1 = "f15-victor";      // Nome da rede Wi-Fi
const char *password1 = "31441232";          // Senha da rede Wi-Fi
const int DISTANCIA_PRESENCA_CM = 15;       // Distância máxima para considerar presença (em cm)
String mensagemSistema = "";                // Mensagem do sistema para feedback na web/LCD
int tempacionamento = 26;                   // Temperatura para ligar ventoinha automática
int tempdesligamento = 24;                  // Temperatura para desligar ventoinha automática

// Definição dos pinos do ESP32 para cada periférico
const byte PINO_RFID_SS = 5;                // Pino SS do RFID
const byte PINO_RFID_RST = 0;               // Pino RST do RFID
const byte PINO_DHT = 15;                   // Pino do sensor DHT11
const byte PINO_BUZZER = 32;                // Pino do buzzer
const byte PINO_TRIG = 16;                  // Pino TRIG do ultrassônico
const byte PINO_ECHO = 17;                  // Pino ECHO do ultrassônico
const byte PINO_VENTOINHA_AUTO = 2;         // Pino ventoinha automática
const byte PINO_VENTOINHA_MANUAL = 13;      // Pino ventoinha manual
const byte PINO_LUZ = 14;                   // Pino da luz

#define LCD_ENDERECO 0x27                   // Endereço I2C do LCD
#define LCD_COLUNAS  16                     // Número de colunas do LCD
#define LCD_LINHAS   2                      // Número de linhas do LCD

const byte servo = 4;                       // Pino do servo motor
Servo ServoPorta;                           // Objeto para controlar o servo motor

// Variáveis de posição do servo motor (em microssegundos)
const int posicaoAberta = 500;              // Posição aberta
const int posicaoFechada = 1495;            // Posição fechada

const long tempoMinimoPresenca = 5000;      // Tempo mínimo de presença para acionar luz (ms)
unsigned long tempoInicioPresenca = 0;

// ==============================================================================
// ESTRUTURAS DE DADOS
// ==============================================================================

struct Usuario {                            // Estrutura para armazenar usuários autorizados
  byte uid[4];                              // UID do cartão RFID (4 bytes)
  const char *nome;                         // Nome do usuário
};

const Usuario usuariosAutorizados[] = {     // Lista de usuários autorizados
    {{207, 219, 197, 196}, "Anne Beatriz"},
    {{30, 157, 226, 105}, "Victor Augusto"}
};
const int totalUsuarios = sizeof(usuariosAutorizados) / sizeof(usuariosAutorizados[0]); // Total de usuários

bool portaAberta = false;                   // Estado da porta (aberta/fechada)
byte ultimoUID[4] = {0, 0, 0, 0};           // UID do último usuário que abriu a porta

// ==============================================================================
// INSTÂNCIAS DE OBJETOS
// ==============================================================================

LiquidCrystal_I2C lcd(LCD_ENDERECO, LCD_COLUNAS, LCD_LINHAS); // LCD I2C
MFRC522 rfid(PINO_RFID_SS, PINO_RFID_RST);                    // Leitor RFID
DHT dht(PINO_DHT, DHT11);                                     // Sensor DHT11
Ultrasonic ultrasonic1(PINO_TRIG, PINO_ECHO);                 // Sensor ultrassônico
WebServer server(80);                                         // Servidor web na porta 80

// ==============================================================================
// VARIÁVEIS GLOBAIS DE ESTADO
// ==============================================================================

bool ventilacaoState = false;               // Estado da ventoinha manual
bool iluminacaoState = false;               // Estado da luz
bool ocupacao = false;                      // Estado de ocupação da sala
int temperaturaAtual = 0;                   // Temperatura lida do sensor
bool ventilacaoAutomaticaState = false;     // Estado da ventoinha automática
unsigned long millisAnterior = 0;           // Armazena o tempo da última leitura de temperatura
const long intervaloLeituraTemp = 5000;     // Intervalo entre leituras de temperatura (ms)
bool luzDesligadaManualmente = false;       // NOVO: Flag para indicar que a luz foi desligada manualmente com a sala ocupada

// ==============================================================================
// DECLARAÇÃO DE FUNÇÕES (PROTÓTIPOS)
// ==============================================================================

void handleRoot();                          // Handler da página principal do servidor web
void controleLuz(bool ligar);               // Função para controlar a luz
void controleVentilacao(bool ligar);        // Função para controlar a ventoinha manual
void redirectToRoot();                      // Redireciona para a página principal
void lerRfid();                             // Função para ler o cartão RFID
void atualizarEstadoOcupacao();             // Atualiza a variável de ocupação
void atualizarDisplayTempUmi();             // Atualiza o display LCD com temp/umidade
void controleAutomaticoVentoinha();         // Controla a ventoinha automática
void verificarDesligamentoPorAusencia();    // Desliga luz/ventoinha manual se sala vazia

// ==============================================================================
// SETUP: Executado uma vez na inicialização do ESP32
// ==============================================================================

void setup() {
    ServoPorta.setPeriodHertz(50);          // Define frequência do servo (50Hz)
    ServoPorta.attach(servo, 500, 2500);    // Anexa o servo ao pino, define limites
    ServoPorta.write(0);                    // Move servo para posição inicial (0°)
    delay(2000);                            // Aguarda 2 segundos
    ServoPorta.write(90);                   // Move servo para posição 90°
    Serial.begin(115200);                   // Inicializa comunicação serial (debug)
    pinMode(PINO_BUZZER, OUTPUT);           // Define pino do buzzer como saída
    pinMode(PINO_LUZ, OUTPUT);              // Define pino da luz como saída
    pinMode(PINO_VENTOINHA_AUTO, OUTPUT);   // Define pino ventoinha automática como saída
    pinMode(PINO_VENTOINHA_MANUAL, OUTPUT); // Define pino ventoinha manual como saída
    digitalWrite(PINO_LUZ, LOW);            // Garante luz desligada
    digitalWrite(PINO_VENTOINHA_AUTO, LOW); // Garante ventoinha automática desligada
    digitalWrite(PINO_VENTOINHA_MANUAL, LOW);// Garante ventoinha manual desligada
    dht.begin();                            // Inicializa sensor DHT11
    SPI.begin();                            // Inicializa barramento SPI
    rfid.PCD_Init();                        // Inicializa leitor RFID
    lcd.init();                             // Inicializa LCD
    lcd.backlight();                        // Liga backlight do LCD
    lcd.clear();                            // Limpa display LCD
    Serial.println(F("Sensores e atuadores inicializados.")); // Mensagem debug
    lcd.setCursor(0, 0);                    // Cursor LCD início
    lcd.print("Conectando WiFi");           // Mensagem LCD
    WiFi.begin(ssid1, password1);           // Inicia conexão Wi-Fi
    while (WiFi.status() != WL_CONNECTED) {   // Aguarda conexão Wi-Fi
        delay(500);
        Serial.print(".");
    }
    lcd.clear();                            // Limpa LCD
    lcd.print("Conectado!");                // Mensagem LCD
    lcd.setCursor(0, 1);                    // Segunda linha
    lcd.print(WiFi.localIP());              // Mostra IP no LCD
    Serial.print(F("\nEndereço IP: "));     // Mostra IP no Serial
    Serial.println(WiFi.localIP());
    delay(3000);                            // Aguarda 3 segundos
    server.on("/", handleRoot);             // Rota principal do servidor web
    server.on("/luz/on", []() { controleLuz(true); });        // Rota para ligar luz
    server.on("/luz/off", []() { controleLuz(false); });      // Rota para desligar luz
    server.on("/ventilacao/on", []() { controleVentilacao(true); });  // Ligar ventoinha manual
    server.on("/ventilacao/off", []() { controleVentilacao(false); }); // Desligar ventoinha manual
    server.onNotFound(handleRoot);          // Qualquer outra rota: página principal
    server.begin();                         // Inicia servidor web
    Serial.println(F("Servidor HTTP iniciado.")); // Mensagem debug
    lcd.clear();                            // Limpa LCD
}

// ==============================================================================
// LOOP: Executado continuamente após o setup
// ==============================================================================

void loop() {
    server.handleClient();                  // Processa requisições web
    lerRfid();                              // Lê cartão RFID (controle de acesso)
    atualizarEstadoOcupacao();              // Atualiza estado de ocupação da sala
    atualizarDisplayTempUmi();              // Atualiza LCD com temp/umidade
    controleAutomaticoVentoinha();          // Controla ventoinha automática
    verificarDesligamentoPorAusencia();     // Desliga luz/ventoinha manual se sala vazia
}

// ==============================================================================
// FUNÇÕES PRINCIPAIS DE LÓGICA
// ==============================================================================

/**
 * @brief Desliga a luz e a ventoinha manual se a sala estiver vazia para economizar energia.
 * @note Esta função não afeta a ventoinha automática de temperatura.
 */
void verificarDesligamentoPorAusencia() {
    // Se a sala NÃO está ocupada E (a luz está ligada OU a ventoinha manual está ligada)
    if (!ocupacao && (iluminacaoState || ventilacaoState)) {
        digitalWrite(PINO_LUZ, LOW);            // Desliga luz
        iluminacaoState = false;                // Atualiza estado da luz
        digitalWrite(PINO_VENTOINHA_MANUAL, LOW);// Desliga ventoinha manual
        ventilacaoState = false;                // Atualiza estado da ventoinha manual
        mensagemSistema = "Luz e ventoinha manual desligadas por ausência."; // Mensagem para web/LCD
        Serial.println("AUTOMAÇÃO: Luz e ventoinha manual desligadas, sala vazia."); // Debug
        delay(50);                              // Pequeno delay para evitar sobrescrita da mensagem
    }
}

/**
 * @brief Lê o sensor RFID, verifica a autorização e controla a cancela.
 */
void lerRfid(void) {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return; // Se não há novo cartão, sai

    bool autorizado = false;                    // Flag de autorização
    const char *nomeUsuario = "";               // Nome do usuário

    // Verifica se UID lido está na lista de autorizados
    for (int i = 0; i < totalUsuarios; i++) {
        if (memcmp(rfid.uid.uidByte, usuariosAutorizados[i].uid, rfid.uid.size) == 0) {
            autorizado = true;
            nomeUsuario = usuariosAutorizados[i].nome;
            break;
        }
    }

    lcd.clear();                                // Limpa LCD
    lcd.setCursor(0, 0);                        // Cursor início

    if (autorizado) {                           // Se autorizado
        lcd.print("Bem-vindo:");                // Mensagem LCD
        lcd.setCursor(0, 1);
        lcd.print(nomeUsuario);                 // Mostra nome do usuário
        Serial.print(">> Usuario: ");
        Serial.println(nomeUsuario);            // Debug
        delay(1500);                            // Pausa para exibir nome
        ServoPorta.detach();                    // Desanexa servo (proteção)
        delay(100);
        tone(PINO_BUZZER, 659, 150);
        delay(200);
        tone(PINO_BUZZER, 784, 150);
        delay(200);
        tone(PINO_BUZZER, 880, 150);
        delay(200);
        noTone(PINO_BUZZER);                    // Para buzzer
        ServoPorta.attach(servo, 500, 2500);    // Reanexa servo
        delay(250);

        lcd.clear();
        lcd.setCursor(0, 0);

        if (!portaAberta) {                     // Se porta está fechada
            ServoPorta.writeMicroseconds(posicaoAberta); // Abre porta
            delay(100);
            portaAberta = true;                 // Atualiza estado
            memcpy(ultimoUID, rfid.uid.uidByte, 4); // Salva UID
            lcd.print("Porta: ABERTA");         // Mensagem LCD
            Serial.println(">> Porta ABERTA."); // Debug
            ServoPorta.detach();                // Desanexa servo (proteção)
            delay(600);
            tone(PINO_BUZZER, 1000, 150);       // Buzzer: nota aguda
            delay(200);
            tone(PINO_BUZZER, 1500, 150);       // Buzzer: nota mais aguda
            delay(200);
            noTone(PINO_BUZZER);                // Para buzzer
            ServoPorta.attach(servo, 500, 2500); // Reanexa servo
            delay(250);

        } else if (memcmp(rfid.uid.uidByte, ultimoUID, 4) == 0) { // Mesmo usuário fecha
            ServoPorta.writeMicroseconds(posicaoFechada); // Fecha porta
            portaAberta = false;                // Atualiza estado
            lcd.print("Porta: FECHADA");        // Mensagem LCD
            Serial.println(">> Porta FECHADA.");// Debug
        } else {                                // Outro usuário tenta fechar
            lcd.print("Ja aberta por");
            lcd.setCursor(0, 1);
            lcd.print("outro usuario");
            Serial.println(">> Outro usuario tentou fechar a porta.");
            ServoPorta.detach();                // Desanexa servo (proteção)
            delay(100);
            tone(PINO_BUZZER, 750, 200); // Aviso sonoro mediano de 200ms
            delay(200);
            tone(PINO_BUZZER, 750, 200); // Aviso sonoro mediano de 200ms
            delay(200);
            noTone(PINO_BUZZER);                // Para buzzer
            ServoPorta.attach(servo, 500, 2500); // Reanexa servo
            delay(250);
        }
    } else {                                    // Se não autorizado
        lcd.print("Acesso NEGADO");             // Mensagem LCD
        lcd.setCursor(0, 1);
        lcd.print("Cartao invalido");
        Serial.println(">> Acesso Negado.");    // Debug
        delay(100);

        ServoPorta.detach();                    // Desanexa servo
        delay(100);
        for (int i = 0; i < 2; i++) {           // Buzzer: 2 bipes graves
            tone(PINO_BUZZER, 300, 250);
            delay(250);
        }
        noTone(PINO_BUZZER);                    // Para buzzer
        ServoPorta.attach(servo, 500, 2500);    // Reanexa servo
        delay(250);
    }

    delay(1500);                                // Pausa para feedback
    rfid.PICC_HaltA();                          // Finaliza comunicação com cartão
    rfid.PCD_StopCrypto1();                     // Finaliza criptografia
    millisAnterior = millis();                  // Atualiza tempo para leitura temp/umi
}

/**
 * @brief Lê o sensor ultrassônico, atualiza 'ocupacao' e gerencia a luz automática.
 * @details A luz só acende automaticamente se não tiver sido desligada manualmente
 * enquanto a sala estava ocupada. A flag é resetada quando a sala fica vazia.
 */
void atualizarEstadoOcupacao() {
    long distancia = ultrasonic1.read();
    bool presencaAtual = (distancia > 0 && distancia <= DISTANCIA_PRESENCA_CM);
    ocupacao = presencaAtual;

    if (presencaAtual) {
        if (tempoInicioPresenca == 0) {
            tempoInicioPresenca = millis();
        }
        // ALTERADO: Adicionado a verificação '!luzDesligadaManualmente'
        // A luz só liga automaticamente se a flag de desligamento manual não estiver ativa.
        if ((millis() - tempoInicioPresenca >= tempoMinimoPresenca) && !iluminacaoState && !luzDesligadaManualmente) {
            Serial.println("AUTOMAÇÃO: Presença detectada. Ligando a luz.");
            controleLuz(true); // A própria função controleLuz(true) vai resetar a flag.
        }
    } else { // Se a sala está vazia
        tempoInicioPresenca = 0;
        // NOVO: Reseta a flag quando a sala fica vazia.
        // Isso permite que a luz acenda automaticamente na próxima vez que alguém entrar.
        luzDesligadaManualmente = false;
    }
}


/**
 * @brief Lê o sensor DHT11 e atualiza o display LCD. Usa um timer não bloqueante.
 */
void atualizarDisplayTempUmi() {
    if (millis() - millisAnterior < intervaloLeituraTemp) return; // Só executa se passou o intervalo
    millisAnterior = millis();                  // Atualiza tempo da última leitura
    float umidade = dht.readHumidity();         // Lê umidade
    float temp = dht.readTemperature();         // Lê temperatura
    if (isnan(umidade) || isnan(temp)) {        // Se leitura inválida
        Serial.println(F("Falha ao ler dados do sensor DHT!"));
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERRO SENSOR");
        delay(1000);
        return;
    }
    temperaturaAtual = (int)temp;               // Atualiza variável global de temperatura
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Umi: "); lcd.print(umidade, 1); lcd.print("%"); // Mostra umidade
    lcd.setCursor(0, 1); lcd.print("Temp: "); lcd.print(temperaturaAtual); lcd.print((char)223); lcd.print("C"); // Mostra temp
}

/**
 * @brief Controla a ventoinha automática com base na temperatura.
 */
void controleAutomaticoVentoinha() {
    if (temperaturaAtual >= tempacionamento && !ventilacaoAutomaticaState) { // Se temp alta e ventoinha desligada
        digitalWrite(PINO_VENTOINHA_AUTO, HIGH); // Liga ventoinha automática
        ventilacaoAutomaticaState = true;       // Atualiza estado
        mensagemSistema = "Ventoinha LIGADA automaticamente por temperatura alta.";
        Serial.println("Ventoinha AUTOMÁTICA LIGADA.");
    } else if (temperaturaAtual < tempdesligamento && ventilacaoAutomaticaState) { // Se temp baixa e ventoinha ligada
        digitalWrite(PINO_VENTOINHA_AUTO, LOW); // Desliga ventoinha automática
        ventilacaoAutomaticaState = false;      // Atualiza estado
        mensagemSistema = "Ventoinha DESLIGADA automaticamente.";
        Serial.println("Ventoinha AUTOMÁTICA DESLIGADA.");
    }
}

// ==============================================================================
// FUNÇÕES DE CONTROLE (ACIONADAS PELO SERVIDOR WEB)
// ==============================================================================

/**
 * @brief Controla a iluminação, gerenciando a flag de controle manual.
 * @param ligar Booleano que define se a ação é para ligar (true) ou desligar (false).
 */
void controleLuz(bool ligar) {
    if (ligar) {                                // Se for para ligar
        if (ocupacao) {                         // Só liga se sala ocupada
            digitalWrite(PINO_LUZ, HIGH);       // Liga luz
            iluminacaoState = true;             // Atualiza estado
            luzDesligadaManualmente = false;    // NOVO: Reseta a flag ao ligar manualmente.
            mensagemSistema = "Luz ligada com sucesso.";
        } else {
            mensagemSistema = "⚠️ N&atilde;o &eacute; poss&iacute;vel ligar a luz: sala est&aacute; vazia.";
        }
    } else {                                    // Se for para desligar
        digitalWrite(PINO_LUZ, LOW);            // Desliga luz
        iluminacaoState = false;                // Atualiza estado
        if (ocupacao) {                         // NOVO: Se desligou com a sala ocupada...
            luzDesligadaManualmente = true;     // ...ativa a flag para bloquear o acendimento automático.
        }
        mensagemSistema = "Luz desligada.";
    }
    redirectToRoot();                           // Redireciona para página principal
}


/**
 * @brief Controla a ventilação manual, verificando a ocupação da sala.
 * @param ligar Booleano que define se a ação é para ligar (true) ou desligar (false).
 */
void controleVentilacao(bool ligar) {
    if (ligar) {                                // Se for para ligar
        if (ocupacao) {                         // Só liga se sala ocupada
            digitalWrite(PINO_VENTOINHA_MANUAL, HIGH); // Liga ventoinha manual
            ventilacaoState = true;             // Atualiza estado
            mensagemSistema = "Ventilacao manual ligada com sucesso.";
        } else {
            mensagemSistema = "⚠️ N&atilde;o &eacute; poss&iacute;vel ligar a ventoinha: sala est&aacute; vazia.";
        }
    } else {                                    // Se for para desligar
        digitalWrite(PINO_VENTOINHA_MANUAL, LOW); // Desliga ventoinha manual
        ventilacaoState = false;                // Atualiza estado
        mensagemSistema = "Ventilacao manual desligada.";
    }
    redirectToRoot();                           // Redireciona para página principal
}

// ==============================================================================
// FUNÇÕES DO SERVIDOR WEB (HANDLERS)
// ==============================================================================

/**
 * @brief Gera e envia a página HTML principal para o cliente (navegador).
 */
void handleRoot() {
    String html = "<!DOCTYPE html><html><head><title>Controle de Sala</title><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='10'>"; // Atualiza página a cada 10s
    html += "<style>html{font-family: Helvetica, Arial, sans-serif; display: inline-block; margin: 0px auto; text-align: center;} body{background-color: #f4f4f4; max-width: 600px; margin: 0 auto;} h1{color: #333;} h3{color: #555; border-top: 2px solid #ccc; padding-top: 15px; margin-top: 20px;} .button{background-color:#4CAF50;border:none;color:white;padding:14px 30px;text-decoration:none;font-size:22px;margin:2px;cursor:pointer;border-radius:8px;} .button2{background-color:#f44336;} p{font-size: 18px;} .status{font-weight: bold;} .msg{color:blue; font-weight:bold; background-color: #e0e0ff; padding: 10px; border-radius: 5px;}</style></head><body><h1>Controle da Sala - ESP32</h1>";
    if (mensagemSistema != "") {                // Se há mensagem do sistema
        html += "<p class='msg'>" + mensagemSistema + "</p>"; // Mostra mensagem
        mensagemSistema = "";                   // Limpa mensagem
    }
    html += "<p><b>Temperatura Atual:</b> " + String(temperaturaAtual) + "&deg;C</p>"; // Mostra temp
    html += "<p><b>Ocupa&ccedil;&atilde;o da Sala:</b> <span class='status'>" + String(ocupacao ? "OCUPADA" : "LIVRE") + "</span></p>"; // Ocupação
    html += "<h3>Ilumina&ccedil;&atilde;o</h3>";
    html += "<p>Estado: <span class='status'>" + String(iluminacaoState ? "LIGADA" : "DESLIGADA") + "</span></p>"; // Estado luz
    if (iluminacaoState) html += "<a href='/luz/off'><button class='button button2'>Desligar</button></a>"; // Botão desligar
    else html += "<a href='/luz/on'><button class='button'>Ligar</button></a>"; // Botão ligar
    html += "<h3>Ventila&ccedil;&atilde;o (Autom&aacute;tica)</h3>";
    html += "<p>Aciona em: " + String(tempacionamento) + "&deg;C</p>"; // Temp de acionamento
    html += "<p>Estado: <span class='status'>" + String(ventilacaoAutomaticaState ? "LIGADA" : "DESLIGADA") + "</span></p>"; // Estado ventoinha auto
    html += "<h3>Ventila&ccedil;&atilde;o (Manual)</h3>";
    html += "<p>Estado: <span class='status'>" + String(ventilacaoState ? "LIGADA" : "DESLIGADA") + "</span></p>"; // Estado ventoinha manual
    if (ventilacaoState) html += "<a href='/ventilacao/off'><button class='button button2'>Desligar</button></a>"; // Botão desligar
    else html += "<a href='/ventilacao/on'><button class='button'>Ligar</button></a>"; // Botão ligar
    html += "</body></html>";
    server.send(200, "text/html", html);        // Envia página HTML ao navegador
}

/**
 * @brief Redireciona o navegador do cliente para a página raiz ("/").
 * Usado após uma ação (clique de botão) para atualizar a página.
 */
void redirectToRoot() {
    server.sendHeader("Location", "/");         // Header de redirecionamento
    server.send(302, "text/plain", "");         // Resposta HTTP 302 (redirect)
}