#include <Stepper.h>

#define Clock_BTN_BIT 7          
#define CounterClock_BTN_BIT 5    

#define DDR_Clock DDRL
#define PORT_Clock PORTL
#define PIN_Clock PINL
#define DDR_CounterClock DDRL   
#define PORT_CounterClock PORTL
#define PIN_CounterClock PINL

unsigned long Button1 = 0;
unsigned long Button2 = 0;
const unsigned long Debounce = 300;
int Cnt1 = 0;
int Cnt2 = 0;
int Direction = 3;

Stepper VentMotor(2048, 22, 26, 24, 28);

void setup() {
  VentMotor.setSpeed(10);
   VentMotor.step(0);
    DDR_Clock &= ~(1 << Clock_BTN_BIT);
    PORT_Clock |= (1 << Clock_BTN_BIT);
    DDR_CounterClock &= ~(1 << CounterClock_BTN_BIT);
    PORT_CounterClock |= (1 << CounterClock_BTN_BIT);
}

void loop() {
  unsigned long now = millis();
  bool Btn1 = !(PIN_CounterClock & (1 << CounterClock_BTN_BIT));
  bool Btn2 = !(PIN_Clock & (1 << Clock_BTN_BIT));
  
if (Btn1 && (now - Button1 > Debounce)) {
    Button1 = now;
    Cnt1++;
    if (Cnt1 != 2) {
      Cnt2 = 0;
      Direction = 1;
    } else {
      Cnt1 = 0;
      Direction = 3;
    }
}

if (Btn2 && (now - Button2 > Debounce)) {
    Button2 = now;
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
}
