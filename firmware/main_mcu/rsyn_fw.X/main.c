#include <xc.h>
#include <stdio.h>
#include <string.h>

const char idn_string[13] = "RSYN v 1.00\n\0";

#define		_XTAL_FREQ		64000000
#pragma config FEXTOSC = OFF    // External Oscillator mode Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_64MHZ// Power-up default value for COSC bits (HFINTOSC with HFFRQ = 64 MHz and CDIV = 1:1)
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)
#pragma config MCLRE = EXTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTE = ON       // Power-up Timer Enable bit (Power up timer enabled)
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)
#pragma config BORV = VBOR_2P45 // Brown Out Reset Voltage selection bits (Brown-out Reset Voltage (VBOR) set to 2.45V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = OFF
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config DEBUG = OFF      // Debugger Enable bit (Background debugger disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF       // WDT operating mode (WDT Disabled)
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)


#define	USART1_BAUD	9600
#define	USART1_BRG	((_XTAL_FREQ/64)/USART1_BAUD)-1
#define	USART2_BAUD 9600
#define	USART2_BRG	((_XTAL_FREQ/64)/USART2_BAUD)-1

typedef union
{
struct
 {
    unsigned k1:1;
    unsigned k2:1;
    unsigned k3:1;
    unsigned k4:1;
    unsigned k5:1;
    unsigned k6:1;
    unsigned k7:1;
 };
uint8_t  CHAR;
}key_var;

key_var keys_new,keys_old,keys;

void tx2_char (char data)
	{
	while (PIR3bits.TX2IF==0);
	TX2REG = data;
	}

void tx1_char (char data)
	{
	while (PIR3bits.TX1IF==0);
	TX1REG = data;
	}

void tx2_chars (char * data)
	{
	while (*data) 
		tx2_char(*data++);
	}

void tx1_chars (char * data)
	{
	while (*data) 
		tx1_char(*data++);
	}


char tx_str[20];
unsigned char var1;


void set_u2_pps (unsigned char uart_cfg)
	{
	GIE = 0;
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0;
	if (uart_cfg==0)
		{
		RB0PPS = 0x0B;	//TX2
		RB2PPS = 0;
		}
	else
		{
		RB0PPS = 0;
		RB2PPS = 0x0B;	//TX2		
		}
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 1;
	GIE = 1;
	}

unsigned char key_read (void)
	{
	return PORTC&0x0F;
	}


typedef struct range
	{
	float setval_max;
	float setval_min;
	float range_mul;
	} 
range;

typedef struct cal_single
	{
	float cal_offset;
	float cal_gain;
	} 
cal_single;


unsigned long dacval;
unsigned int setval_exp;
unsigned char range_act,blink_on;
float setval,setval_inc,setval_temp;
float setval_inc_table [5] = {0.1,0.01,0.001,0.0001,0.00001};

#define RANGE_NUM	2
range ranges[RANGE_NUM];
cal_single cal[RANGE_NUM];
char rcvd_str[40],rcvd_ptr;
volatile char rcvd_flag;


void display_set_value (float val, unsigned char exp, unsigned char range, unsigned char blink_en)
	{
	static unsigned char blink;
	blink++;
	if (range==0) sprintf (tx_str,"%07.4f kOhm\n",val);
	if (range==1) sprintf (tx_str,"%07.3f kOhm\n",val);
	if ((blink&0x02)&&(blink_en!=0)) 
		{
		if (exp>range) exp++;
		tx_str[exp+1]=' ';
		}
	set_u2_pps(0);
	__delay_ms(1);
	tx2_chars(tx_str);
	__delay_ms(5);
	}


void __interrupt() ISR(void)
{
char temp;
if (RC1IF)
	{
	RC1IF = 0;
	temp = RC1REG;
	if (temp<0x20)
		{
		if (rcvd_ptr>0)
			{
			rcvd_str[rcvd_ptr] = 0;
			rcvd_flag = 1;
			rcvd_ptr = 0;
			}
		}
	else
		{
		rcvd_str[rcvd_ptr++] = temp;
		}
	if (rcvd_ptr>=sizeof(rcvd_str)) rcvd_ptr = 0;
	}
}

