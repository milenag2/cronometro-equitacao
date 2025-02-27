//LoraReceiverQueDeuCerto//
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <iostream>
#include <string>
#include <ESPmDNS.h>


// Import Wi-Fi library
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>

// Libraries to get time from NTP Server
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <SPIFFS.h>
 
/* Definicoes para comunicação com radio LoRa */
#define SCK_LORA           5
#define MISO_LORA          19
#define MOSI_LORA          27
#define RESET_PIN_LORA     14
#define SS_PIN_LORA        18
 
#define HIGH_GAIN_LORA     20  /* dBm */
#define BAND               915E6  /* 915MHz de frequencia */
 
/* Definicoes do OLED */
#define OLED_SDA_PIN    4
#define OLED_SCL_PIN    15
#define SCREEN_WIDTH    128 
#define SCREEN_HEIGHT   64  
#define OLED_ADDR       0x3C 
#define OLED_RESET      16
 
/* Offset de linhas no display OLED */
#define OLED_LINE1     0
#define OLED_LINE2     10
#define OLED_LINE3     20
#define OLED_LINE4     30
#define OLED_LINE5     40
#define OLED_LINE6     50
 
/* Definicoes gerais */
#define DEBUG_SERIAL_BAUDRATE    115200

// Replace with ESP32 network credentials
const char* ssid     = "ESP32-WIFI";
const char* password = "testando";

// Initialize variables to get and save LoRa data
int rssi;
String minutos2="";
String segundos2="";
String centesimos2="";

// Define NTP Client to get time
WiFiUDP ntpUDP;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
/* Variaveis e objetos globais */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if (var == "MINUTOS"){
    return minutos2;
  }
  else if (var == "SEGUNDOS"){
    return segundos2;
  }
  else if (var == "CENTESIMOS"){
    return centesimos2;
  }
  return String();
}

/* Local prototypes */
bool display_ok=false;
bool wifi_connected=false;
bool status_init=false;  
void display_init(void);
bool init_comunicacao_lora(void);
bool cavaloJaPassou = false; //Indica se o cronometro começou a contar ou não
bool pareiCronometro=false;// variável que vamos usar para parar o cronometro
long tempo;
 
/* Funcao: inicializa comunicacao com o display OLED
 * Parametros: nenhnum
 * Retorno: nenhnum
*/ 
void display_init(void)
{
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) 
    {
        Serial.println("[LoRa Receiver] Falha ao inicializar comunicacao com OLED");        
    }
    else
    {
        Serial.println("[LoRa Receiver] Comunicacao com OLED inicializada com sucesso");
     
        /* Limpa display e configura tamanho de fonte */
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
    }
}
 
/* Funcao: inicia comunicação com chip LoRa
 * Parametros: nenhum
 * Retorno: true: comunicacao ok
 *          false: falha na comunicacao
*/
bool init_comunicacao_lora(void)
{
    status_init = false;
    Serial.println("[LoRa Receiver] Tentando iniciar comunicacao com o radio LoRa...");
    SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_PIN_LORA);
    LoRa.setPins(SS_PIN_LORA, RESET_PIN_LORA, LORA_DEFAULT_DIO0_PIN);
     
    if (!LoRa.begin(BAND)) 
    {
        Serial.println("[LoRa Receiver] Comunicacao com o radio LoRa falhou. Nova tentativa em 1 segundo...");        
        delay(1000);
        status_init = false;
    }
    else
    {
        /* Configura o ganho do receptor LoRa para 20dBm, o maior ganho possível (visando maior alcance possível) */ 
        LoRa.setTxPower(HIGH_GAIN_LORA); 
        Serial.println("[LoRa Receiver] Comunicacao com o radio LoRa ok");
        status_init = true;
    }
 
    return status_init;
}

