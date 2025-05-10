// CPE 301 Final Project
// Code developed by: Dax Quick, Autumn Heckler, Tracey Msangi, Ziyue Liang

//Import allowed libraries.
#include <LiquidCrystal.h>
#include <DHT.h>    
#include <RTClib.h>
#include <Wire.h>
#include <Stepper.h>

// DHT11 (Temp/Humidity Sensor) data pin is connected to digital pin 7 
#define DHTPIN 7 
#define DHTTYPE DHT11  

// Water Reservoir Monitoring Definitions
#define RDA 0x80
#define TBE 0x20  

// Vent Control Definitions
#define Clock_BTN_BIT 7          
#define CounterClock_BTN_BIT 5    
#define DDR_Clock DDRL
#define PORT_Clock PORTL
#define PIN_Clock PINL
#define DDR_CounterClock DDRL   
#define PORT_CounterClock PORTL
#define PIN_CounterClock PINL

// Water Monitoring References
volatile unsigned char* my_ADMUX  = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int*  my_ADC_DATA = (unsigned int*) 0x78;

//Water monitoring state change variable.
bool wasEmpty = false;

// Vent Control button variable initializations
unsigned long Button1 = 0;
unsigned long Button2 = 0;
const unsigned long Debounce = 300;
int Cnt1 = 0;
int Cnt2 = 0;
int Direction = 3;

//Stepper motor initialization
Stepper VentMotor(2048, 22, 26, 24, 28);

RTC_DS1307 rtc;
DHT dht(DHTPIN, DHTTYPE);    
// LCD pins: RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5
LiquidCrystal lcd(11, 12, 2, 3, 4, 5);



void setup() {

  //This block is associated with water reservoir monitoring functionality. 
  // setup the ADC
  adc_init();  
  
  //This block is associated with date + time functionality.
  unsigned int ubrr = 103;
  UBRR0H = (ubrr >> 8);
  UBRR0L = ubrr;
  UCSR0B = (1 << TXEN0);                          
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); 

  //This block is associated with DC Fan Motor functionality.
  // Set D8 (PH5), D9 (PH6) as output
  DDRH |= 0b01100000;
  // Set D10 (PB4) as output
  DDRB |= 0b00010000;
  PORTH &= 0b10011111;  // Clear PH5, PH6
  PORTB &= 0b11101111;  // Clear PB4

  //This block is associated with Vent Functionality.
  VentMotor.setSpeed(10);
  VentMotor.step(0);
  DDR_Clock &= ~(1 << Clock_BTN_BIT);
  PORT_Clock |= (1 << Clock_BTN_BIT);
  DDR_CounterClock &= ~(1 << CounterClock_BTN_BIT);
  PORT_CounterClock |= (1 << CounterClock_BTN_BIT);

  //Initialize Various Devices.
  lcd.begin(16, 2);     
  dht.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));          
}


//Create variable for previous millisecond value.
unsigned long previousMillis = 0;



//BEGIN Date + Time Support Functions
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



//BEGIN Water Sensor Support Functions
void adc_init() 
{
  // setup the A register
  *my_ADCSRA = 0;
  *my_ADCSRA |= 0x80;  // set bit 7 to 1 to enable the ADC
  *my_ADCSRA &= ~0x40; // clear bit 6 to disable ADC trigger
  *my_ADCSRA &= ~0x08; // clear bit 3 to disable interrupt
  *my_ADCSRA |= 0x03;  // set prescaler bits (slow reading)

  // setup the B register
  *my_ADCSRB &= ~0x08; // clear bit 3 to reset MUX5
  *my_ADCSRB &= ~0x07; // free running mode (clear bits 2, 1, 0)

  // setup the MUX Register
  *my_ADMUX &= ~0x80;  // clear bit 7 for AVCC ref
  *my_ADMUX |= 0x40;   // set bit 6 for AVCC ref
  *my_ADMUX &= ~0x20;  // clear bit 5 for right adjust result
  *my_ADMUX &= 0xE0;   // clear MUX 4:0 (keep only bits 7, 6)
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  // Clear channel selection bits (MUX 4:0)
  *my_ADMUX &= 0xE0;
  // clear the channel selection bits (MUX 5) (it is in ADCSRB)
  *my_ADCSRB &= ~0x08;
  // set the channel selection bits for channel 0
  *my_ADMUX |= (adc_channel_num & 0x1F);
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  
  // Wait for conversion to complete
  while ((*my_ADCSRA & 0x40) != 0);

  // Return 10-bit result
  return *my_ADC_DATA;
}



void loop() {
  //Time check.
  unsigned long nowCheck = millis();

  //BEGIN code for Vent Control.
  bool Btn1 = !(PIN_CounterClock & (1 << CounterClock_BTN_BIT));
  bool Btn2 = !(PIN_Clock & (1 << Clock_BTN_BIT));

  if (Btn1 && (nowCheck - Button1 > Debounce)) {
    Button1 = nowCheck;
    Cnt1++;
    if (Cnt1 != 2) {
      Cnt2 = 0;
      Direction = 1;
    } else {
      Cnt1 = 0;
      Direction = 3;
    }
}

if (Btn2 && (nowCheck - Button2 > Debounce)) {
    Button2 = nowCheck;
    Cnt2++;
    if (Cnt2 != 2) {
      Cnt1 = 0;
      Direction = 2;
    } else {
      Cnt2 = 0;
      Direction = 3;
    }
}
 
if (Direction == 1) {
    VentMotor.step(-1);
  } else if (Direction == 2) {
    VentMotor.step(1);
  }


  //BEGIN code to check water reservoir state and proceed with any necessary actions.
  unsigned int waterDataInput = adc_read(0); //This is not using ADC Library, see ADC functions.
  unsigned int threshold = 20;           

  //Check if the waterDataInput is less than the threshold, if so send message to LCD.
  if(waterDataInput < threshold)
  {
  //Only send message once per second (1000ms)
    if (nowCheck - previousMillis >= 1000) {
       previousMillis = nowCheck;
       
    // Display Error Message to LCD
    lcd.setCursor(0, 0);
    lcd.print("ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Reservoir Empty!");
    }
    wasEmpty = true;
  }
  else {
  if (wasEmpty) {
        lcd.clear();  // Clear only once when reservoir refills
        wasEmpty = false;
    }  
  }



  //BEGIN code to refresh + update LCD once per minute (60000ms) with fresh data.
  if (nowCheck - previousMillis >= 60000) {
    previousMillis = nowCheck;

    // Read values from DHT11 (Temp and Humidity)
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


   
  //BEGIN code for temp condition activities
  DateTime now = rtc.now();
  float temp = dht.readTemperature();
  if (temp > 30.0) {

    //Only log time once per second (1000ms)
    if (nowCheck - previousMillis >= 1000) {
       previousMillis = nowCheck;
    //Send date/time info over serial for logging purposes.
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
    }

    // Turn DC fan ON: IN1=HIGH (PH6), IN2=LOW (PH5), EN1=HIGH (PB4)
    PORTH |=  0b01000000;   // PH6 = 1
    PORTH &= ~0b00100000;   // PH5 = 0
    PORTB |=  0b00010000;   // PB4 = 1
  } 
  
  else {
    // Turn fan OFF when temp condition is not met: EN1=LOW
    PORTB &= ~0b00010000;   // PB4 = 0
  }
  
}
