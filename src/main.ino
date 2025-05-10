#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <stdio.h>

#define WATER_LEVEL_THRESHOLD  10
#define TEMPERATURE_THRESHOLD  30

#define DHTPIN   7
#define DHTTYPE  DHT11

RTC_DS1307 rtc;
DHT dht(DHTPIN, DHTTYPE);

void SerialSetup() {
  unsigned int ubrr = 103;
  UBRR0H = (ubrr >> 8);
  UBRR0L = ubrr;
  UCSR0B = (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}
void sendChar(char c) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = c;
}
void sendStr(const char* s) { while (*s) sendChar(*s++); }
void sendNum(uint8_t num) {
  sendChar('0' + num / 10);
  sendChar('0' + num % 10);
}
void oneSecDelay() {
  TCCR1A = 0; TCCR1B = 0; TCNT1 = 0;
  TCCR1B |= (1 << CS12) | (1 << CS10);
  while (TCNT1 < 15625);
  TCCR1B = 0;
}

// ADC 
volatile unsigned char *my_ADMUX    = (unsigned char*)0x7C;
volatile unsigned char *my_ADCSRB   = (unsigned char*)0x7B;
volatile unsigned char *my_ADCSRA   = (unsigned char*)0x7A;
volatile unsigned int  *my_ADC_DATA = (unsigned int* )0x78;

void adc_init() {
  *my_ADCSRA = 0;
  *my_ADCSRA |= (1<<7); *my_ADCSRA |= 0x07;
  *my_ADCSRB &= ~0x08; *my_ADCSRB &= ~0x07;
  *my_ADMUX &= ~0x80; *my_ADMUX |= 0x40; *my_ADMUX &= ~0x20; *my_ADMUX &= 0xE0;
}
unsigned int adc_read(uint8_t ch) {
  *my_ADMUX = (*my_ADMUX & 0xE0) | (ch & 0x1F);
  *my_ADCSRA |= (1<<6);
  while (*my_ADCSRA & (1<<6));
  return *my_ADC_DATA & 0x03FF;
}

//  LED 
volatile unsigned char* ddr_c  = (unsigned char*)0x27;
volatile unsigned char* port_c = (unsigned char*)0x28;
volatile unsigned char* ddr_b  = (unsigned char*)0x24;
volatile unsigned char* port_b = (unsigned char*)0x25;
volatile unsigned char* pin_b  = (unsigned char*)0x23;

#define LED_YELLOW (1<<5)  // PC5 = D32
#define LED_GREEN  (1<<3)  // PC3 = D34
#define LED_RED    (1<<0)  // PC0 = D37
#define LED_BLUE   (1<<1)  // PC1 = D36
#define LED_ALL    (LED_YELLOW|LED_GREEN|LED_RED|LED_BLUE)
#define SYS_BTN    (1<<7)  // PB7 = D13

enum State { DISABLED=0, IDLE=1, ERROR=2, RUNNING=3 };
State state = DISABLED;
volatile unsigned long lastBtnTime = 0;  // for ISR debounce

#define Clock_BTN_BIT        7   // PL7 = D42
#define CounterClock_BTN_BIT 5   // PL5 = D44
#define DDR_Clock        DDRL
#define PORT_Clock       PORTL
#define PIN_Clock        PINL
#define DDR_CounterClock DDRL
#define PORT_CounterClock PORTL
#define PIN_CounterClock  PINL

Stepper VentMotor(2048, 22, 26, 24, 28);
unsigned long btnT1=0, btnT2=0;
const unsigned long Debounce=300;
int cnt1=0, cnt2=0, dir=3;

void VentSetup() {
  VentMotor.setSpeed(10); VentMotor.step(0);
  DDR_Clock &= ~(1<<Clock_BTN_BIT);      PORT_Clock |= (1<<Clock_BTN_BIT);
  DDR_CounterClock &= ~(1<<CounterClock_BTN_BIT); PORT_CounterClock |= (1<<CounterClock_BTN_BIT);
}
void VentLoop() {
  unsigned long now = millis();
  bool bCCW = !(PIN_CounterClock & (1<<CounterClock_BTN_BIT));
  bool bCW  = !(PIN_Clock        & (1<<Clock_BTN_BIT));
  if (bCCW && now - btnT1 > Debounce) { btnT1=now; cnt1++; if(cnt1!=2){cnt2=0;dir=1;}else{cnt1=0;dir=3;} }
  if (bCW  && now - btnT2 > Debounce) { btnT2=now; cnt2++; if(cnt2!=2){cnt1=0;dir=2;}else{cnt2=0;dir=3;} }
  if (dir==1) VentMotor.step(-1);
  else if (dir==2) VentMotor.step(1);
}