unsigned char ee_read(unsigned int address)
	{
	NVMCON1=0;				//ensure CFGS=0 and EEPGD=0
	NVMADRL = address&0xFF;
	//EEADRH= (address>>8)&0x03;
	NVMCON1bits.RD = 1;
	NVMCON1=0x80;
	return(NVMDAT);
	}


void ee_write(unsigned int address,unsigned char data)
	{
	unsigned char SaveInt;
	SaveInt=INTCON;			//save interrupt status
	NVMCON1=0;				//ensure CFGS=0 and EEPGD=0
	NVMCON1bits.WREN = 1;	//enable write to EEPROM
	NVMADRL = address&0xFF;
	//EEADRH= (address>>8)&0x03;
	NVMDAT = data;			//and data
	INTCONbits.GIE=0;		//No interrupts
	NVMCON2 = 0x55;			//required sequence #1
	NVMCON2 = 0xAA;			//#2
	NVMCON1bits.WR = 1;		//#3 = actual write
	__delay_us(100);
	INTCON=SaveInt;			//restore interrupts
	while(NVMCON1bits.WR);	//wait until finished
	NVMCON1bits.WREN = 0;	//disable write to EEPROM
	NVMCON1=0x80;
	}

void ee_save_calib(cal_single * cal_arr)
	{
	volatile unsigned char i,addr,temp;
	unsigned char * c_ptr;
	c_ptr = (unsigned char *)(cal_arr);
	addr = 0;
	for (i=0;i<4*4;i++)
		{
		temp = *c_ptr++;
		ee_write(addr++,temp);
		}
	}

void ee_load_calib(cal_single * cal_arr)
	{
	volatile unsigned char i,addr,temp;
	unsigned char * c_ptr;
	c_ptr = (unsigned char *)(cal_arr);
	addr = 0;
	for (i=0;i<4*4;i++)
		{
		temp = ee_read(addr++);
		*c_ptr = temp;
		*c_ptr++;
		}
	
	for (i=0;i<2;i++)
		{
		if ((cal[i].cal_gain > 1000000.0)|(cal[i].cal_gain < -1000000.0)) cal[i].cal_gain = 12345.0;
		if ((cal[i].cal_offset > 10.0)|(cal[i].cal_offset < -10.0)) cal[i].cal_offset = 0.0;
		}
	}

void serial_tasks (void)
	{
	char log_row[70];
	int i,n,len;
	double val_d;
	char *ptr;
	int status;
	if (rcvd_flag == 1)
		{
		rcvd_flag = 0;
		sprintf (log_row,"error\n");
		len = strlen (rcvd_str);
		for (i=0;i<len;i++)
			{
			if (rcvd_str[i]==',') rcvd_str[i] = '.';
			}
		if (strncmp("idn",rcvd_str,3)==0) sprintf (log_row,"%s\n",idn_string);
		if (strncmp("scc",rcvd_str,3)==0)
			{
			n = strtol(rcvd_str+4,&ptr,10);
			val_d = strtod(ptr,NULL);
			*(&(cal[0].cal_offset)+n) = val_d;
			sprintf (log_row,"scc: %d %1.6f\n",n, val_d);
			}
		if (strncmp("gcc",rcvd_str,3)==0) 
			{
			n = strtol(rcvd_str+4,&ptr,10);
			sprintf (log_row,"gcc: %d %1.3f\n",n,*(&(cal[0].cal_offset)+n));
			}
		if (strncmp("gcd",rcvd_str,3)==0)
			{
			sprintf (log_row,"gcd:\n");
			tx1_chars(log_row);
			for (i=0;i<4;i++)
				{
				sprintf (log_row,"gcd: %d %1.3f\n",i,*(&(cal[0].cal_offset)+i));
				tx1_chars(log_row);
				}
			sprintf (log_row,"\n");
			tx1_chars(log_row);
			}
		if (strncmp("wmc",rcvd_str,3)==0)
			{
			sprintf (log_row,"wmc: ");
			tx1_chars(log_row);
			ee_save_calib(cal);
			sprintf (log_row,"calibration array saved\n");
			}

		if (strncmp("grv",rcvd_str,3)==0) sprintf (log_row,"grv: %1.3f\n",setval);
		if (strncmp("srv",rcvd_str,3)==0)
			{
			val_d = strtod(rcvd_str+4,NULL);
			setval = val_d;
			sprintf (log_row,"srr: %d %1.6f\n",n, val_d);
			}

		if (strncmp("grr",rcvd_str,3)==0) sprintf (log_row,"grr: %d\n",range_act);		
		if (strncmp("srr",rcvd_str,3)==0)
			{
			n = strtol(rcvd_str+4,&ptr,10);
			if ((n<RANGE_NUM)&(n>=0))
				range_act = n;
			else
				n=0;
			sprintf (log_row,"srr: %d\n",range_act);
			}
		
		tx1_chars(log_row);
		}
	
	}

