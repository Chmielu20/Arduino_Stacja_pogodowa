// Filip Chmielowski
// Wrocław, 09.02.2023r.
// Stacja pogodowa

#include <OneWire.h> // Biblioteka odpowiadająca za komunikację protokołem 1-Wire czujników DS18B20
#include <DallasTemperature.h> // Biblioteka odpowiadająca za cyfrowe czujniki temperatury
#include "DHT.h"

#define PHOTORESISTOR_PIN A0
#define LM35_TOP_PIN A1
#define LM35_MIDDLE_PIN A2
#define LM35_BOTTOM_PIN A3
#define DS18B20_PIN A4
#define DHT11_PIN 2
#define BUTTON_PIN 3

OneWire oneWire(DS18B20_PIN); // Podłączenie cyfrowych czujników temperatury do pinu A4
DallasTemperature DS18B20_Sensors(&oneWire); // Przekazanie powyższej informacji do biblioteki

// Adresy posiadanych dwóch cyfrowych czujników temperatury DS18B20:
// -> Czujnik górny: 28 A0 24 12 0 0 0 9A
// -> Czujnik dolny: 28 AA  C 12 0 0 0 93
DeviceAddress DS18B20_Top_Sensor = {0x28, 0xA0, 0x24, 0x12, 0x0, 0x0, 0x0, 0x9A};
DeviceAddress DS18B20_Bottom_Sensor = {0x28, 0xAA, 0xC, 0x12, 0x0, 0x0, 0x0, 0x93};

DHT DHT11_Sensor; // Deklaracja czujnika wilgotności i temperatury DHT-11

double photoresistor_data = 0; // Zmienna przechowująca odczyt z fotorezystora
double photoresistor_data_to_volts = 0; // Zamiana danych na zakres 0-5 [V]

void setup()
{
  Serial.begin(9600); // Uruchomienie komunikacji przez UART

  DS18B20_Sensors.begin(); // Inicjalizacja cyfrowych czujników temperatury DS18B20

  DHT11_Sensor.setup(DHT11_PIN); // Informacja o komunikacji z czujnikiem wilgotności DHT-11

  pinMode(BUTTON_PIN, INPUT_PULLUP); // Przycisk jako wejście typu 'pull-up'

  // Deklaracja przerwania zewnętrznego na pinie 3, gdzie podłączono przycisk.
  // Na zmianę wartości ze stanu niskiego na wysoki, wywoływana jest funkcja
  // wyświetlająca dane pogodowe (nieprzyjmująca argumentów i niezwracająca niczego).
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), weather_forecast, RISING);
}