// ────── DHT + LCD 显示 ──────
LiquidCrystal lcd(11,12,2,3,4,5);
unsigned long lastDht = 0;

void DhtLcdSetup() {
  lcd.begin(16,2);
  dht.begin();
}
void DhtLcdLoop() {
  if (millis() - lastDht < 2000) return;
  lastDht = millis();
  float T = dht.readTemperature();
  float H = dht.readHumidity();
  lcd.clear();
  if (isnan(T)||isnan(H)) {
    lcd.print("Sensor err");
  } else {
    float f = T*9.0/5.0 + 32.0;
    lcd.setCursor(0,0);
    lcd.print("T:"); lcd.print(f,1); lcd.write(223); lcd.print("F");
    lcd.setCursor(0,1);
    lcd.print("H:"); lcd.print(H,1); lcd.print("%");
  }
}

// ────── RTC + fan ──────
void FanRtcSetup() {
  SerialSetup();
  Wire.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
void FanRtcLoop() {
  DateTime now = rtc.now();
  float temp = dht.readTemperature();
  if (temp > TEMPERATURE_THRESHOLD) {
    PORTH |=  (1<<6);
    PORTH &= ~(1<<5);
    PORTB |=  (1<<4);
    sendStr("Date: ");
    sendNum(now.month()); sendChar('/'); sendNum(now.day()); sendChar('/');
    sendNum(now.year()%100); sendChar(' ');
    sendStr("Time: ");
    sendNum(now.hour()); sendChar(':'); sendNum(now.minute()); sendChar(':'); sendNum(now.second()); sendChar('\n');
    oneSecDelay();
  } else {
    PORTB &= ~(1<<4);
  }
}

// ────── Water Level & LCD  ──────
bool inErrorDisplayed = false;
void WaterLevelLoop() {
  unsigned int lvl = adc_read(0);
  if (lvl < WATER_LEVEL_THRESHOLD) {
    if (!inErrorDisplayed) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Low Water!");
      inErrorDisplayed = true;
    }
  }
}

// ────── ISR: D13 ──────
ISR(PCINT0_vect) {
  if (!(PINB & SYS_BTN)) {
    unsigned long now = millis();
    if (now - lastBtnTime > 50) {
      lastBtnTime = now;
      state = (state == DISABLED ? IDLE : DISABLED);
      inErrorDisplayed = false; 
    }
  }
}

// 
void setup() {
  *ddr_c |= LED_ALL;   *port_c &= ~LED_ALL;
  *ddr_b &= ~SYS_BTN;  *port_b |= SYS_BTN;
  DDRH |= (1<<5)|(1<<6);  
  DDRB |= (1<<4);         // EN1

  PCICR |= (1 << PCIE0);     //  Port B 
  PCMSK0 |= (1 << PCINT7);   //D13
  sei();                     // 

  adc_init();
  VentSetup();
  DhtLcdSetup();
  FanRtcSetup();
}

// ────── main loop ──────
void loop() {
  // LED state
  uint8_t leds = 0;
  switch (state) {
    case DISABLED: leds = LED_YELLOW; break;
    case IDLE:     leds = LED_GREEN;  break;
    case ERROR:    leds = LED_RED;    break;
    case RUNNING:  leds = LED_BLUE;   break;
  }
  *port_c = (*port_c & ~LED_ALL) | leds;

  // states
  if (state != RUNNING) {
    PORTH &= ~((1<<5)|(1<<6));
    PORTB &= ~(1<<4);
  }

  if (state != DISABLED) VentLoop();

  switch (state) {
    case DISABLED:
      break;
    case IDLE:
      if (inErrorDisplayed) {
        lcd.clear();
        inErrorDisplayed = false;
      }
      WaterLevelLoop();
      DhtLcdLoop();
      if (adc_read(0) < WATER_LEVEL_THRESHOLD) state = ERROR;
      else if (dht.readTemperature() > TEMPERATURE_THRESHOLD) state = RUNNING;
      break;
    case ERROR:
      WaterLevelLoop();
      if (adc_read(0) >= WATER_LEVEL_THRESHOLD) {
        lcd.clear();
        inErrorDisplayed = false;
        state = IDLE;
      }
      break;
    case RUNNING:
      FanRtcLoop();
      if (adc_read(0) < WATER_LEVEL_THRESHOLD) state = ERROR;
      else if (dht.readTemperature() <= TEMPERATURE_THRESHOLD) state = IDLE;
      break;
  }

  delayMicroseconds(5000);
}
