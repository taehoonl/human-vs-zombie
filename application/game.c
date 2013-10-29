/*------------------------------------------------------------
 * Core Humans vs. Zombies game application
 * 		Initializes variables and game state
 *----------------------------------------------------------*/

#include "bsp.h"
#include "mrfi.h"
#include "radios/family1/mrfi_spi.h"
#include "msp430x22x4.h"
#include "hvzgame.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define RED 0x01
#define GREEN 0x02

// Variables
player_t thisPlayer;
unsigned int initPlayer;
unsigned int startGame;
unsigned int human;
unsigned int counter; //Variable used to count
unsigned int play;    //Time is not up

mrfiPacket_t zProbe;  //The Zombie probe packet
mrfiPacket_t zAttack;  //The Zombie attack packet
mrfiPacket_t hNear;  //The Human near packet
mrfiPacket_t hDead;  //The Human dead packet
mrfiPacket_t hStun;  // The Human stun packet
mrfiPacket_t gScore; // report the score back to the 

unsigned int endTime; //End time stored in min
//Current time
unsigned int msec;
unsigned int sec; 
unsigned int min;
unsigned int hour;
unsigned int day;

// Function prototype defs
void delay(unsigned int);
void gameInit(void);
void packetsInit(void);
void zombInit(void);

/* Main:
 * Should:
 * 		Set score to zero.
 * 		Initialize radio state.
 * 		Initialize any interrupts.
 * 		Begin as human.
*/

void main(void){
	int j;
	WDTCTL = WDTPW + WDTHOLD; /* turn off watchdog */
	// Initialize radios, game and interrupts
	gameInit();
	//GREEN
	// wait to receive an ID #, player
	while(initPlayer){
		P1OUT ^= RED;
		delay(50000);
	}
	packetsInit();
	P1OUT =GREEN + 0x04;
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
	P1OUT = RED + 0x04;
	
	// wait to receive start game signal 
	while(startGame){
		P1OUT ^= RED;
		delay(50000); 
	}
	__bic_SR_register(GIE);
	
	P1OUT = RED + GREEN + 0x04;
	
	for(j=0; j< 450; j++){
			P1OUT ^= RED + GREEN;
			delay(5000);
		}
	
	__bis_SR_register(GIE);
	
  	// Human loop do nothing (while safe from zombies)
    counter = 0;
    play= 1;
    P1OUT = GREEN + 0x04;
    
    // If human, press the button for 15 seconds to switch to a zombie
  	while(human && play){  // Polling for suicide button
  		if (counter < 255){ //prevent the counter to roll back to 0
        	counter++;
        	delay(10000);
      	}
      	if ((~P1IN & 0x04) == 0x00){
      	   counter = 0; //reset the counter back to 0 when button is not pressed
     	}
 
	    if (counter > 100){
	         human = 0;
	         thisPlayer.killedBy = thisPlayer.id;
	         P1OUT = RED + 0x04;
	         break;
		}
  	}
  	
  	// Zombie
  	if(play){
  		zombInit();
  	}
  	
	// Decrease power output to -30dB
	MRFI_SetRFPwr(0);
	
	// Zombie Loop send probe signals
	while(play){		
		MRFI_Transmit(&zProbe , MRFI_TX_TYPE_FORCED);
		delay(15000);
		delay(15000);
	}
	P1OUT= RED + GREEN;
	while(1){
			P1OUT ^= RED + GREEN;
			delay(50000);
	}
  	
}

void gameInit(void){
	unsigned char status;
	uint8_t address[] = {0x12,0x34,0xab,0xcd}; // Set filter address
	initPlayer = 1;
	startGame = 1;
	human= 1;
	thisPlayer.killedBy = 0;
		
	// Radio setup
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
		
	/* Red and green LEDs are output, green starts on as default state
	 * is human. */
	P1DIR = RED+GREEN;
	P1OUT = GREEN;
	
	
	// Port 1 Setup
	P1REN |= 0x04;			// Set P1.2 to have resistor
	P1DIR &= ~0x04;			// Set P1.2 to input direction
	P1OUT |= 0x04;			// Set P1.2 output 
  	//Timer B - Real-time keeper
	TBCTL = TASSEL_2 + MC_1;  // Use SMCLK with no division and count up
	TBCCR0 = 1000;  // Count to 1000 (at 1 MHz this creates a period of 1ms)
	TBCCTL0 = CCIE;		// Enables interrupts for timer B
  	
}

