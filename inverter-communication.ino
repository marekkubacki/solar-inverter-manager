#include <Arduino.h>
#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <math.h>

// Ustawienia UART
#define RX_PIN 16  // Pin RX na ESP32
#define TX_PIN 17  // Pin TX na ESP32
#define RX2_PIN 19 // RX dla drugiego portu
#define TX2_PIN 21  // TX dla drugiego portuc:\Users\AMD Ryzen PC\Documents\Arduino\modbus_test\modbus_test.ino
#define BAUD_RATE 9600  // Szybkość komunikacji Modbus RTU

// Adres urządzenia Modbus (falownik)
#define INVERTER_ID 1

// Inicjalizacja ModbusMaster
ModbusMaster node;
HardwareSerial modbusSerial(1);
HardwareSerial dataloger(2);

// Definicje czasów
const unsigned long longPressTime = 1500;  // Czas długiego naciśnięcia
const unsigned long debounceDelay = 50;   // Opóźnienie dla eliminacji drgań
const unsigned long multiClickTime = 800; // Czas na kolejne kliknięcia

// Zmienne dla obsługi przycisku
volatile int clickCount = 0;              // Licznik kliknięć
volatile bool manual = false; // Flaga dla długiego naciśnięcia
volatile bool isButtonPressed = false;   // Stan przycisku
//przesyl esp now
uint8_t receiverMACAddress[] = {0xA4, 0xCF, 0x12, 0x42, 0xB0, 0x0C};

typedef struct struct_message {
    uint8_t klucz;
    int trybik;
} struct_message;

struct_message myData;

int tryb=0, tryb1=0;
int flaga=0;
void buttonTask(void *param) 
{
  unsigned long lastDebounceTime = 0;
  unsigned long buttonPressStart = 0;
  unsigned long lastClickTime = 0;
  bool buttonState = HIGH;
  bool lastButtonState = HIGH;

  while (true) 
  {
    // Odczyt stanu przycisku
    bool reading = digitalRead(0);

    // Eliminacja drgań styków
    if (reading != lastButtonState) 
    {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) 
    {
      
      if (reading != buttonState) 
      {
        buttonState = reading;

        if (buttonState == LOW) 
        {
          // Przycisk wciśnięty
          buttonPressStart = millis();
        } 
        else 
        {
          // Przycisk puszczony
          unsigned long pressDuration = millis() - buttonPressStart;

          if (pressDuration >= longPressTime) 
          {
            tryb=0;
            manual = true;
            clickCount = 0; // Reset kliknięć po długim naciśnięciu
          } 
          else 
          {
            manual = true;
            // Krótkie naciśnięcie
            if ((millis() - lastClickTime) > multiClickTime) 
            {
              clickCount = 0; // Reset licznika, jeśli czas na kolejne kliknięcie minął
            }

            clickCount++;
            lastClickTime = millis();
          }
        }
      }
    }

    lastButtonState = reading;

    // Dodanie małego opóźnienia, aby wątek nie zajmował zbyt dużo zasobów
    vTaskDelay(15 / portTICK_PERIOD_MS);
  }
}
void setup() {
  // Inicjalizacja portu szeregowego do debugowania
  Serial.begin(115200);
  
  // Inicjalizacja portu szeregowego dla Modbus RTU
  modbusSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  dataloger.begin(BAUD_RATE, SERIAL_8N1, RX2_PIN, TX2_PIN); 
  // Podłączenie ModbusMaster do portu szeregowego
  node.begin(INVERTER_ID, modbusSerial);
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

  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  digitalWrite(32, 0);
  digitalWrite(33, 0);
  
  pinMode(2,OUTPUT);
  digitalWrite(2, LOW);         // Początkowy stan LED: wyłączona
  pinMode(0,INPUT_PULLUP); //przycisk
  delay(3000);

  xTaskCreatePinnedToCore(
        posrednik,        // Funkcja wątku
        "posrednik",    // Nazwa wątku
        2048,            // Rozmiar stosu
        NULL,            // Parametr
        1,               // Priorytet
        NULL,            // Uchwyt wątku
        1                // Rdzeń CPU
    );
  // Tworzenie wątku migania
  xTaskCreate(
    blinkTask,           // Funkcja wątku
    "Blink Task",        // Nazwa wątku (dla debugowania)
    1024,                // Rozmiar stosu (1024 bajty)
    NULL,                // Parametr przekazywany do wątku (brak)
    3,                   // Priorytet wątku
    NULL                // Uchwyt wątku (niepotrzebny)
  );

  // Tworzenie wątku obsługi przycisku
  xTaskCreatePinnedToCore(
    buttonTask,          // Funkcja wątku
    "Button Task",       // Nazwa wątku
    2048,                // Rozmiar stosu
    NULL,                // Parametry
    2,                   // Priorytet
    NULL,                // Uchwyt wątku
    1                   // Rdzeń procesora (1 lub 0)
  );
}
void grzanie (int i, int paczka)
{
  myData.klucz = 0x01;
  myData.trybik=paczka;
  esp_now_send(receiverMACAddress, (uint8_t *)&myData, sizeof(myData));
  switch(i)
  {
    case 0:
      digitalWrite(32, 0);
      digitalWrite(33, 0);
    break;
    case 1:
      digitalWrite(32, 1);
      digitalWrite(33, 0);
    break;
    case 2:
      digitalWrite(32, 0);
      digitalWrite(33, 1);
    break;
    case 3:
      digitalWrite(32, 1);
      digitalWrite(33, 1);
    break;
  }
}
int normowanie (int liczba)
{
  if(liczba>50000)
  {
    return 0;
  }
  else
  {
    return liczba;
  }
}

