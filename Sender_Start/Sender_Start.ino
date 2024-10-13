#include <Wire.h>
#include "SparkFun_VL53L1X.h" //Click here to get the library: http://librarymanager/All#SparkFun_VL53L1X
#include <LoRa.h>
#include <SPI.h>

/* Definicoes para comunicação com radio LoRa */
#define SCK_LORA           5
#define MISO_LORA          19
#define MOSI_LORA          27
#define RESET_PIN_LORA     14
#define SS_PIN_LORA        18
 
#define HIGH_GAIN_LORA     20  /* dBm */
#define BAND               915E6  /* 915MHz de frequencia */
 
/* Definicoes gerais */
#define DEBUG_SERIAL_BAUDRATE    115200
 
/* Variaveis globais */
long informacao_a_ser_enviada = 0;

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5          /* Time ESP32 will go to sleep (in seconds) */

/* Local prototypes */
bool init_comunicacao_lora(void);

bool init_comunicacao_lora(void)
{
    bool status_init = false;
    Serial.println("[LoRa Sender] Tentando iniciar comunicacao com o radio LoRa...");
    SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_PIN_LORA);
    LoRa.setPins(SS_PIN_LORA, RESET_PIN_LORA, LORA_DEFAULT_DIO0_PIN);
     
    if (!LoRa.begin(BAND)) 
    {
        Serial.println("[LoRa Sender] Comunicacao com o radio LoRa falhou. Nova tentativa em 1 segundo...");        
        delay(1000);
        status_init = false;
    }
    else
    {
        /* Configura o ganho do receptor LoRa para 20dBm, o maior ganho possível (visando maior alcance possível) */
        LoRa.setTxPower(HIGH_GAIN_LORA); 
        Serial.println("[LoRa Sender] Comunicacao com o radio LoRa ok");
        status_init = true;
    }
 
    return status_init;
}

SFEVL53L1X distanceSensor;

void setup(void)
{
  Wire.begin();
  Serial.begin(DEBUG_SERIAL_BAUDRATE);
  Serial.println("VL53L1X Qwiic Test");
  if (distanceSensor.begin() != 0) //Begin returns 0 on a good init
  {
    Serial.println("Sensor failed to begin. Please check wiring. Freezing...");
    while (1)
      ;
  }
  Serial.println("Sensor online!");
  while (!Serial);
 
  /* Tenta, até obter sucesso, comunicacao com o chip LoRa */
  while(init_comunicacao_lora() == false){
    Serial.println("Sem comunicação com o LoRa!");
  }
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  LoRa.beginPacket();//Esta linha inicia a transmissão de um pacote de dados pelo módulo LoRa. Ela sinaliza que o microcontrolador está prestes a enviar dados para outro dispositivo ou nó na rede LoRa.
  LoRa.print('A'); //Avisa que acordou
  LoRa.endPacket();//Esta linha encerra o pacote de dados LoRa, indicando que todos os dados foram transmitidos e o pacote está pronto para ser enviado para o receptor LoRa.
      
  int distance = 1000;
  while(distance>=1000){
    distanceSensor.startRanging(); // este comando inicia a medição de distância pelo sensor 
    while (!distanceSensor.checkForDataReady())//: Este trecho de código cria um loop que verifica se os dados do sensor estão prontos. Enquanto os dados não estiverem prontos, o programa aguardará por um milissegundo (por meio da funçã
    {
      delay(1);
    }
    distance = distanceSensor.getDistance(); //Este comando obtém a distância medida pelo sensor. Presumivelmente, ele retorna a distância em milímetros e a armazena na variável distance.
    distanceSensor.clearInterrupt();//Esta linha de código parece estar limpando alguma interrupção no sensor de distância. Pode ser necessária para garantir que o sensor esteja pronto para a próxima medição.
    distanceSensor.stopRanging();// Este comando para de medir a distância. Ele pode economizar energia ou recursos do sistema interrompendo a medição quando não for mais necessária.

    //Serial.print("Distância (mm): ");
    //Serial.println(distance);
    if (distance<1000)
    {
      Serial.println("Distância menor que 1 m");
      LoRa.beginPacket();//Esta linha inicia a transmissão de um pacote de dados pelo módulo LoRa. Ela sinaliza que o microcontrolador está prestes a enviar dados para outro dispositivo ou nó na rede LoRa.
      LoRa.print('S'); //indica que o cavalo passou
      LoRa.endPacket();//Esta linha encerra o pacote de dados LoRa, indicando que todos os dados foram transmitidos e o pacote está pronto para ser enviado para o receptor LoRa.
      LoRa.sleep();
    }
  }
  Serial.println("Going to sleep now");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop(void){
}