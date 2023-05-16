#define F_CPU 4000000UL

#define USART_BAUDRATE 9600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <util/delay.h>
#include<avr/interrupt.h>
#include <avr/pgmspace.h>

#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1

#define     SR_DI_H    BIT_SET(PORTC,3)
#define     SR_DI_L    BIT_CLEAR(PORTC,3)
#define     SR_CLK_H   BIT_SET(PORTC,0)
#define     SR_CLK_L   BIT_CLEAR(PORTC,0)
#define     SR_LE_H    BIT_SET(PORTC,1)
#define     SR_LE_L    BIT_CLEAR(PORTC,1)
#define     SR_OE_H    BIT_SET(PORTC,2)
#define     SR_OE_L    BIT_CLEAR(PORTC,2)
#define     DA_1_H     BIT_SET(PORTC,5)
#define     DA_1_L     BIT_CLEAR(PORTC,5)
#define     DA_2_H     BIT_SET(PORTC,4)
#define     DA_2_L     BIT_CLEAR(PORTC,4)
#define     DA_3_H     BIT_SET(PORTD,5)
#define     DA_3_L     BIT_CLEAR(PORTD,5)
#define     DA_4_H     BIT_SET(PORTD,6)
#define     DA_4_L     BIT_CLEAR(PORTD,6)

#define     DP_1_H     BIT_SET(PORTD,2)
#define     DP_1_L     BIT_CLEAR(PORTD,2)
#define     DP_2_H     BIT_SET(PORTD,3)
#define     DP_2_L     BIT_CLEAR(PORTD,3)
#define     DP_3_H     BIT_SET(PORTD,4)
#define     DP_3_L     BIT_CLEAR(PORTD,4)


//slighly altered from https://github.com/dmadison/LED-Segment-ASCII
const PROGMEM uint16_t ASCII[96] = {
	0b0000000000000000, /* (space) */
	0b0000000000001100, /* ! */
	0b0000001000000100, /* " */
	0b1010101000111100, /* # */
	0b1010101010111011, /* $ */
	0b1110111010011001, /* % */
	0b1001001101110001, /* & */
	0b0000001000000000, /* ' */
	0b0001010000000000, /* ( */
	0b0100000100000000, /* ) */
	0b1111111100000000, /* * */
	0b1010101000000000, /* + */
	0b0100000000000000, /* , */
	0b1000100000000000, /* - */
	0b0001000000000000, /* . */
	0b0100010000000000, /* / */
	0b0100010011111111, /* 0 */
	0b0000010000001100, /* 1 */
	0b1000100001110111, /* 2 */
	0b0000100000111111, /* 3 */
	0b1000100010001100, /* 4 */
	0b1001000010110011, /* 5 */
	0b1000100011111011, /* 6 */
	0b0000000000001111, /* 7 */
	0b1000100011111111, /* 8 */
	0b1000100010111111, /* 9 */
	0b0010001000000000, /* : */
	0b0100001000000000, /* ; */
	0b1001010000000000, /* < */
	0b1000100000110000, /* = */
	0b0100100100000000, /* > */
	0b0010100000000111, /* ? */
	0b0000101011110111, /* @ */
	0b1000100011001111, /* A */
	0b0010101000111111, /* B */
	0b0000000011110011, /* C */
	0b0010001000111111, /* D */
	0b1000000011110011, /* E */
	0b1000000011000011, /* F */
	0b0000100011111011, /* G */
	0b1000100011001100, /* H */
	0b0010001000110011, /* I */
	0b0000000001111100, /* J */
	0b1001010011000000, /* K */
	0b0000000011110000, /* L */
	0b0000010111001100, /* M */
	0b0001000111001100, /* N */
	0b0000000011111111, /* O */
	0b1000100011000111, /* P */
	0b0001000011111111, /* Q */
	0b1001100011000111, /* R */
	0b1000100010111011, /* S */
	0b0010001000000011, /* T */
	0b0000000011111100, /* U */
	0b0100010011000000, /* V */
	0b0101000011001100, /* W */
	0b0101010100000000, /* X */
	0b1000100010111100, /* Y */
	0b0100010000110011, /* Z */
	0b0010001000010010, /* [ */
	0b0001000100000000, /* \ */
	0b0010001000100001, /* ] */
	0b0101000000000000, /* ^ */
	0b0000000000110000, /* _ */
	0b0000000100000000, /* ` */
	0b1010000001110000, /* a */
	0b1010000011100000, /* b */
	0b1000000001100000, /* c */
	0b0010100000011100, /* d */
	0b1100000001100000, /* e */
	0b1010101000000010, /* f */
	0b1010001010100001, /* g */
	0b1010000011000000, /* h */
	0b0010000000000000, /* i */
	0b0010001001100000, /* j */
	0b0011011000000000, /* k */
	0b0000000011000000, /* l */
	0b1010100001001000, /* m */
	0b1010000001000000, /* n */
	0b1010000001100000, /* o */
	0b1000001011000001, /* p */
	0b1010001010000001, /* q */
	0b1000000001000000, /* r */
	0b1010000010100001, /* s */
	0b1000000011100000, /* t */
	0b0010000001100000, /* u */
	0b0100000001000000, /* v */
	0b0101000001001000, /* w */
	0b0101010100000000, /* x */
	0b0000101000011100, /* y */
	0b1100000000100000, /* z */
	0b1010001000010010, /* { */
	0b0010001000000000, /* | */
	0b0010101000100001, /* } */
	0b1100110000000000, /* ~ */
	0b0000000000000000, /* (del) */
};

