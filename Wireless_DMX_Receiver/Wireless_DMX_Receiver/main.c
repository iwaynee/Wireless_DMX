/*******************************************************************************
Firma        : Roche Diagnostics International AG
               VET Department
Autor        : Ivan Heinzer

Projekt      : Wireless_DMX_Receiver
Version      : 1.0
Dateiname    : main.c
Erstelldatum : 23/05/2016 9:32:40 AM

********************************************************************************
Chip type         : ATmega32u4
CPU-Clock         : 16.0000 MHz

********************************************************************************
Datum                     Vers.    Comment / Change due
23/05/2016 9:32:40 AM     1.0      Created
13/09/2016 9:32:40 AM     2.0      Automatic Channel choice

*******************************************************************************/

/*------------------ Definitions for Delay Function --------------------------*/
#define F_CPU 16000000UL

/*------------------ Macros --------------------------------------------------*/
#define SETBIT(Adr,Bit)         (Adr |= (1<<Bit))
#define CLEARBIT(Adr,Bit)       (Adr &= ~(1<<Bit))
#define CHECKBIT(Adr,Bit)       (Adr & (1<<Bit))
#define INVERTBIT(Adr,Bit)      (Adr ^= (1<<Bit))

// Watchdog Software reset
#define soft_reset()        \
do                          \
{                           \
  wdt_enable(WDTO_15MS);  \
  for(;;)                 \
  {                       \
  }                       \
} while(0)

/*------------------------------- Include ------------------------------------*/
#include <avr/io.h>                 // I/O-Definitions
#include <avr/delay.h>              // _delay_ms() and _delay_us()
#include <avr/interrupt.h>          // Interrupt-Definitions

#include "../../Librarys/nrf24.c"   // NRF24 Library
#include "../../Librarys/radioPinFunctions.c" // NRF Pinconfiguration
#include "../../Librarys/DMX512_Lib.c" // DMX Library
#include <avr/wdt.h>                //Watchdog

/*------------------------ const and definitions  ----------------------------*/
#define   RGB_LED   PORTB
#define   PIN_RED   PORTB0
#define   PIN_GREEN PORTB1
#define   PIN_BLUE  PORTB2

/*------------------------ Global Variables  ---------------------------------*/
uint8_t Package_Data[32];
uint8_t rx_address[5] = {0xE7,0xE7,0xE7,0xE7,0x00};

/*----------------------------Functions--------------------------------------*/
void SetLED(char Red, char Green, char Blue);
char nrf24_search_channel();
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

/*----------------------------- Mainprogram --------------------------------*/

int main(void)
{
  // I/O-Configurations
  //  RGB LED
  DDRB = 0b00000111;
  //  DIP-Switch
  DDRD = 0b00000000;
  PORTD= 0b11110000;
  //  MAX485
  DDRE = 0b01000000;
  
  // Variables 
  int StatusLED = 0;
  int Counter = 0;
  
  rx_address[4] = (PIND&0b11110000)>>4; // Get the Adress from DIP's
  
  // Clear the Channel Array
  for (Counter=0; Counter<513; Counter++)
  {
    DMX_Data[Counter]= 0;
  }
  
  SetLED(1,1,0);

  // NRF Init
  nrf24_init();
  nrf24_rx_address(rx_address); // Set the device addresses
  nrf24_search_channel();       // Search Transmit Channel/Frequency
  
  // DMX Init
  SETBIT(PORTE,PORTE6);         // Set SN75176b to transmit
  init_DMX_TX();
  sei();                        // Global Interrupts Enable
  
  while(1)
  {
    // If the Transmitt Adress should be changed, the Device should do a Soft reset
    if (rx_address[4] != ((PIND&0b11110000)>>4)){
      soft_reset();
    }
     
    if(nrf24_dataReady())       // Wait for NRF to be Ready
    {
      SetLED(0,1,0);            // Green ON
      StatusLED = 1;

      nrf24_getData(Package_Data);// Get Data
      for (Counter = 1; Counter < 32; Counter++) // Convert Data
      {
        DMX_Data[Package_Data[0]*31+Counter-1] = Package_Data[Counter];        
      }
    }
    else
    {
      // NRF is NOT Ready
      SetLED(1,1,0);          // Red ON Green ON
      StatusLED++ ;            
    }
    if (StatusLED > 500)      // if more than 500 bad status were recognized
    {
      SetLED(1,0,0);          // Red ON
      nrf24_search_channel(); // After Connection Timeout search for another Channel
    }
    
  }
}

void SetLED(char Red, char Green, char Blue) {
  if (Red) {
    SETBIT(PORTB,PIN_RED);
    } else {
    CLEARBIT(PORTB,PIN_RED);
  }
  if (Green) {
    SETBIT(PORTB,PIN_GREEN);
    } else {
    CLEARBIT(PORTB,PIN_GREEN);
  }
  if (Blue) {
    SETBIT(PORTB,PIN_BLUE);
    } else {
    CLEARBIT(PORTB,PIN_BLUE);
  }
}


// scan for a transmiter device 
char nrf24_search_channel()
{
  // Variables
  char Channel = 0;
  char NrOfTry;

  while (1) //Endless until a channel is found
  {
    // If the Transmitt Adress should be changed, the Device should do a Soft reset
    if (rx_address[4] != ((PIND&0b11110000)>>4)){
      soft_reset();
    }
    
    // Test only the first 80 Channels
    // Visit: https://en.wikipedia.org/wiki/List_of_WLAN_channels#Interference_concerns
    if (Channel > 80){
      Channel = 1;
    } else {
      Channel++;
    }
    
    // Set new Channel
    nrf24_powerDown();
    nrf24_config(Channel,32);                     // Try new Channel, payload length: 32
    nrf24_powerUpRx();

    for (NrOfTry = 0; NrOfTry < 25; NrOfTry++)    // Refresh it 25 Times per Channel
    {
      if (nrf24_dataReady())                      // Check if the NRF is Ready
      {
        nrf24_getData(Package_Data);                // Get Data
        if ((Package_Data[0] > 0) && (Package_Data[0] < 30)) // Check if the Package is corrupt
        {
          return;                                 // New Channel is found and Setup
        }
      }
      _delay_us(10);
    }
  }
}


// Function Implementation
// For more Information please visit: http://www.atmel.com/webdoc/AVRLibcReferenceManual/FAQ_1faq_softreset.html
void wdt_init(void)
{
  MCUSR = 0;
  wdt_disable();

  return;
}

