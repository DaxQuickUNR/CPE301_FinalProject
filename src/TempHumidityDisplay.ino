#include <LiquidCrystal.h>   
#include <DHT.h>            

#define DHTPIN 7             // DHT11 data pin is connected to digital pin 7
#define DHTTYPE DHT11        

DHT dht(DHTPIN, DHTTYPE);    

// LCD pins: RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5
LiquidCrystal lcd(11, 12, 2, 3, 4, 5);

unsigned long previousMillis = 0;
const unsigned long interval = 60000;  // update every 60 seconds

void setup() {
  lcd.begin(16, 2);     
  dht.begin();          
}

void loop() {
  unsigned long now = millis();
  if (now - previousMillis >= interval) {
    previousMillis = now;

    // Read values from DHT11
    float celsius = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check for errors
    if (isnan(celsius) || isnan(humidity)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sensor error");
      return;
    }

    float fahrenheit = celsius * 9.0 / 5.0 + 32.0;

    // Clear previous LCD content
    lcd.clear();

    // Display temperature
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(fahrenheit, 1);  
    lcd.write(223);           
    lcd.print("F");

    // Display humidity
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(humidity, 1);
    lcd.print("%");
  }
}
