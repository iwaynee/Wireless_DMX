/* FOR ATMEGA32u4!!!!!!!!!!!!!!!! Tiny Print*/

#include <avr/io.h>

#define set_bit(reg,bit) reg |= (1<<bit)
#define clr_bit(reg,bit) reg &= ~(1<<bit)
#define check_bit(reg,bit) (reg&(1<<bit))

#define NRFPORT      PORTB
#define NRFPIN       PINB
#define NRFDDR       DDRB

#define NRF_CE      5
#define NRF_SCK     7
#define NRF_MOSI    4
#define NRF_MISO    6
#define NRF_CSN     3

/* ------------------------------------------------------------------------- */
void nrf24_setupPins()
{
    set_bit(NRFDDR,NRF_CE); // CE output
    set_bit(NRFDDR,NRF_CSN); // CSN output
    set_bit(NRFDDR,NRF_SCK); // SCK output
    set_bit(NRFDDR,NRF_MOSI); // MOSI output
    clr_bit(NRFDDR,NRF_MISO); // MISO input
}
/* ------------------------------------------------------------------------- */
void nrf24_ce_digitalWrite(uint8_t state)
{
    if(state)
    {
        set_bit(NRFPORT,NRF_CE);
    }
    else
    {
        clr_bit(NRFPORT,NRF_CE);
    }
}
/* ------------------------------------------------------------------------- */
void nrf24_csn_digitalWrite(uint8_t state)
{
    if(state)
    {
        set_bit(NRFPORT,NRF_CSN);
    }
    else
    {
        clr_bit(NRFPORT,NRF_CSN);
    }
}
/* ------------------------------------------------------------------------- */
void nrf24_sck_digitalWrite(uint8_t state)
{
    if(state)
    {
        set_bit(NRFPORT,NRF_SCK);
    }
    else
    {
        clr_bit(NRFPORT,NRF_SCK);
    }
}
/* ------------------------------------------------------------------------- */
void nrf24_mosi_digitalWrite(uint8_t state)
{
    if(state)
    {
        set_bit(NRFPORT,NRF_MOSI);
    }
    else
    {
        clr_bit(NRFPORT,NRF_MOSI);
    }
}
/* ------------------------------------------------------------------------- */
uint8_t nrf24_miso_digitalRead()
{
    return check_bit(NRFPIN,NRF_MISO);
}
/* ------------------------------------------------------------------------- */
