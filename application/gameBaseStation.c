/*	Game Base Station:  Uses code from lab3_uart_test
 * 
 */
#include "bsp.h"
#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"
#include "hvzgame.h"
#include "msp430x22x4.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define RED 0x01
#define GREEN 0x02

char buffer[48];
int buffi;
char curId;
int state;           //Used to keep track of state in terminal (adding a player)
mrfiPacket_t pkt;
player_t *head;
char msg[48];

// Function Prototypes
void init_uart(void);
void uart_putc(char c);
void uart_puts(char *str);
void uart_clear_screen(void);
void init_radios(void);
void delay (unsigned int);

void main(void){
    int coun;
	WDTCTL = WDTPW + WDTHOLD;
	state= 0;
	curId= 1;
	P1DIR= 0x03;
	init_uart();
	init_radios();

	//Set up a UART terminal
	uart_puts("\nWelcome to Humans VS Zombies -- V0.1\r\n");
	//Enable interrupts to start the clock
	uart_puts("\n\nPlease enter a command or \"h\" for a list of commands:\r\n");
	buffi=0;
	__bis_SR_register(GIE);
	
	MRFI_SetRFPwr(0);
	
	//Waiting for game to start
	while(state!=3){}
	
	
	MRFI_SetRFPwr(2);

	//Send end time for a while so everyone receives it.
	for(coun=0;coun<200;coun++){
		MRFI_Transmit(&pkt , MRFI_TX_TYPE_FORCED);
		P1OUT= GREEN;
		delay(5000);
		P1OUT= 0;
		delay(5000);
	}
	state= 0;
	
	while(1){//Game running state
	}
}

/* Initialize radios */
void init_radios(void){
	unsigned char status;
	uint8_t address[] = {0x12,0x34,0xab,0xcd}; // Set filter address
	BSP_Init(); // Perform board-specific initialization
	
	/* Initialize minimal RF interface, wake up radio */
	MRFI_Init();
	MRFI_WakeUp();
	/* Currently the power level should be set to the default
	 * but we'll set it anyway to entry 3 (normal power 0dB gain)
	 */	
	MRFI_SetRFPwr(2);
			
	/* Attempt to turn on address filtering
	 *   If unsuccessful, turn on both LEDs and wait forever */
	status = MRFI_SetRxAddrFilter(address);	
	MRFI_EnableRxAddrFilter();
	if( status != 0) {
		P1OUT = 0x01 | 0x02;
		while(1);
	}
	
	/* Turn on the radio receiver */
	MRFI_RxOn();
	/* Initialize default packet addresses */
		pkt.frame[0] = 0;
		
		pkt.frame[1] = 0x12;		/* Destination */
		pkt.frame[2] = 0x34;		
		pkt.frame[3] = 0xab;
		pkt.frame[4] = 0xcd;
		
		pkt.frame[5] = 0x02;		/* Source */
		pkt.frame[6] = 0x00;
		pkt.frame[7] = 0x01;
		pkt.frame[8] = 0x02;
}

/* Initialize the UART for TX (9600, 8N1) */
/* Settings taken from TI UART demo */ 
void init_uart(void) {
	BCSCTL1 = CALBC1_1MHZ;        /* Set DCO for 1 MHz */
	DCOCTL  = CALDCO_1MHZ;
	P3SEL = 0x30;                 /* P3.4,5 = USCI_A0 TXD/RXD */
	UCA0CTL1 |= UCSSEL_2;         /* SMCLK */
	UCA0BR0 = 104;                /* 1MHz 9600 */
	UCA0BR1 = 0;                  /* 1MHz 9600 */
	UCA0MCTL = UCBRS0;            /* Modulation UCBRSx = 1 */
	UCA0CTL1 &= ~UCSWRST;         /* Initialize USCI state machine */
	IE2 |= UCA0RXIE;              // Enable USCI_A0 RX interrupt
	uart_clear_screen();
}

/* Transmit a single character over UART interface */
void uart_putc(char c) {
    while(!(IFG2 & UCA0TXIFG)); /* Wait for TX buffer to empty */
    UCA0TXBUF = c;				/* Transmit character */
}

/* Transmit a nul-terminated string over UART interface */
void uart_puts(char *str) {
	while (*str) {
		/* Replace newlines with \r\n carriage return */
		if(*str == '\n') { uart_putc('\r'); }
		uart_putc(*str++);
	}
}

/* Clear terminal screen using VT100 commands */
/* http://braun-home.net/michael/info/misc/VT100_commands.htm */
void uart_clear_screen(void) {
	uart_putc(0x1B);		/* Escape character */
 	uart_puts("[2J");		/* Clear screen */
 	uart_putc(0x1B);		/* Escape character */
 	uart_puts("[0;0H");		/* Move cursor to 0,0 */
}
//Delay function
void delay (unsigned int i)
{
	int j;
	for (j=0; j < i; j++) {
		__no_operation();
		__no_operation();
		__no_operation();
	}
}

