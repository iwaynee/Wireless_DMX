#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define IBG   		(10)				//interbyte gap [us]

extern void 	init_DMX_TX(void);
extern void    	init_DMX_RX( unsigned int ubrr);

// DMX variabeln
volatile unsigned char DMX_Data [513];