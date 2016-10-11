#include "DMX512_Lib.h"
// ********************* local definitions *********************
		int 	gCurDmxCh;		//current DMX channel
		char	gDmxState;		//state of transition

enum {BREAK, STARTB, DATA};

// ********************* local definitions *********************
#ifndef F_CPU
	#define F_CPU 16000000UL
#endif
#define BAUD 250000
#define MYUBRR F_CPU/16/BAUD-1

volatile unsigned int byte_counter = 0;


// *************** DMX Reception Initialisation ****************
void init_DMX_RX( unsigned int ubrr)
{
	/* Set baud rate */
	UBRR1H = (unsigned char)(ubrr>>8);
	UBRR1L = (unsigned char)ubrr;
	/* Enable receiver and Interrupt*/
	UCSR1B = (1<<RXCIE1)|(1<<RXEN1);//|(1<<TXEN)
	/* Set frame format: 8data, 2stop bit */
	UCSR1C = (1<<USBS1)|(3<<UCSZ10);//
}

// *************** DMX Reception Initialisation ****************
void init_DMX_TX(void)
{
	//USART
	UBRR1H  = 0;
	UBRR1L  = ((F_CPU/4000000)-1);			//250kbaud, 8N2
	UCSR1C |= (3<<UCSZ10)|(1<<USBS1);
	UCSR1B |= (1<<TXEN1)|(1<<TXCIE1);
	UDR1    = 0;							//start USART

	//Data
	gDmxState= BREAK;					//start with break
}


// *************** DMX Reception ISR ****************
ISR(USART1_RX_vect)//Frame Recieve Interrupt
{
	unsigned char dummy;

	if(UCSR1A &(1<<FE1))//Frame Error?
	{//JA/////////////
		byte_counter = 0;
		dummy = UDR1;
		UCSR1A &= 0b11101111;
	}
	else
	{ //NEIN/////////////
		DMX_Data[byte_counter-2] = UDR1; //First bytes are wrong
	}
	byte_counter++;
}

// ****************** DMX Transmission ISR ********************

ISR (USART1_TX_vect)
{
	uint8_t DmxState= gDmxState;

	if (DmxState == BREAK)
	{
		UBRR1H  = 0;
		UBRR1L  = (F_CPU/1600000);					//90.9kbaud
		UDR1    = 0;								//send break
		gDmxState= STARTB;
	}
	else if (DmxState == STARTB)
	{
		UBRR1H  = 0;
		UBRR1L  = ((F_CPU/4000000)-1);				//250kbaud
		UDR1    = 0;								//send start byte
		gDmxState= DATA;
		gCurDmxCh= 0;
	}
	else
	{
		_delay_us(IBG);
		uint16_t CurDmxCh= gCurDmxCh;
		UDR1= DMX_Data[CurDmxCh++];				//send data
		if (CurDmxCh == sizeof(DMX_Data)) 
			gDmxState= BREAK; //new break if all ch sent
		else 
			gCurDmxCh= CurDmxCh;
	}
}