/* Funcao de setup */
void setup() 
{
    Serial.begin(DEBUG_SERIAL_BAUDRATE);
    while (!Serial);
 
    /* Tenta, até obter sucesso, comunicacao com o chip LoRa */
    while(init_comunicacao_lora() == false); 

    if(status_init){
        /* Configuracao da I²C para o display OLED */
      Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
   
      /* Display init */
      display_init();
   
      /* Print message telling to wait */
      display.clearDisplay();    
      display.setCursor(0, OLED_LINE1);
      display.print("Aguarde...");
      display.display();
      display_ok=true;
        if(display_ok){
        WiFi.softAP(ssid, password); //Inicia o ponto de acesso
        Serial.print("Se conectando a: "); //Imprime mensagem sobre o nome do ponto de acesso
        Serial.println(ssid);
        IPAddress ip = WiFi.softAPIP(); //Endereço de IP
        
        delay(500);

        Serial.print("Endereço de IP: "); //Imprime o endereço de IP
        Serial.println(ip);
        Serial.println("Servidor online"); //Imprime a mensagem de início
        wifi_connected=true;}
        }
   if(!SPIFFS.begin()){
   Serial.println("An Error has occurred while mounting SPIFFS");
   return;
    }
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/minutos2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", minutos2.c_str());
  });
  server.on("/segundos2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", segundos2.c_str());
  });
  server.on("/centesimos2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", centesimos2.c_str());
  });
    server.on("/ime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/ime.png", "image/png");
  });
    server.on("/cavalaria", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/cavalaria.png", "image/png");
  });
    server.on("/cavalo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/cavalo.png", "image/png");
  });
  // Start server
  server.begin();
  delay(500);

  if(MDNS.begin("esp32")){
    Serial.println("MNDS responder started");
    }   
}
 
/* Programa principal */
void loop() {
    if(!cavaloJaPassou && !pareiCronometro){
      display.clearDisplay();    
      display.setCursor(0, OLED_LINE1);
      display.print("Cronometro");
      display.setCursor(0, OLED_LINE2);
      display.print("00:00:00");
      display.setCursor(0, OLED_LINE3);
      display.print("Acesse o cronometro:");
      display.setCursor(0, OLED_LINE4);
      display.print(WiFi.softAPIP());

      display.display();
    }
    /* Verifica se chegou alguma informação do tamanho esperado */
      int packetSize = LoRa.parsePacket();
      delay(1);

      Serial.println(packetSize);

      if (packetSize!=0) {
        // received a packet
        // read packet
        while (LoRa.available()) {
          Serial.print((char)LoRa.read());
          tempo=millis();
          if(!cavaloJaPassou){
          cavaloJaPassou=true;
          }
          else
          {
            cavaloJaPassou=false;
            pareiCronometro=true;
          }
        }
        // print RSSI of packet
        rssi = LoRa.packetRssi();
        Serial.print("' with RSSI ");
        Serial.println(LoRa.packetRssi());

      }
      long deltaTempo=(millis()-tempo);
      long minutos=deltaTempo/60000; //Calcula quantos minutos se passaram
      long segundos=(deltaTempo/1000)%60; //Calcula quantos segundos se passaram
      long centesimos=(deltaTempo/10)%100; //Calcula quantos centesimos se passaram
      
      //deltaTempo=String(deltaTempo2); //converte long pra string e salva o tempo2 no tempo
   
      
      if(cavaloJaPassou){
        display.clearDisplay();
        display.setCursor(0, OLED_LINE1);
        display.print("Cronometro");
        display.setCursor(0, OLED_LINE2);
        if(minutos<10){
          display.print("0");
          minutos2=String( "0" + String(minutos));
        }
        else{
          minutos2=String(minutos);
          }
        display.print(minutos);
        display.print(":");
        if(segundos<10){
          display.print("0");
          segundos2=String("0"+ String(segundos));
        }
        else{
          segundos2=String(segundos);
          }
        display.print(segundos);
        display.print(":");
        display.print(centesimos);
        display.setCursor(0, OLED_LINE3);
        display.print("Acesse o cronometro:");
        display.setCursor(0, OLED_LINE4);
        display.print(WiFi.softAPIP());
        display.display();
        minutos2=String(minutos);
        segundos2=String(segundos);
        centesimos2=String(centesimos);
      }


}