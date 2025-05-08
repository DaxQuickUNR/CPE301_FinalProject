#include <RTClib.h>
#include <Wire.h>
#include <DHT.h>

#define DHTPIN 7
#define DHTTYPE DHT11

RTC_DS1307 rtc;
DHT dht(DHTPIN, DHTTYPE);

void oneSecDelay() {
  // Stop Timer1
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Set prescaler to 1024: CS12 = 1, CS10 = 1
  TCCR1B |= (1 << CS12) | (1 << CS10);

  // Wait for 1 second worth of ticks: 16MHz / 1024 = 15625
  while (TCNT1 < 15625);

  // Stop timer
  TCCR1B = 0;

}

void setup() {
  unsigned int ubrr = 103;
  UBRR0H = (ubrr >> 8);
  UBRR0L = ubrr;
  UCSR0B = (1 << TXEN0);                          
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); 

  Wire.begin();
  dht.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

}

void sendChar(char c) {
  while (!(UCSR0A & (1 << UDRE0))); 
  UDR0 = c;
}

void sendStr(const char* s) {
  while (*s) {
    sendChar(*s++);
  }
}

void sendNum(uint8_t num) {
  sendChar('0' + num / 10);
  sendChar('0' + num % 10);
}



void loop() {
  // put your main code here, to run repeatedly:
  DateTime now = rtc.now();
  float temp = dht.readTemperature();
  if (temp > 30.0) {
    uint8_t m = now.month();
    uint8_t d = now.day();
    uint8_t y = now.year() % 100;
    uint8_t h = now.hour();
    uint8_t min = now.minute();
    uint8_t s = now.second();

    sendStr("Date: ");
    sendNum(m);
    sendChar('/');
    sendNum(d);
    sendChar('/');
    sendNum(y);  
    sendChar(' ');

    sendStr("Time: ");
    sendNum(h);
    sendChar(':');
    sendNum(min);
    sendChar(':');
    sendNum(s);
    sendChar('\n');
    oneSecDelay();
 }
}