uint16_t segment_bitmap[12];
uint8_t disp_msg[12];
uint8_t disp_msg_tmp[12];
uint8_t anode_act=0;


uint16_t segment_order (uint16_t in)
	{
	uint16_t out=0;
	if (in&(1<<0)) out|=(1<<6);
	if (in&(1<<1)) out|=(1<<8);
	if (in&(1<<2)) out|=(1<<3);
	if (in&(1<<3)) out|=(1<<10);
	if (in&(1<<4)) out|=(1<<9);
	if (in&(1<<5)) out|=(1<<15);
	if (in&(1<<6)) out|=(1<<12);
	if (in&(1<<7)) out|=(1<<5);
	if (in&(1<<8)) out|=(1<<1);
	if (in&(1<<9)) out|=(1<<2);
	if (in&(1<<10)) out|=(1<<7);
	if (in&(1<<11)) out|=(1<<4);
	if (in&(1<<12)) out|=(1<<11);
	if (in&(1<<13)) out|=(1<<13);
	if (in&(1<<14)) out|=(1<<14);
    if (in&(1<<15)) out|=(1<<0);
	return out;
	}

void set_sr_16 (uint16_t data)
{
unsigned char i;
for (i=0;i<16;i++)
	{
	if ((data&0x8000)==0)
		{
		SR_DI_L;
		}
	else
		{
		SR_DI_H;
		}
	SR_CLK_H;
	data = data << 1;
	SR_CLK_L;
	}
}

void set_sr_3x16 (uint16_t data1, uint16_t data2, uint16_t data3)
{
set_sr_16(data1);
set_sr_16(data2);
set_sr_16(data3);
_delay_us(10);
SR_LE_H;
_delay_us(10);
SR_LE_L;

}


void set_anode (uint8_t anode)
{
DA_1_H;
DA_2_H;
DA_3_H;
DA_4_H;
if (anode==0) DA_1_L;
if (anode==1) DA_2_L;
if (anode==2) DA_3_L;
if (anode==3) DA_4_L;
}


// timer0 overflow
ISR(TIMER0_OVF_vect) {
    TCNT0=220;
}

ISR (TIMER1_OVF_vect)    // Timer1 ISR
{
	set_anode(5);
	if (anode_act==0)
		{
		set_sr_3x16(segment_bitmap[10],segment_bitmap[4],segment_bitmap[2]);
		set_anode(0);
		}
	if (anode_act==1)
		{
		set_sr_3x16(segment_bitmap[11],segment_bitmap[5],segment_bitmap[3]);
		set_anode(1);
		}
	if (anode_act==2)
		{
		set_sr_3x16(segment_bitmap[8],segment_bitmap[6],segment_bitmap[0]);
		set_anode(2);
		}
	if (anode_act==3)
		{
		set_sr_3x16(segment_bitmap[9],segment_bitmap[7],segment_bitmap[1]);
		set_anode(3);
		}
	anode_act++;
	if (anode_act==4) anode_act=0;
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

int main (void)
{
    uint8_t i,rx_rcvd;
    DDRB |= 0xFF; 
	DDRC |= 0xFF; 
	DDRD |= 0xFF; 
    UCSRB = (1 << RXEN) | (1 << TXEN);   // Turn on the transmission and reception circuitry
//    UCSRB |= (1 << RXCIE);
    UBRRH = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
    UBRR = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
	SR_OE_L;
    TCNT1 = 63974;   // for 1 sec at 16 MHz	
	TCCR1A = 0x00;
    TCCR1B = (1<<CS10) | (1<<CS11);
    TCNT0=0x00;
    TCCR0 = (1<<CS02) | (1<<CS00);
	TIMSK=(1<<TOIE0) | (1<<TOIE1);
	sei();        // Enable global interrupts by setting global interrupt enable bit in SREG
    DP_1_H;
    DP_2_H;
    DP_3_H;
    for (i=0;i<12;i++) disp_msg[i] = '-';
    for (i=0;i<12;i++) disp_msg_tmp[i] = disp_msg[i];
    while(1) 
		{
		if (uart_rx_rdy()!=0)
			{
			rx_rcvd = uart_rx_read();
			if (rx_rcvd>(' '-1))
				{
				for (i=1;i<12;i++) disp_msg_tmp[i-1] = disp_msg_tmp[i];
				disp_msg_tmp[11] = rx_rcvd;
				}
			else
				{
				for (i=0;i<12;i++) disp_msg[i] = disp_msg_tmp[i];
				}
			}
        for (i=0;i<12;i++)
            {
            segment_bitmap[i] = segment_order(pgm_read_word (ASCII+disp_msg[i]-32));
            }
		}
}