void main(void)
	{
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0;
	RB0PPS = 0x0B;	//TX2
	RB4PPS = 0x09;	//TX1
	RX1PPS = 0x0D;	//RX1
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 1;

	TX1STA = 0x20;
	RC1STA = 0x90;
	SPBRG1 = USART1_BRG;
	TX2STA = 0x20;
	RC2STA = 0x90;
	SPBRG2 = USART2_BRG;
	
	TRISC = 0x0F;
	ANSELC = 0;
	ANSELB = 0;
	
	RC1IF = 0;
	RC1IE = 1;
	PEIE = 1;
	GIE = 1;

	TRISAbits.TRISA6 = 0;
	LATAbits.LATA6 = 1;
	set_u2_pps(0);
	range_act = 0;
	
	ranges[0].setval_min = 0.05;
	ranges[0].setval_max = 10.0;
	ranges[0].range_mul = 10.0;

	ranges[1].setval_min = 0.05;
	ranges[1].setval_max = 85.0;
	ranges[1].range_mul = 100.0;

/*	cal[0].cal_gain = 26223.5;
	cal[0].cal_offset = -0.03;
	cal[1].cal_gain = 2655.27;
	cal[1].cal_offset = -0.035;
*/	
	ee_load_calib(cal);
	setval = 1.00;
	while (1)
		{
        keys_new.CHAR = key_read();
		if (keys_new.CHAR!=0) blink_on=30;
		keys.CHAR = keys.CHAR | ((keys_new.CHAR^keys_old.CHAR)&keys_new.CHAR);
		keys_old.CHAR = keys_new.CHAR;
		__delay_ms(50);
		
		if (keys.k2)
			{
			keys.k2=0;
			if (keys_new.k1)
				{
				if (range_act==0) range_act=1;
				else range_act=0;
				}
			else
				{
				if (setval_exp<5) setval_exp = setval_exp + 1;
				}
			}
		if (keys.k1)
			{
			keys.k1=0;
			if (setval_exp>0) setval_exp = setval_exp -1;
			}
		if (keys.k3)
			{
			keys.k3=0;
			setval = setval - (setval_inc_table[setval_exp]*ranges[range_act].range_mul);
			if (setval<ranges[range_act].setval_min) setval = ranges[range_act].setval_min;
			}
		if (keys.k4)
			{
			keys.k4=0;
			setval = setval + (setval_inc_table[setval_exp]*ranges[range_act].range_mul);
			if (setval> ranges[range_act].setval_max) setval = ranges[range_act].setval_max;
			}
		
		LATCbits.LATC5 = ~ LATCbits.LATC5;
		
		if (blink_on>0) blink_on--;
		display_set_value(setval,setval_exp,range_act,blink_on);
		
		setval_temp = setval + cal[range_act].cal_offset;
		setval_temp = setval_temp * cal[range_act].cal_gain;
		dacval = setval_temp;
		
		sprintf (tx_str,"d%6.6ld\n",(65536*4)-1-dacval);
		set_u2_pps(1);
		__delay_ms(1);
		tx2_chars(tx_str);
		__delay_ms(5);
		sprintf (tx_str,"r%d\n",range_act);
		__delay_ms(1);
		tx2_chars(tx_str);
		__delay_ms(5);

		
		serial_tasks();
		
		}
	}