void packetsInit(void){
	// zProbe packet
		char msg[] = "P\r\n";
		char msgb[8];
		char msgc[] = "N\r\n";
		char msge[] = "S\r\n";
		
		sprintf(msgb, "A%c\r\n", thisPlayer.id);
		
		zProbe.frame[0] = strlen(msg) + 8;	/* Length includes 8-byte address header */
		
		/* Next 8 bytes are addresses, 4 each for source and dest. */
		zProbe.frame[1] = 0x12;		/* Destination */
		zProbe.frame[2] = 0x34;		
		zProbe.frame[3] = 0xab;
		zProbe.frame[4] = 0xcd;
		
		zProbe.frame[5] = 0x02;		/* Source */
		zProbe.frame[6] = 0x00;
		zProbe.frame[7] = 0x01;
		zProbe.frame[8] = 0x02;
		// Data
		strcpy( (char *) &zProbe.frame[9] , msg );
		
	// zAttack packet
		zAttack.frame[0] = strlen(msgb) + 8;	/* Length includes 8-byte address header */
		
		/* Next 8 bytes are addresses, 4 each for source and dest. */
		zAttack.frame[1] = 0x12;		/* Destination */
		zAttack.frame[2] = 0x34;		// Human
		zAttack.frame[3] = 0xab;
		zAttack.frame[4] = 0xcd;
		
		zAttack.frame[5] = 0x02;		/* Source */
		zAttack.frame[6] = 0x00;
		zAttack.frame[7] = 0x01;
		zAttack.frame[8] = 0x02;
		// Data
		strcpy( (char *) &zAttack.frame[9] , msgb );

				
	// hStun packet
		hStun.frame[0] = strlen(msge) + 8 ;
		
		/* Next 8 bytes are addresses, 4 each for source and dest. */
		hStun.frame[1] = 0x12;		/* Destination */
		hStun.frame[2] = 0x34;		// Zombie
		hStun.frame[3] = 0xab;
		hStun.frame[4] = 0xcd;
		
		hStun.frame[5] = 0x02;		/* Source */
		hStun.frame[6] = 0x00;
		hStun.frame[7] = 0x01;
		hStun.frame[8] = 0x02;
		// Data
		strcpy( (char *) &hStun.frame[9] , msge );
		
	// hNear packet
		hNear.frame[0] = strlen(msgc) + 8 ;
		
		/* Next 8 bytes are addresses, 4 each for source and dest. */
		hNear.frame[1] = 0x12;		/* Destination */
		hNear.frame[2] = 0x34;		// Zombie
		hNear.frame[3] = 0xab;
		hNear.frame[4] = 0xcd;
		
		hNear.frame[5] = 0x02;		/* Source */
		hNear.frame[6] = 0x00;
		hNear.frame[7] = 0x01;
		hNear.frame[8] = 0x02;
		// Data
		strcpy( (char *) &hNear.frame[9] , msgc );
		
	// hDead packet

		/* Next 8 bytes are addresses, 4 each for source and dest. */
		hDead.frame[1] = 0x12;		/* Destination */
		hDead.frame[2] = 0x34;		// Zombie
		hDead.frame[3] = 0xab;
		hDead.frame[4] = 0xcd;
		
		hDead.frame[5] = 0x02;		/* Source */
		hDead.frame[6] = 0x00;
		hDead.frame[7] = 0x01;
		hDead.frame[8] = 0x02;
		// Data
		
	// gScore packet

		/* Next 8 bytes are addresses, 4 each for source and dest. */
		gScore.frame[1] = 0x12;		/* Destination */
		gScore.frame[2] = 0x34;		
		gScore.frame[3] = 0xab;
		gScore.frame[4] = 0xcd;
		
		gScore.frame[5] = 0x02;		/* Source */
		gScore.frame[6] = 0x00;
		gScore.frame[7] = 0x01;
		gScore.frame[8] = 0x02;
		// Data
		
}

void zombInit(void){
	int j;
	__bic_SR_register(GIE);
	// Toggles Red and Green LEDs to indicate a switch from a human to a zombie
	// Takes about 15 seconds
	P1OUT = RED + 0x04;
	for(j=0; j< 200; j++){
			P1OUT ^= RED + GREEN;
			delay(5000);
		}
		
	// Red LED to indicate it is a zombie
	P1OUT = RED + 0x04;
	__bis_SR_register(GIE);
}

