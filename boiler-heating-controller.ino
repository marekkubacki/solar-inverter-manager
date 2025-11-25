#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HardwareSerial.h>
#include <Arduino.h>

float temp = 0;
int aktualny_tryb=0;
int rozkaz_tryb=0;
int licznik=0;

// Struktura danych
typedef struct struct_tryb {
  uint8_t klucz;
  int trybik;
} struct_tryb;
typedef struct struct_temp {
  uint8_t klucz;
  float temp;
} struct_temp;

void setup() 
{
  Serial.begin(9600);
  delay(1000);
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(27, OUTPUT);
  digitalWrite(32, 1);
  digitalWrite(33, 1);
  digitalWrite(27, 1);
  pinMode(2,OUTPUT);
  digitalWrite(2, LOW);         // Początkowy stan LED: wyłączona

  WiFi.mode(WIFI_AP_STA);  // Tryb mieszany dla lepszego ESP-NOW
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);  // Long Range Mode
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR);  // Tryb AP też w LR
  esp_wifi_set_max_tx_power(78);  // Maksymalna moc nadawania
  esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_250K);  // Najwolniejsza prędkość
  
  // Ustawienie kanału Wi-Fi (musi być takie samo na obu ESP)
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
  
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Błąd inicjalizacji ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataReceived);
}

int maks_wartosc(int tryb, int maks)
{
  if(tryb>maks)
  {
    tryb=maks;
  }
  return tryb;
}
void uwzglednij_temp(int tryb)
{
  if(temp<44)
  {
    aktualny_tryb = maks_wartosc(tryb, 3);
  }
  if(temp>45 && temp<56)
  {
    aktualny_tryb = maks_wartosc(tryb, 2);
  }
  if(temp>57 && temp<63)
  {
    aktualny_tryb = maks_wartosc(tryb, 1);
  }
  if(temp>63)
  {
    aktualny_tryb = maks_wartosc(tryb, 0);
  }

  grzanie(aktualny_tryb);

  Serial.print("aktualny tryb :  ");
  Serial.println(aktualny_tryb);
}

void grzanie(int i)
{
  switch(i)
  {
    case 0:
      digitalWrite(32, 1);
      digitalWrite(33, 1);
      digitalWrite(27, 1);
    break;
    case 1:
      digitalWrite(32, 0);
      digitalWrite(33, 1);
      digitalWrite(27, 1);
    break;
    case 2:
      digitalWrite(32, 0);
      digitalWrite(33, 0);
      digitalWrite(27, 1);
    break;
    case 3:
      digitalWrite(32, 0);
      digitalWrite(33, 0);
      digitalWrite(27, 0);
    break;
  }
}
// Funkcja odbierająca dane
void OnDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) 
{
  uint8_t typ = incomingData[0];  // Pierwszy bajt to identyfikator

  if (typ == 0x02 && len == sizeof(struct_temp)) 
  {  
    struct_temp receivedTemp;
    memcpy(&receivedTemp, incomingData, sizeof(receivedTemp));
    temp = receivedTemp.temp;
    Serial.print("Odebrana temperatura: ");
    Serial.println(temp);
  }
  else if (typ == 0x01 && len == sizeof(struct_tryb)) 
  {  
    struct_tryb receivedTryb;
    memcpy(&receivedTryb, incomingData, sizeof(receivedTryb));
    rozkaz_tryb = receivedTryb.trybik;
    Serial.print("Odebrany tryb: ");
    Serial.println(rozkaz_tryb);
    uwzglednij_temp(rozkaz_tryb);
    licznik=0;
  
    for(int i=0; i<=rozkaz_tryb; i++) //blik przy odbiorze data
    {
      digitalWrite(2, HIGH);
      delay(180);
      digitalWrite(2, LOW);
      delay(170);
    }
  }

}

void loop() 
{
  licznik++;
  if(licznik > 500)  //blad polaczenia
  {
    aktualny_tryb=0;
    rozkaz_tryb=0;
    uwzglednij_temp(0);
    digitalWrite(2, HIGH);
  }
  delay(310);
  Serial.print(" .");
}