void loop()
{
  // ========== PHOTORESISTOR ==========
  photoresistor_data = analogRead(PHOTORESISTOR_PIN); // Odczytanie wartości fotorezystora
  photoresistor_data_to_volts = photoresistor_data * 5.0 / 1024.0; // Zamiana danych na napięcie

  // ========== LM35 SENSORS ==========
  // Przeliczenie odczytu ADC analogowych czujników LM35 na temperaturę
  float LM35_Top_Temperature = ((analogRead(LM35_TOP_PIN) * 5.0) / 1024.0) * 100;
  float LM35_Middle_Temperature = ((analogRead(LM35_MIDDLE_PIN) * 5.0) / 1024.0) * 100;
  float LM35_Bottom_Temperature = ((analogRead(LM35_BOTTOM_PIN) * 5.0) / 1024.0) * 100;

  // ========== DHT-11 SENSOR ==========
  int DHT11_Humidity = DHT11_Sensor.getHumidity(); // Pobranie danych o wilgotności [%RH]

  int DHT11_Temperature_C = DHT11_Sensor.getTemperature(); // Pobranie danych o temperaturze [*C]
  
  // Przeliczenie temperatury ze skali Celsjusza na skalę Fahrenheita
  int DHT11_Temperature_F = DHT11_Sensor.toFahrenheit(DHT11_Temperature_C);

  // ========== DS18B20 SENSORS ==========
  DS18B20_Sensors.requestTemperatures(); // Pobranie temperatury z cyfrowych czujników DS18B20

  float DS18B20_Top_Temperature = DS18B20_Sensors.getTempC(DS18B20_Top_Sensor);
  float DS18B20_Bottom_Temperature = DS18B20_Sensors.getTempC(DS18B20_Bottom_Sensor);
  
  // ========== AVERAGE TEMPERATURES ==========
  // Uśrednienie temperatur zebranych z analogowych czujników temperatury LM35
  float LM35_Average_Temperature = (LM35_Top_Temperature + LM35_Middle_Temperature + LM35_Bottom_Temperature) / 3.0;
  
  // Uśrednienie temperatur zebranych z cyfrowych czujników temperatury DS18B20 oraz DHT-11
  float DS18B20_DHT11_Average_Temperature = (DHT11_Temperature_C + DS18B20_Top_Temperature + DS18B20_Bottom_Temperature) / 3.0;
  
  // Uśrednienie temperatur z czujników analogowych oraz czujników cyfrowych
  float average_temperature_C = (LM35_Average_Temperature + DS18B20_DHT11_Average_Temperature) / 2.0;

  // Zamiana uśrednionej wartości wszystkich temperatur ze skali Celsjusza na skalę Fahrenheita
  float average_temperature_F = ((average_temperature_C * 9.0) / 5.0) + 32.0;

  // ========== DATA PRINTING ==========
  // Jeżeli czujnik wilgotności DHT-11 działa poprawnie
  if(DHT11_Sensor.getStatusString() == "OK")
  {
    Serial.println("WEATHER FORECAST FROM THE WEATHER STATION");
    
    Serial.print("-> TEMPERATURE: ");
    Serial.print(average_temperature_C);
    Serial.print(" [*C] | ");
    Serial.print(average_temperature_F);
    Serial.println(" [F]");

    Serial.print("-> HUMIDITY:    ");
    Serial.print(DHT11_Humidity);
    Serial.println(" [%RH]");
    
    Serial.print("-> CLOUDINESS:  ");
    if(photoresistor_data_to_volts >= 0 && photoresistor_data_to_volts < 1)
    {
      Serial.println("Cloudy (5/5)");
    }
    else if(photoresistor_data_to_volts >= 1 && photoresistor_data_to_volts < 2)
    {
      Serial.println("Partly cloudy (4/5)");
    }
    else if(photoresistor_data_to_volts >= 2 && photoresistor_data_to_volts < 3)
    {
      Serial.println("Slightly cloudy (3/5)");
    }
    else if(photoresistor_data_to_volts >= 3 && photoresistor_data_to_volts < 4)
    {
      Serial.println("Scattered clouds (2/5)");
    }
    else if(photoresistor_data_to_volts >= 4 && photoresistor_data_to_volts <= 5)
    {
      Serial.println("Sunny (1/5)");
    }
    else
    {
      Serial.println("What the f**k is going on with this crazy weather?!");
    }

    Serial.print("\n");
  }

  // ========== APPROPRIATE DELAY ==========
  // Odczekanie wymaganego przez czujnik wilgotności DHT-11 czasu
  delay(DHT11_Sensor.getMinimumSamplingPeriod());
}

// Funkcja wyświetlająca informacje o autorze programu oraz o samym programie
void weather_forecast()
{
  Serial.println("Author:  Filip Chmielowski");
  Serial.println("Written: Wrocław, 09.02.2023r.");
  Serial.println("Project: Weather station");
  Serial.print("\n");

  delay(10); // Opóźnienie eliminujące drgania styków przycisku
}

/*
STACJA POGODOWA - Opis działania programu
* Program monitoruje / mierzy temperaturę, wilgotność oraz zachmurzenie w danym pomieszczeniu.
* Program obsługuje następujące czujniki:
  -> 1x fotorezystor GL5537-1;
  -> 3x analogowy czujnik temperatury LM35;
  -> 2x cyfrowy czujnik temperatury DS18B20;
  -> 1x cyfrowy czujnik wilgotności i temperatury DHT-11.
* Program komunikuje się z użytkownikiem poprzez monitor portu szeregowego (komunikacja UART).
* Dane aktualizowane oraz wyświetlane są na komputerze co sekundę.
* Dodatkiem jest przycisk, działający na przerwaniu, po którego naciśnięciu następuje
*  wyświetlenie informacji o autorze programu oraz o samym programie.
*/