void MRFI_RxCompleteISR(void) {
/* Read the received data packet */
	int i;
	char msgd[8];
	char msgScore[8];
	mrfiPacket_t packet;
	MRFI_Receive(&packet);
	
	if(initPlayer){
		thisPlayer.id = packet.frame[9];
		thisPlayer.kills = 0;
		initPlayer = 0;
	}
	
	else if(startGame){
		if(packet.frame[9]=='Q'){
			endTime= 24*60*((int)(10*(packet.frame[10] - '0') + packet.frame[11] - '0'));
			endTime+= 60*((int)(10*(packet.frame[13] - '0') + packet.frame[14] - '0'));
			endTime+= (int)(10*(packet.frame[16] - '0') + (packet.frame[17] - '0'));
			min= 0;
			sec= 0;
			msec= 0;
			day= 0;
			hour= 0;
			startGame = 0;
		}
	}
	
	else if(packet.frame[9] == 'G'){
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
		if(human){
			sprintf(msgScore, "K%c;%i;\r\n",thisPlayer.id, thisPlayer.kills);
			gScore.frame[0] = strlen(msgScore) + 8 ;
			strcpy( (char *) &gScore.frame[9] , msgScore );
			MRFI_Transmit(&gScore , MRFI_TX_TYPE_FORCED);
		}
		else if(!human){
			sprintf(msgScore, "K%c;%i;;%c;", thisPlayer.id, thisPlayer.kills, thisPlayer.killedBy);
			gScore.frame[0] = strlen(msgScore) + 8 ;
			strcpy( (char *) &gScore.frame[9] , msgScore );
			MRFI_Transmit(&gScore , MRFI_TX_TYPE_FORCED);
		}
	}	
		
	else if(human){// You are a human	
		// Response to zombie probing
		if(packet.frame[9]== 'P'){
			//Send back a NEAR signal then blink RED
			MRFI_Transmit(&hNear , MRFI_TX_TYPE_FORCED);
			
			P1OUT = RED + GREEN + 0x04;
			i= 0;
			counter= 0;
			
			while(i < 100){
				//prevent the counter to roll back to 0
        		delay(1000);
        		i++;
        		counter++;
      			if ((~P1IN & 0x04) == 0x00){
      	  			 counter = 0; //reset the counter back to 0 when button is not pressed
     			} 
	      		if (counter > 10){
	        	 	MRFI_Transmit(&hStun , MRFI_TX_TYPE_FORCED);
	         		break;
				}
  			}
  			P1OUT = GREEN + 0x04;
		}
		
		// Response to zombie attack
		if (packet.frame[9]== 'A'){
			thisPlayer.killedBy = packet.frame[10];
			
			// set up the hDead packet with the attacker's id
			sprintf(msgd, "D%c\r\n", thisPlayer.killedBy);
			hDead.frame[0] = strlen(msgd) + 8 ;
			strcpy( (char *) &hDead.frame[9] , msgd );
			
			//Immediately send back a DEAD signal
			MRFI_Transmit(&hDead , MRFI_TX_TYPE_FORCED);
			//Change state to zombie
			human = 0;
		}
		
	}
	else if(!human){  //You are a zombie
		unsigned int zombieCounter;
		// Response to human near
		if(packet.frame[9]== 'N'){
			P1OUT = RED + GREEN + 0x04;
			//Blink GREEN and enable attacking
			i= 0;
			zombieCounter= 0;
			while(i < 100){
				//prevent the counter to roll back to 0
        		delay(1000);
        		i++;
        		zombieCounter++;
      			if ((~P1IN & 0x04) == 0x00){
      	  			 zombieCounter = 0; //reset the counter back to 0 when button is not pressed
     			} 
	      		if (zombieCounter > 10){
	        	 	MRFI_Transmit(&zAttack , MRFI_TX_TYPE_FORCED);
	        	 	delay(50000);
	         		break;
				}
  			}

  			P1OUT = RED + 0x04;
		}
		
		// Response to human kill
		if (packet.frame[9]== 'D' && packet.frame[10] == thisPlayer.id){
			int k = 0;
			//Increment kills and display a success light
			thisPlayer.kills++;
			P1OUT = RED + GREEN;
			for(k=0; k< 200; k++){
				P1OUT ^= RED + GREEN;
				delay(5000);
			}
			P1OUT = RED + 0x04;
		}
		
		// Response to human stunning
		if (packet.frame[9] == 'S'){
			zombInit();
		}
	}
}

void delay (unsigned int i)
{
	int j;
	for (j=0; j < i; j++) {
		__no_operation();
		__no_operation();
		__no_operation();
	}
}

#pragma vector=TIMERB0_VECTOR
__interrupt void my_handler2 (void)
{
	msec++;
	if (msec >= 1000){
		sec++;
		msec = 0;
	}
	if (sec >=60){
		min++;
		sec=0;
	}
	if (min >= 60){
		hour++;
		min =0;
	}
	if (hour>=24){
		day++;
		hour= 0;
	}
	if((day*24*60+hour*60+min)>=endTime){
		play=0;
	}
	
}
