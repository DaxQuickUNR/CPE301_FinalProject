//Water Reservoir Monitoring
//By: Dax Quick
//Note: Implimentation based off of prior CPE301 LAB assignments.

#define RDA 0x80
#define TBE 0x20  

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

volatile unsigned char* my_ADMUX  = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int*  my_ADC_DATA = (unsigned int*) 0x78;

//Duplicate of Delay implementation from DateTimeSensor Code, will merge in  final code file.
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

void setup() 
{
  // setup the UART
  U0init(9600);   
  // setup the ADC
  adc_init();      
}

void loop() 
{
  //Setup variable to read waterDataInput from channel 0 (A0) as this is where the sensor is connected.
  unsigned int waterDataInput = adc_read(0);
  
  unsigned int threshold = 80;           

  //If the waterDataInput is less than the threshold, display a warning message on the Serial monitor.
  if(waterDataInput < threshold)
  {
    char *thresholdMessage = "Water Level is Low! Please fill resevoir. \r\n";
    oneSecDelay(); //Delay so serial monitor isn't spammed.
    int i = 0;
    while (thresholdMessage[i] != '\0')
    {
      //We can use "U0outchar" to send each character to the console one at a time.
      U0putchar(thresholdMessage[i]);
      i++;
    } 
  }
}

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

void U0init(int U0baud)
{
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0  = tbaud;
}

unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while ((*myUCSR0A & TBE) == 0);
  *myUDR0 = U0pdata;
}
