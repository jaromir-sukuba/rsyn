#define F_CPU 8000000UL

#define USART_BAUDRATE 9600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <util/delay.h>
#include<avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>


#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1

#define     SW_1_H     BIT_SET(PORTD,2)
#define     SW_1_L     BIT_CLEAR(PORTD,2)
#define     SW_2_H     BIT_SET(PORTD,3)
#define     SW_2_L     BIT_CLEAR(PORTD,3)
#define     SW_3_H     BIT_SET(PORTD,4)
#define     SW_3_L     BIT_CLEAR(PORTD,4)
#define     SW_4_H     BIT_SET(PORTD,5)
#define     SW_4_L     BIT_CLEAR(PORTD,5)
#define     SW_5_H     BIT_SET(PORTD,6)
#define     SW_5_L     BIT_CLEAR(PORTD,6)
#define     SW_6_H     BIT_SET(PORTB,1)
#define     SW_6_L     BIT_CLEAR(PORTB,1)
#define     SWI_1_H     BIT_SET(PORTD,7)
#define     SWI_1_L     BIT_CLEAR(PORTD,7)
#define     SWI_2_H     BIT_SET(PORTB,0)
#define     SWI_2_L     BIT_CLEAR(PORTB,0)

#define     LED_H     BIT_SET(PORTB,2)
#define     LED_L     BIT_CLEAR(PORTB,2)

#define     MOSI_H     BIT_SET(PORTC,4)
#define     MOSI_L     BIT_CLEAR(PORTC,4)
#define     SCK_H     BIT_SET(PORTC,3)
#define     SCK_L     BIT_CLEAR(PORTC,3)
#define     CS_H     BIT_SET(PORTC,5)
#define     CS_L     BIT_CLEAR(PORTC,5)
#define     MS_H     BIT_SET(PORTC,1)
#define     MS_L     BIT_CLEAR(PORTC,1)



// timer0 overflow
ISR(TIMER0_OVF_vect) {
    TCNT0=220;
}

ISR (TIMER1_OVF_vect)    // Timer1 ISR
{
	TCNT1 = 65488;
}

ISR(UART_RX_vect)
{
   char ReceivedByte;
   ReceivedByte = UDR; // Fetch the received byte value into the variable "ByteReceived"
   UDR = ReceivedByte; // Echo back the received byte back to the computer
}


void uart_tx (uint8_t data)
{
    while ((UCSRA & (1 << UDRE)) == 0) {};
    UDR = data;
}

uint8_t uart_rx_rdy (void)
    {
    if (UCSRA & (1 << RXC))
        return 1;
    else 
        return 0;
    }

uint8_t uart_rx_read (void)
    {
    return UDR;
    }


void spi_out (uint8_t data)
	{
	uint8_t i;
	for (i=0;i<8;i++)
		{
		if (data&0x80)
			{
			MOSI_H;
			}
		else
			{
			MOSI_L;
			}
		_delay_us (10);
		SCK_H;
		_delay_us (10);
		SCK_L;
		_delay_us (10);
		data = data << 1;
		}
		
	}


uint8_t rx_state=0,rx_data,rx_ptr;
uint32_t dac_val;
char rx_str[10];
char tx_str[10];

int main (void)
{
    DDRB |= 0xFF; 
	DDRC |= 0xFB; 
	DDRD |= 0xFF; 
    UCSRB = (1 << RXEN) | (1 << TXEN);   // Turn on the transmission and reception circuitry
//    UCSRB |= (1 << RXCIE);
    UBRRH = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
    UBRR = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
	TCNT1 = 63974;   // for 1 sec at 16 MHz	
	TCCR1A = 0x00;
    TCCR1B = (1<<CS10) | (1<<CS11);
    TCNT0=0x00;
    TCCR0 = (1<<CS02) | (1<<CS00);
	TIMSK=(1<<TOIE0) | (1<<TOIE1);
//	sei();        // Enable global interrupts by setting global interrupt enable bit in SREG
	
	SW_1_L;
	SW_2_H;
	SW_3_H;
	SW_4_L;
	SW_5_H;
	SW_6_H;
	SWI_1_H;
	SWI_2_L;
	MS_H;
    while(1) 
		{
		if (uart_rx_rdy()!=0)
			{
			rx_data = uart_rx_read();
//			uart_tx(rx_data);
			if (rx_data<' ') 
				{
				rx_str[rx_ptr] = 0;
				rx_state = 1;
				rx_ptr = 0;
				}
			else 
				rx_str[rx_ptr++] = rx_data;
//			if (rx_ptr>sizeof(rx_str)) rx_ptr--;
			}
		if (rx_state == 1)
			{
			rx_state = 0;
			if (rx_str[0]=='d')
				{
				dac_val = atol(rx_str+1);
				dac_val = dac_val << 6;
				CS_L;
				spi_out(0x70);
				spi_out((dac_val>>16)&0xFF);
				spi_out((dac_val>>8)&0xFF);
				spi_out((dac_val>>0)&0xFF);
				CS_H;
				}
			if (rx_str[0]=='r')
				{
                if (rx_str[1]=='0')
                    {
	                SWI_1_H;
	                SWI_2_L;
                    }
                if (rx_str[1]=='1')
                    {
	                SWI_1_L;
	                SWI_2_H;
                    }
                }

			}

		}
}
