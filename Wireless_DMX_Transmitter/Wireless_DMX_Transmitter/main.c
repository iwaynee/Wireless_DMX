/*******************************************************************************
Firma        : Roche Diagnostics International AG
               VET department
Autor        : Ivan Heinzer

Projekt      : Wireless_DMX_Transmitter
Version      : 1.0
Dateiname    : main.c
Erstelldatum : 23/05/2016 9:32:40 AM

********************************************************************************
Chip type         : ATmega32u4
CPU-Clock         : 16.0000 MHz

********************************************************************************
Datum                     Vers.    Kommentar / Aenderungsgrund
23/05/2016 9:32:40 AM     1.0      Creation
23/04/2016 9:32:40 AM     2.0      Send only new 
13/05/2016 9:32:40 AM     3.0      Automatic Channel choice

*******************************************************************************/

/*------------------ Definitions for Delay Function --------------------------*/
#define F_CPU 16000000UL
#define USE_RGBLED              //DELET THIS FOR MORE PERFORMANCE

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

#define   RAWDATASIZE 126

/*------------------------ Global Variables  ---------------------------------*/
uint8_t Package_Data[32];
char data_compare[18];
uint8_t tx_address[5] = {0xE7,0xE7,0xE7,0xE7,0x00};
  
/*----------------------------Functions--------------------------------------*/
void SetLED(char Red, char Green, char Blue);
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
  
/*----------------------------- Hauptprogramm --------------------------------*/

int main(void)
{
  // I/O-Konfigurationen
  //RGB LED
  DDRB = 0b00000111;
  //DIP-Switch
  DDRD = 0b00000000;
  PORTD= 0b11110000;
  //MAX485
  DDRE = 0b01000000;
  
  // Variables
  int StatusLED = 0;
  int Counter;
  int PackageCounter;
  int nData_compare = 0;
  char Best_Channel;
  
  // Get the Adress from DIP's
  tx_address[4] = (PIND&0b11110000)>>4; 
  
  // Clear the Channel Array
  for (Counter=0; Counter<513; Counter++) 
  {
    DMX_Data[Counter]= 0;
  }
  
  SetLED(1,1,0); // Show Red/Green
  
  // NRF Init
  nrf24_init();
  nrf24_config(2,32);                 // Channel #2 , payload length: 32
  nrf24_tx_address(tx_address);       // Set the device addresses
  
  Best_Channel = nRFscanChannels();
  nrf24_config(Best_Channel,32);      // Channel #2 , payload length: 32
  
  // DMX Init
  CLEARBIT(PORTE,PORTE6);             // Set to recieve
  init_DMX_RX(MYUBRR);
  sei();                              //Globale Interrupts Enable
  
  while(1)
  {
    // If the Transmitt Adress should be changed, the Device should do a Soft reset
    if (tx_address[4] != ((PIND&0b11110000)>>4)){
      soft_reset();
    }
    
    // Split Data in 31 Byte Packages
    for (PackageCounter = 0; PackageCounter < 17; PackageCounter++)
    {
      nData_compare = 0;                //Reset data compare
      Package_Data[0] = PackageCounter;   //Write Packet-Nr
              
      for (Counter = 1; Counter < 32; Counter++) //Write packet-Data
      {
        if ((PackageCounter*31+Counter) < 512)
        {
          Package_Data[Counter] = DMX_Data[PackageCounter*31+Counter-1];
          nData_compare += Package_Data[Counter];
        }
      }
      
      //Send only If Data has been changed. (Receiver will listen to these Packages 
      //                                     Package 0 and 8 will be sent every time.)
      if (data_compare[PackageCounter] != nData_compare  || PackageCounter == 0 || PackageCounter == 8  )
      {      
        nrf24_send(Package_Data); //Send Packet
        
        /* Wait for transmission to end */
        while(nrf24_isSending()); // Wait for the last transmission to end
   
        if (nrf24_lastMessageStatus() == NRF24_TRANSMISSON_OK)
        {
            StatusLED = 0;
            SetLED(0,1,0);
        } else if (StatusLED < 25)      // if more than 25 bad status were recognized
        {
            SetLED(1,1,0);
            StatusLED++ ;
        } else {
          SetLED(1,0,0);
        }  
      }
      data_compare[PackageCounter] = nData_compare; //Set new Compare data
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
  
// scanning all channels in the 2.4GHz band and search cleanest Channel
void nRFscanChannels(void) {
  unsigned char numberOfChannels = 80; //there are 126, 0 to 125
  unsigned int i,j;
  char CD_value;
  char RAWDATA[RAWDATASIZE];
  int CCHANNEL[8+1];
  char BestChannel;
  

  //Start with CE clear
  nrf24_ce_digitalWrite(LOW);
  //Set RX mode
  nrf24_powerUpRx();

  //clear RAWDATA
  for (i = 0; i < RAWDATASIZE; i++){
    RAWDATA[i] = 0;
  }
  
  //clear CCHANNEL
  for (i = 0; i < 9; i++){
    CCHANNEL[i] = 0;
  }

  RAWDATA[0] = (char) numberOfChannels;

  nrf24_ce_digitalWrite(LOW);
  for (j = 0; j < 40; j++) {

    for (i = 0; i < numberOfChannels; i++) {
      // select a new channel
      nrf24_configRegister(RF_CH,i); //will do every second one;
      nrf24_ce_digitalWrite(HIGH);
      //recomended at least 258uS but I found 350uS is need for reliability
      _delay_us(300);

      // received power > -64dBm
      nrf24_readRegister(CD,&CD_value,1);
      if (CD_value){
        RAWDATA[i + 1]++;
      }

      //this seems to be needed after the timeout not before
      nrf24_ce_digitalWrite(LOW);
      _delay_us(4);

    }
  }
  
  // Do average of 10 Channels 
  for (j = 1; j < 80; j++)
  {
    i = j / 10;
    CCHANNEL[i] += RAWDATA[j];
  }
  
  //Find best area of 8 areas
  i = 0xFF;
  for (j = 0; j < 8; j++)
  {
    if (CCHANNEL[j] < i){
      i = CCHANNEL[j];
      BestChannel = j * 10;
    }
  }
  
  //Find best channel in the cleanest Area
  i = 0xFF;
  for (j = BestChannel; j < (BestChannel+10); j++)
  {
    if (RAWDATA[j] < i){
      i = RAWDATA[j];
      BestChannel = j;
    }
  }
  
  return BestChannel;
}

// Function Implementation
void wdt_init(void)
{
  MCUSR = 0;
  wdt_disable();

  return;
}