void MRFI_RxCompleteISR(void) {
	char tmpId;
	player_t *tmpPlayer;
	mrfiPacket_t	packet;
	int i= 12;
	MRFI_Receive(&packet);
	if(packet.frame[9]=='K'){
		delay(5000);
		P1OUT ^= GREEN;
		delay(5000);
		P1OUT ^= GREEN;
		delay(5000);
		P1OUT ^= GREEN;
		delay(5000);
		P1OUT ^= GREEN;
		delay(5000);
		P1OUT ^= GREEN;
		delay(5000);
		P1OUT ^= GREEN;
		tmpId= packet.frame[10];
		tmpPlayer= head;
		//Parse Player GET
		while(tmpPlayer->next!=NULL && tmpPlayer->id!=tmpId){
			tmpPlayer= tmpPlayer->next;
		}
		
		while(packet.frame[i]!=';'){
			tmpPlayer->kills= (tmpPlayer->kills)*10 + (int)(packet.frame[i]-'0');
			i++;
		}
		i++;
		if(packet.frame[i]==';'){// they were a zombie
			i++;
			tmpPlayer->killedBy= packet.frame[i];
		} else {tmpPlayer->killedBy= 0;}
		sprintf(msg,"Received score for:%s\r\n",tmpPlayer->name);
		uart_puts(msg);
		memset(msg,0,sizeof msg);
	}
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
  player_t *t= NULL;
  player_t *t2=NULL;
  int i;

  while (!(IFG1 & UCA0TXIFG)); // USART0 TX buffer ready?
  if(buffi==48){
  	uart_puts("\nInput too long, start again (max size 48)\r\n");
  	uart_puts("\r\n");
  	memset(buffer, 0, sizeof buffer);
  	buffi= 0;
  	return;
  }
  buffer[buffi++]= UCA0RXBUF;
  if(!(buffer[buffi-1] == '\r')){
 	UCA0TXBUF = buffer[buffi-1]; // Echo (gives a nice terminal feel)

  } else { //Return pressed get command
  	uart_puts("\r\n");
  	if(state==1){ // Adding a player
  		sprintf(msg,"Player: %s\r\n",buffer);
  		uart_puts(msg);
  		memset(msg,0,sizeof msg);
  		uart_puts("Created and sending!\r\n");
  		
  		// Create a new player_t
  		t= malloc( sizeof(player_t));
  		t->id= curId++;
  		t->kills= 0;
  		t->killedBy= 0;
  		t->next= NULL;
  		strcpy(t->name,buffer);
  		if(head==NULL){
  			t->next= NULL;
  			head= t;
  		} else {
  			t->next= head;
  			head= t;
  		}
  		
  		//Send id to player device
  		sprintf(msg,"%c\r\n",t->id);
  		pkt.frame[0] = strlen(msg) + 8;
  		strcpy( (char *) &pkt.frame[9] , msg );
  		MRFI_Transmit(&pkt , MRFI_TX_TYPE_FORCED);
  		
  		//Player added successfully
  		memset(msg, 0, sizeof msg);
  		state= 0;
  	}
  	else if(state==2){   	//Time was entered move on to state 3
  		//Input buffer is the play time
		memset(msg, 0, sizeof msg);
		sprintf(msg,"Q%s",buffer);
		pkt.frame[0] = strlen(msg) + 8;
  		strcpy( (char *) &pkt.frame[9] , msg);
  		state= 3;
  	}
  	else if(buffer[0]=='h'){ //Help command
  		uart_puts("\nHELP: List of commands\r\n");
  		uart_puts("\nadd - adds a player, will prompt for user name\r\n");
  		uart_puts("\nend - ends the game and displays the top scores\r\n");
  		uart_puts("\nget - gets score of player nearby using wireless\r\n");
  		uart_puts("\nstart - start game, gather all players and use\n        once all players have been added\r\n");
  		uart_puts("\r\n");

  	} else if (buffer[0]=='a' && buffer[1]=='d' && buffer[2]=='d'){
  		//Add player
  		uart_puts("\nEnter a player name then press return to send to device\r\n");
  		state= 1;
  	} else if (buffer[0]=='e' && buffer[1]=='n' && buffer[2]=='d'){
  		//End game display scores
  		t= head;
  		uart_puts("\nFINAL SCORES:\r\n");
  		i= 1;
  		while(i<curId){
  			uart_puts("\r\n");
  			if(t->killedBy==0){//Still human
  				sprintf(msg,"%s - HUMAN\r\n",t->name);
  			} else {
  				t2= head;
  				while(t2->id != t->killedBy && t2->next!= NULL){t2= t2->next;}
  				sprintf(msg,"%s - ZOMBIE, Killed By: %s, Total Kills: %d\r\n",t->name, t2->name, t->kills);
  			}
  			uart_puts(msg);
  			memset(msg, 0, sizeof msg);
  			if(t->next!=NULL){t= t->next;}
  			i++;
  		}
  		
  	} else if (buffer[0]=='g' && buffer[1]=='e' && buffer[2]=='t'){
  		//Get score
		MRFI_SetRFPwr(0);
		sprintf(msg,"G\r\n");
		pkt.frame[0] = strlen(msg) + 8;
  		strcpy( (char *) &pkt.frame[9] , msg );
  		MRFI_Transmit(&pkt , MRFI_TX_TYPE_FORCED);
  		memset(msg, 0, sizeof msg);
  		MRFI_SetRFPwr(2);
  	} else if (buffer[0]=='s' && buffer[1]=='t' && buffer[2]=='a' && buffer[3]=='r' && buffer[4]=='t'){
  		//start game
  		uart_puts("\nEnter game length\r\n");
  		uart_puts("You must use the form Days;Hours;Min\r\n");
  		uart_puts("Where Days is a zero padded two digit integer\r\n");
  		uart_puts("Hours is a zero padded two digit integer <24\r\n");
  		uart_puts("Min is a zero padded two digit integer <60\r\n");
  		state= 2;
  	}

  	memset(buffer, 0, sizeof buffer);
  	buffi= 0;
  }
  
}

