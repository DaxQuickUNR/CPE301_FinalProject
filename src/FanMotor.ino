#include <dht.h>
#include <avr/io.h>

// DHT11  pin 7
#define DHT11_PIN   7

// Mega2560：D8→PH5, D9→PH6, D10→PB4
#define IN2_MASK    0b00100000   // PORTH5 (D8)
#define IN1_MASK    0b01000000   // PORTH6 (D9)
#define EN1_MASK    0b00010000   // PORTB4 (D10)

// Timer1 
volatile unsigned char *myTCCR1A = (unsigned char*)0x0080;
volatile unsigned char *myTCCR1B = (unsigned char*)0x0081;
volatile unsigned int  *myTCNT1  = (unsigned int *)0x0084;
volatile unsigned char *myTIFR1  = (unsigned char*)0x0036;

dht DHT;

void my_delay_ms(unsigned int ms) {
  unsigned int ticks = 0b11111010;  
  *myTCCR1B &= 0b11111000;
  *myTCNT1   = 65536 - ticks;
  *myTCCR1A  = 0b00000000;
  *myTCCR1B |= 0b00000011;
  while (!(*myTIFR1 & 0b00000001));
  // Stop Timer1
  *myTCCR1B &= 0b11111000;
  *myTIFR1  |= 0b00000001;
}

void setup() {
  // GPIO  
  // D9、D8 output
  DDRH   |= 0b01100000;
  // D10 input
  DDRB   |= 0b00010000;

  PORTH  &= 0b10011111;
  PORTB  &= 0b11101111;
}

void loop() {
  // Every Second
  my_delay_ms(1000);

  // read DHT11 temperature
  DHT.read11(DHT11_PIN);
  float temp = DHT.temperature;

  if (temp > 30.0) {
    PORTH |=  0b01000000; 
    PORTH &= ~0b00100000;  
    PORTB |=  0b00010000;  
  } else {
    PORTB &= ~0b00010000;  
  }
}
