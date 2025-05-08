##include <DHT.h>
#include <avr/io.h>

// DHT configuration
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);  

// Mega2560: D8 → PH5, D9 → PH6, D10 → PB4

// Timer1 register pointers
volatile unsigned char *myTCCR1A = (unsigned char*)0x0080;
volatile unsigned char *myTCCR1B = (unsigned char*)0x0081;
volatile unsigned int  *myTCNT1  = (unsigned int *)0x0084;
volatile unsigned char *myTIFR1  = (unsigned char*)0x0036;


void my_delay_ms(unsigned int ms) {
  unsigned int ticks = 0b11111010 * ms;  // 250 

  *myTCCR1B &= 0b11111000;      // Stop Timer1
  *myTCNT1   = 65536 - ticks;  
  *myTCCR1A  = 0b00000000;      
  *myTCCR1B |= 0b00000011;      

  while (!(*myTIFR1 & 0b00000001));  
  *myTCCR1B &= 0b11111000;           
  *myTIFR1  |= 0b00000001;           
}

void setup() {
  // Set D8 (PH5), D9 (PH6) as output
  DDRH |= 0b01100000;

  // Set D10 (PB4) as output
  DDRB |= 0b00010000;

  PORTH &= 0b10011111;  // Clear PH5, PH6
  PORTB &= 0b11101111;  // Clear PB4

  dht.begin();  
}

void loop() {
  my_delay_ms(1000);  // Wait 1 second

  float temp = dht.readTemperature();
  if (isnan(temp)) return;

  if (temp > 30.0) {
    // Turn fan ON: IN1=HIGH (PH6), IN2=LOW (PH5), EN1=HIGH (PB4)
    PORTH |=  0b01000000;   // PH6 = 1
    PORTH &= ~0b00100000;   // PH5 = 0
    PORTB |=  0b00010000;   // PB4 = 1
  } else {
    // Turn fan OFF: EN1=LOW
    PORTB &= ~0b00010000;   // PB4 = 0
  }
}