int outputpow=0, batteryV=0, pvpower=0, nadmiar=0, batbool=0;
bool bladpolaczenia=false;

// Funkcja wątku migania
void blinkTask(void *param) 
{
  while (true) 
  {
    if(bladpolaczenia==false || manual==true)
    {
      for (int count = 0; count < tryb; count++) 
      {
        if(manual==false)
        {
          digitalWrite(2, HIGH); // Włącz diodę
          vTaskDelay(300 / portTICK_PERIOD_MS); // Czekaj 300 ms
          digitalWrite(2, LOW);  // Wyłącz diodę
          vTaskDelay(300 / portTICK_PERIOD_MS); // Czekaj 300 ms
        }
        else
        {
          digitalWrite(2, LOW); 
          vTaskDelay(300 / portTICK_PERIOD_MS); // Czekaj 300 ms
          digitalWrite(2, HIGH);  
          vTaskDelay(300 / portTICK_PERIOD_MS); // Czekaj 300 ms   
        }
      }
      if(manual==true && tryb==0)
      {
        digitalWrite(2, HIGH);
      }
      vTaskDelay(2000 / portTICK_PERIOD_MS); // Przerwa 2 sekundy
    }
    else if(bladpolaczenia==true)
    {
      digitalWrite(2, HIGH);
      vTaskDelay(150 / portTICK_PERIOD_MS); // Czekaj 300 ms
      digitalWrite(2, LOW);
      vTaskDelay(150 / portTICK_PERIOD_MS); // Czekaj 300 ms
    }
  }
}
void posrednik(void *param) 
{
    while (true) 
    {
        if (dataloger.available()) 
        {
            flaga=1;
            uint8_t data = dataloger.read();
            Serial.print(" dataloger: ");
            Serial.write(data);  // Debugowanie
            modbusSerial.write(data);  // Przekazanie do TX2
        }
        if (modbusSerial.available() && flaga==1) 
        {
            uint8_t data = modbusSerial.read();
            Serial.print(" falownik: ");
            Serial.write(data);  // Debugowanie
            dataloger.write(data);  // Przekazanie do TX0
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Minimalne opóźnienie
    }
}
void bladpol() //błąd połączenia
{
  bladpolaczenia=true;
  vTaskDelay(700 / portTICK_PERIOD_MS);
  modbusSerial.end();  // Zamknij port szeregowy
  modbusSerial.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);  // Zainicjuj go ponownie
  node.begin(INVERTER_ID, modbusSerial);  // Ponowne połączenie Modbus
  vTaskDelay(700 / portTICK_PERIOD_MS);  // Krótkie opóźnienie
}
int pobierzdane(int rejestr)
{
  vTaskDelay(180 / portTICK_PERIOD_MS);
  while (flaga==1) 
  {
    vTaskDelay(8000 / portTICK_PERIOD_MS);
    flaga=0;
  }
  uint8_t result;
  int wynik=0;

  result = node.readHoldingRegisters(rejestr, 1);
  if (result == node.ku8MBSuccess) 
  {
    wynik = node.getResponseBuffer(0);  
    bladpolaczenia=false;
  } 
  else 
  {
    bladpol();
  }
  vTaskDelay(310 / portTICK_PERIOD_MS);
  return wynik;
}
int zwieksz(int co, int maks=3)
{
  if(co < maks)
  {
    co++;
  }
  return co;
}
int zmniejsz(int co, int mini=0)
{
  if(co > mini)
  {
    co--;
  }
  return co;
}
void tryby_zmiana()
{
  if(tryb1==3 && nadmiar>2025 && batteryV>=530)
  {
    tryb=zwieksz(tryb);
  }
  if(nadmiar>780 && batteryV >= 530)
  {
    tryb1=zwieksz(tryb1);
  }
  if(nadmiar>1800 && batteryV < 530)
  {
    tryb1=zwieksz(tryb1);
  }

  if(batteryV < 520)
  {
    tryb=0;
  }

  if(tryb==0 && nadmiar < 30 && batteryV>=520)
  {
    tryb1=zmniejsz(tryb1);
  }
  if(tryb==0 && nadmiar < 500 && batteryV<520)
  {
    tryb1=zmniejsz(tryb1);
  }
  if(tryb==0 && nadmiar < -740)
  {
    //tryb1=zmniejsz(tryb1);
    batbool=0;
  }

  if(nadmiar<2)
  {
    tryb=zmniejsz(tryb);
  }
  if(nadmiar < -2030)
  {
    //tryb=zmniejsz(tryb);
    batbool=0;
  }
}
void bateria()
{
  if((batteryV>578 || batbool==1) && (tryb1==3 || batteryV>580))
  {
    batbool=1;
    if(tryb1==3)
    {
      tryb=zwieksz(tryb);
    }
    else
    {
      tryb1=zwieksz(tryb1);
    }
  }
  if(batteryV<550)
  {
    batbool=0;
  }
  if(tryb==0 && batteryV<566)
  {
    batbool=0;
  }
  if(tryb==0)
  {
    if(nadmiar>-620 && batteryV<567)
    {
      batbool=0;
    }
    if(nadmiar>-500 && batteryV<568)
    {
      batbool=0;
    }
    if(nadmiar>-250 && batteryV<570)
    {
      batbool=0;
    }
  }
  
}
void loop() 
{
  outputpow=pobierzdane(254);
  batteryV=pobierzdane(277);
  pvpower=pobierzdane(302);
  
  pvpower=normowanie(pvpower);
  outputpow=normowanie(outputpow);
  if(pvpower==0)
  {
    tryb1=zmniejsz(tryb1);
  }
  nadmiar=pvpower-outputpow;
  tryby_zmiana();
  bateria();
  if(manual==true)
  {
    tryb=clickCount;
  }
  grzanie(tryb,tryb1);

  Serial.print("outputpow: ");
  Serial.print(outputpow); 

  Serial.println(" W");

  Serial.print("batteryV: ");
  Serial.print(batteryV); 
  Serial.println(" V");

  Serial.print("pvpower: ");
  Serial.print(pvpower);
  Serial.println(" W");

  Serial.print("nadmiar: ");
  Serial.print(nadmiar);
  Serial.println(" W");

  Serial.print("tryb: ");
  Serial.println(tryb);

    Serial.print("tryb1: ");
  Serial.println(tryb1);

  Serial.print("batbool: ");
  Serial.println(batbool);

  vTaskDelay(3700 / portTICK_PERIOD_MS);
}