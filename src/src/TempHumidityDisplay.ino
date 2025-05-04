#include <LiquidCrystal.h>   // LCD library
#include <dht.h>             // DHT11 sensor library

#define DHTPIN 7             // DHT11 data pin is wired to digital pin 7
dht DHT;                    // create a DHT object

// LCD pins: RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5
LiquidCrystal lcd(11, 12, 2, 3, 4, 5);

unsigned long previousMillis = 0;
const unsigned long interval = 2000;  // update every 60 seconds

void setup() {
  lcd.begin(16, 2);        // initialize the 16×2 LCD
}

void loop() {
  unsigned long now = millis();
  // check if it's time to refresh
  if (now - previousMillis >= interval) {
    previousMillis = now;

    // trigger a new DHT read
    DHT.read11(DHTPIN);
    int celsius    = DHT.temperature;
    int humidity   = DHT.humidity;

    // convert to Fahrenheit
    float fahrenheit = celsius * 9.0 / 5.0 + 32.0;

    // clear old data
    lcd.clear();

    // display temperature in Fahrenheit
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    if (celsius < -40 || celsius > 80) {
      lcd.print("Err");    // out of DHT11 range
    } else {
      lcd.print(fahrenheit, 1);  // one decimal place
      lcd.write(223);            // degree symbol
      lcd.print("F");
    }

    // display humidity
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    if (humidity < 0 || humidity > 100) {
      lcd.print("Err");
    } else {
      lcd.print(humidity);
      lcd.print("%");
    }
  }

}
