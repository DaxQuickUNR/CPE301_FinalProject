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
  
  //NOTE: 75 is a placeholder value for testing purposes. (This will be modified after testing with actual project reservoir)
  unsigned int threshold = 75;           

  //If the waterDataInput is less than the threshold, display a warning message on the Serial monitor.
  if(waterDataInput < threshold)
  {
    char *thresholdMessage = "Water Level is Low! Please fill resevoir. \r\n";
    int i = 0;
    while (thresholdMessage[i] != '\0')
    {
      //We can use "U0outchar" to send each character to the console one at a time.
      U0putchar(thresholdMessage[i]);
      i++;
    }


    
    //NOTE: Future interfacing with LCD to display error message will go here.


     
  }
else
{
  //If threshold isn't exceeded we want to print data reading. However, this must be done character by character.

    if (waterDataInput >= 100) 
    {
      U0putchar((waterDataInput / 100) % 10 + '0');  // Seperate out and print hundreds digit if it exists
    }

    if (waterDataInput >= 10)
    {
      U0putchar((waterDataInput / 10) % 10 + '0');   // Seperate out and print tens digit if it exists
    }

    U0putchar(waterDataInput % 10 + '0');            // Seperate out and print ones digit (this will always exist)

  U0putchar('\n');  //Move to the next line after digits have been sent to the Serial Monitor.

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
