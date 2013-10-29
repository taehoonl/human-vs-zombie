#ifndef HVZGAME_H_
#define HVZGAME_H_

#include "msp430x22x4.h"

#ifndef NULL
#define NULL 0
#endif



//#extern unsigned int testvariable
//A variable

typedef struct player{
	char id;
	char killedBy;
	unsigned int kills;
	char name[48];
	struct player *next;
}player_t;
//A struct

//void functionthing(void);
//A function


#endif /*HVZGAME_H_*/
