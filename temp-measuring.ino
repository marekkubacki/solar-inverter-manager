#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 25
#define DHTTYPE DHT22 // Typ czujnika
DHT dht(DHTPIN, DHTTYPE);

uint8_t receiverMACAddress[] = {0xA4, 0xCF, 0x12, 0x42, 0xB0, 0x0C};

typedef struct struct_message {
    uint8_t klucz;
    float temp;
} struct_message;

struct_message myData;

void setup()
{
  Serial.begin(115200);
  dht.begin();
  pinMode(2,OUTPUT);
  // ESP now przesyl
    WiFi.mode(WIFI_AP_STA);  // Tryb mieszany dla ESP-NOW
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);  // Long Range Mode
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR);  // Tryb AP też w LR
    esp_wifi_set_max_tx_power(78);  // Maksymalna moc nadawania
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_250K);  // Najwolniejsza prędkość

    // Ustawienie kanału Wi-Fi (musi być takie samo na obu ESP)
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Błąd inicjalizacji ESP-NOW");
        return;
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMACAddress, 6);
    peerInfo.channel = 6;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) 
    {
        Serial.println("Błąd dodawania odbiorcy");
        return;
    } 
  //^^^ esp now

}

void loop() 
{
  myData.klucz = 0x02;
  float temperatura = dht.readTemperature();
  if (isnan(temperatura)) 
  {
    Serial.println("Błąd odczytu temperatury!");
    digitalWrite(2, HIGH);
    delay(200);
    digitalWrite(2, LOW);
    delay(200);
  } 
  else 
  {
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println(" °C");

    myData.temp = temperatura;
    esp_now_send(receiverMACAddress, (uint8_t *)&myData, sizeof(myData));

    digitalWrite(2, HIGH);
    delay(2000);
    digitalWrite(2, LOW);
    delay(1000);
  }
  delay(10000);

}
