// &&& Snake Game &&&&&&&&&&&&&&&&&&&&&&&
/********************************************************
File name:		snakeGame.c
Author:			DTsebrii
Date:			08/AUG/2020
Modified:		None
Description:	The snake videogame
********************************************************/
// *** Libraries ****************************************
#include <stdlib.h>
#include <stdio.h>
#include <p18f45k22.h>

//*** Constants
#define TRUE	1
#define FALSE	0
#define TMR0FLAG INTCONbits.TMR0IF
#define HIGHBIT	0x20
#define LOWBIT	0xCA
#define FIRSTCHAN	0x01
#define ADCMASK	0x83
// Snake game defines
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
// Snake object constants
#define INITLEN		3  // Minimal lenght of a snake
#define MAXLENGHT	20
#define HEAD		0
// Variables to point snake's direction
#define UPMOVE		1
#define RIGHTMOVE	2
#define DOWNMOVE	3
#define LEFTMOVE	4
#define XCORLIM	35 // Maximum allowed value for X coordinate
#define YCORLIM	23  // Maximum allowed value for Y coordinate
#define YSCORE	2
#define XSCORE	29
#define NEXTGEAR	100
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//Needs to be rechecked
//!!!!!!!!!
#define BIGFOODLEN	4  // The leng of the premium food array
// Interrupt Routine
#define DOWN	PORTCbits.RC2
#define RIGHT	PORTDbits.RD1
#define UP		PORTDbits.RD0
#define LEFT	PORTCbits.RC3
#define TWOBITSON	0xC0

void ISR (); //calling the prototype for an interrupt function
#pragma code int_vector = 0x008
//*** int_vector*********************************************
/*Author:	CTalbot
Date: 		14/MAR/2020
Description: Calling the ISR function
Input:		None
Output:		None
*************************************************************/
void int_vector (void)
{
	_asm
		GOTO ISR
	_endasm
}//eo int_vector::
#pragma code

// Global Vars
/*** Block *********************
Structure to represent a single 
piece of snake. 
*******************************/
typedef struct snakeChank
{
	char xCor;
	char yCor;
	char visibility;
} chank_t;

/*** Snake *********************
Object to represent a snake cont-
rolled by a user.
********************************/
typedef struct snakeBody
{
	chank_t body[MAXLENGHT];
	char headDir;
	char cnt;
} snake_t;

chank_t food;
chank_t bigFood[BIGFOODLEN];
snake_t snake;
char cnt = 0;
char foodFlag = FALSE;  // flag to show a food presents
char bigFoodTimer = FALSE;
char direction = RIGHTMOVE;
char tempDir = 0;
char timeFlag = FALSE;  // To count every second
char timer = 0;  // To calculate one minute to send the data
char gameFlag = FALSE;
int score = 0;  // To watch a progress
int stageLimit = 99;



//*** Functions


/*** configInterrupt *******************************************************************************
Author:		DTsebrii
Date:		14/MAR/2020
Modified:	23/APR/2020
Description:Function to set the INTCON register parameters
Input:		None
Output:		None
*********************************************************************************************/
void interruptSetup (void)
{
	//Config TMR0 interrupt
	INTCON |= 0x20;
	INTCON2 &= 0xFB;
	//Global Interrupts
	INTCON |= TWOBITSON;  // 0xC0 
}// eo configInterrupt::

/*****************************************************************
Name:			resetTMR0
Author:			CTalbot
Date:			26/MAR/2020
Description: 	Reset timer0 and writes the preset value into the 
					count register. 
Input:			int preset, the preset count as a 2 byte value
Output:			None
*****************************************************************/
void resetTMR0 ( char lowBit, char highBit )
{
	TMR0FLAG =	FALSE;
	TMR0H =		highBit;
	TMR0L =		lowBit;
}  // eo resetTMR0:: 

/*** timerSetup *******************************************************************************
Author:		DTsebrii
Date:		12/MAR/2020
Description:Function to set the TMR0 parameters
Input:		char lowBit and highBit - TMRH and TMRL bits value
Output:		None
*********************************************************************************************/
void timerSetup ( char lowBit, char highBit )
{
	T0CON = 	0x93; //t0 enabled, 16 bit, clkout, 16 psv
	TMR0H = 	highBit;
	TMR0L = 	lowBit;
	TMR0FLAG = 	FALSE; //Turn back to the zero after event happens
}// eo timerSetup::

/*** oscSetup *******************************************************************************
Author:		DTsebrii
Date:		12/MAR/2020
Description:Function to set the oscilograph parameters
Input:		None
Output:		None
*********************************************************************************************/
void oscSetup (void)
{
	OSCCON = 0x72;  // Fosc = 16MHz
	OSCCON2 = 0x04;
	OSCTUNE = 0x80;
	while( OSCCONbits.HFIOFS!=1 );
}// eo oscSetup::


/*** configSerial *******************************************************************************
Author:		DTsebrii
Date:		12/MAR/2020
Description:Function to set the serial communication parameters
				to communicate with ESP8266
Input:		None
Output:		None
*********************************************************************************************/
void serialSetup()
{
	SPBRG= 		34;	// Sets it to 16MHZ and 115k Baudrate
	TXSTA1= 	0x26;  // BRGH = 1
	RCSTA1= 	0x90;
	BAUDCON1= 	0x48; // BRG16 = 1
}// eo configSerial::

/*** configPORTs *******************************************************************************
Author:			DTsebrii
Date:			12/MAR/2020
Modified:		DTsebrii, 30/MAR/2020
Description:	Function to set the ports parameters
Input:			None
Output:			None
*********************************************************************************************/
void portSetup()
{
	//PORTA
	ANSELA =	0x01;  //RA0 is ADC pin  
	LATA =		0x00;  // No input voltage
	TRISA = 	0xFF;  //All input
	//PORTB
	ANSELB = 	0x00;
	LATB = 		0x00;
	TRISB = 	0xFF;
	//PORTC
	ANSELC = 	0x00;
	LATC = 		0x00;
	TRISC = 	0xFF;
	//PORTD
	ANSELD = 	0x00;
	LATD = 		0x00;
	TRISD = 	0xFF;
	//PORTE
	ANSELE = 	0x00;  
	LATE = 		0x00;
	TRISE = 	0xFF;
}// eo configPORTs::

/*** adcSetup *******************************************************************************
Author:		DTsebrii
Date:		12/MAR/2020
Description:Function to set the ADC parameters
Input:		None
Output:		None
*********************************************************************************************/
void adcSetup (void)
{
	ADCON0|= 0x01;// ADC enable 
	ADCON1 = 0x00;
	ADCON2 = 0xA9;// 12 TAD right just F/8
}// eo adcSetup::


/*** initSnake ******************************************
Author:		DTsebrii
Date:		05/AUG/2020
Description:set the starting parameters for a snake object
Input:		snake_t snPtr - snake object
Return:		snake_t snPtr - snake object
********************************************************/
snake_t initSnake(snake_t snPtr, chank_t fdPtr)
{
	// Initialization of food
	fdPtr.visibility = TRUE;
	fdPtr.xCor = 0;
	fdPtr.yCor = 0;

	// initialization of a snake
	for ( snPtr.cnt = 0; snPtr.cnt < MAXLENGHT; snPtr.cnt++ )
	{
		if (snPtr.cnt == 0 )
		{
			snPtr.body[snPtr.cnt].xCor = XCORLIM/2;
			snPtr.body[snPtr.cnt].yCor = YCORLIM/2;
			snPtr.body[snPtr.cnt].visibility = TRUE;
		}
		else
		{
			if ( snPtr.cnt <= 3 )
			{
				snPtr.body[snPtr.cnt].visibility = TRUE;
			}
			else
				snPtr.body[snPtr.cnt].visibility = FALSE;
			snPtr.body[snPtr.cnt].xCor = snPtr.body[snPtr.cnt - 1 ].xCor - 1;  // Pattern for RIGHTMOVE movement pattern
			snPtr.body[snPtr.cnt].yCor = snPtr.body[snPtr.cnt - 1 ].yCor;
		}
	}
	snPtr.headDir = RIGHTMOVE;
	snPtr.cnt = FALSE;
	return snPtr;
}  // eo initSnake::

/*****************************************************************
Name:			sampADC
Author:			DTsebrii
Date:			23/APR/2019
Description: 	Read the ADC value
Input:			chID, char that represents ADC channel
Output:			ADRES - ADC result
*****************************************************************/
int sampADC( char chID )
{
	ADCON0&= ADCMASK;
	ADCON0|=  chID<<2;
	ADCON0bits.GO = TRUE;
	while( ADCON0bits.GO );
	return ADRES;
	
}  // eo sampADC::

/*****************************************************************
Name:			systemSetup
Author:			DTsebrii
Date:			23/APR/2019
Description: 	Call all initialize/ config functions
Input:			None
Output:			None
*****************************************************************/
void systemSetup()
{
	// Hardwere part
	oscSetup();  // 16MHZ
	portSetup();
	serialSetup();
	adcSetup();  
	timerSetup( LOWBIT, HIGHBIT );
	interruptSetup();
	
	// Software part
	isrand( sampADC( FIRSTCHAN ) );  // Planting a seed
	snake = initSnake( snake, food );
}  // systemSetup::

//*** Display related programms ****************************

/*** fieldCreator ******************************************
Author:		DTsebrii
Date:		06/AUG/2020
Description:Create a gameField
Input:		None
Return:		None
********************************************************/
void fieldCreator(void)
{
	char cnt = 0;
	printf("\033[=17h"); //640x480 Mono
	for (cnt = 0; cnt <= YCORLIM; cnt++)
	{
		if (cnt == 0)
		{
			//puts("\033[0;0H"); // Starting from a home position
			for (cnt = 0; cnt < XCORLIM; cnt++)
				printf("\033[0;%dH#", cnt);
			cnt = 0;
		}
		if (cnt == 1)
		{
			printf("\033[1;%dHScore:", (XCORLIM-12)); // 6 chars here + 5 chars for score + 1 border line
		}
		printf("\033[%d;0H#", cnt);
		printf("\033[%d;%dH#", cnt, XCORLIM);
		if (cnt == YCORLIM)
		{
			for (cnt = 0; cnt < XCORLIM; cnt++)
				printf("#");
		}
	}
	printf("\033[1;1");
} // eo fieldCreator::

/*** foodAppearance ******************************************
Author:		DTsebrii
Date:		06/AUG/2020
Description:Create a food object if required
Input:		snake_t snPtr - object of a snake
Return:		None
********************************************************/
char foodAppearance(snake_t snPtr, chank_t *food)
{
	char cnt = 0;
	while (foodFlag == FALSE)
	{
		food->yCor = rand() % (YCORLIM - 2)+3;  // Create an variable within the screen area
		if(food->yCor >= 3)
			food->xCor = rand() % (XCORLIM - 2) + 2;  // Create an variable within the screen area
		else
			food->xCor = rand() % (XCORLIM - 12) + 2;  // Create an variable within the screen area
		
		for (cnt = 0; cnt < MAXLENGHT; cnt++)
		{
			if (food->xCor == snPtr.body[cnt].xCor)
				food->xCor = rand() % (XCORLIM - 3)+2;
			if (food->yCor == snPtr.body[cnt].yCor)
				food->yCor = rand() % (YCORLIM - 3)+3;
		}
		food->visibility = TRUE;
		return TRUE;
	}
}  // eo foodAppearance::

/*** drawSnake ******************************************
Author:		DTsebrii
Date:		06/AUG/2020
Description:Draw a snake 
Input:		snake_t snPtr - snake object
Return:		None
********************************************************/
void drawSnake(snake_t snPtr, char dir)
{
	char cnt = 0;
	for (cnt = 0; cnt < MAXLENGHT; cnt++)
	{
		if (snPtr.body[cnt].visibility == TRUE)
		{
			if (cnt == 0)
			{
				switch (dir)
				{
				case RIGHTMOVE:
					printf("\033[%d;%dH>", snPtr.body[cnt].yCor, snPtr.body[cnt].xCor);
					break;
				case LEFTMOVE:
					printf("\033[%d;%dH<", snPtr.body[cnt].yCor, snPtr.body[cnt].xCor);
					break;
				case UPMOVE:
					printf("\033[%d;%dH^", snPtr.body[cnt].yCor, snPtr.body[cnt].xCor);
					break;
				case DOWNMOVE:
					printf("\033[%d;%dH%c", snPtr.body[cnt].yCor, snPtr.body[cnt].xCor, 25);
					break;
				}
				
			}
			else
				printf("\033[%d;%dH0", snPtr.body[cnt].yCor, snPtr.body[cnt].xCor);
		}
	}
} // eo drawSnake::

/*** deleteSnake ******************************************
Author:		DTsebrii
Date:		06/AUG/2020
Description:Delete a snake
Input:		snake_t snPtr - snake object
Return:		None
********************************************************/
void deleteSnake(snake_t snPtr)
{
	char cnt = 0;
	for (cnt = 0; cnt < MAXLENGHT; cnt++)
	{
		if (snPtr.body[cnt].visibility == TRUE)
		{
			if (cnt == 0)
				printf("\033[%d;%dH ", snPtr.body[cnt].yCor, snPtr.body[cnt].xCor);
			else
				printf("\033[%d;%dH ", snPtr.body[cnt].yCor, snPtr.body[cnt].xCor);
		}
	}
} // eo deleteSnake::

//*** Gamplay functions ************************************************************
/*** signDesign ******************************************
Author:		DTsebrii
Date:		05/AUG/2020
Description:Check the sign of a given number
Input:		int cor - number of a numeric representation 
				of a difference between head coordinabe
				and current object coordinate
Return:		-1 if number is negative
			+1 if number is positive
********************************************************/
char signDesign(int cor)
{
	return (cor > 0) ? 1 : -1;
}  // eo signDesing::

/*** movePattern ******************************************
Author:		DTsebrii
Date:		05/AUG/2020
Description:set the starting parameters for a snake object
Input:		snake_t snPtr - snake object
			char dir - chosen direction
Return:		snake_t snPtr - snake object
********************************************************/
void movePattern(char dir, snake_t* snPtr)
{
	for (snPtr->cnt = MAXLENGHT - 1; snPtr->cnt >= 1; snPtr->cnt--)
	{
		(*snPtr).body[snPtr->cnt].xCor = (*snPtr).body[snPtr->cnt - 1].xCor;
		(*snPtr).body[snPtr->cnt].yCor = (*snPtr).body[snPtr->cnt - 1].yCor;
	}
	switch (dir)
	{
		case UPMOVE:
			if ( (*snPtr).body[HEAD].yCor <= 2 )
				(*snPtr).body[HEAD].yCor = YCORLIM;
			else
				(*snPtr).body[HEAD].yCor--;
			break;
		case RIGHTMOVE:
			if ( (*snPtr).body[HEAD].xCor == XCORLIM - 1 )
				(*snPtr).body[HEAD].xCor = 2;
			else
				(*snPtr).body[HEAD].xCor++;
			break;
		case DOWNMOVE:
			if ( (*snPtr).body[HEAD].yCor == YCORLIM )
				(*snPtr).body[HEAD].yCor = 2;
			else
				(*snPtr).body[HEAD].yCor++;
			break;
		case LEFTMOVE:
			if ((*snPtr).body[HEAD].xCor == 2
				|| (((*snPtr).body[HEAD].xCor >= (XCORLIM - 12)) 
					 && ((*snPtr).body[HEAD].xCor == 2) 
					)
				)
				(*snPtr).body[HEAD].xCor = XCORLIM - 1;
			else
				(*snPtr).body[HEAD].xCor--;
			break;
		default:
			printf("ValueError. Wrong Input in movePattern!");
	}
}  // eo movePattern::

/*** foodHit ******************************************
Author:		DTsebrii
Date:		06/AUG/2020
Description:Check whatever snake got the food or not
Input:		snake_t snPtr - snake object
Return:		TRUE if hit happened
			FALSe if it does not 
********************************************************/
char foodHit(snake_t snPtr, chank_t *fdPtr)
{
	return (
			fdPtr->xCor == snPtr.body[HEAD].xCor 
			&& fdPtr->yCor == snPtr.body[HEAD].yCor
			&& fdPtr->visibility == TRUE
			) ? TRUE : FALSE;
} // eo deleteSnake::

/*** expandSnake ******************************************
Author:		DTsebrii
Date:		06/AUG/2020
Description:Increasing of a snake size
Input:		snake_t *snPtr - pointer to a snake object
Return:		None
********************************************************/
void expandSnake(snake_t *snPtr)
{
	for (snPtr->cnt = 0; snPtr->cnt < MAXLENGHT; snPtr->cnt++)
	{
		if (snPtr->body[snPtr->cnt].visibility == FALSE)
		{
			snPtr->body[snPtr->cnt].visibility = TRUE;
			break;
		}
	}
} // eo expandSnake::

/*** gameOverCheck ******************************************
Author:		DTsebrii
Date:		06/AUG/2020
Description:Increasing of a snake size
Input:		snake_t *snPtr - pointer to a snake object
Return:		TRUE if game is not over
			FALSE if game is done
********************************************************/
char gameOverCheck(snake_t* snPtr)
{
	char cnt = 0;
	for (snPtr->cnt = 0; snPtr->cnt < MAXLENGHT; snPtr->cnt++)
	{
		for (cnt = 0; cnt < MAXLENGHT; cnt++)
		{
			if (
				snPtr->body[snPtr->cnt].xCor == snPtr->body[cnt].xCor 
				&& snPtr->body[snPtr->cnt].yCor == snPtr->body[cnt].yCor  // Same position 
				&& cnt != snPtr->cnt  // Checking if this the same object or not
				&& snPtr->body[snPtr->cnt].visibility == TRUE  // Check only visible (already in-game) objects
				&& snPtr->body[cnt].visibility == TRUE
				)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
} // eo expandSnake::

/*** checkPushButtons ******************************************
Author:		DTsebrii
Date:		08/AUG/2020
Description:checking the PB's states
Input:		None
Return:		None
********************************************************/
void checkPushButtons(void)
{
		if( !DOWN )
			tempDir = DOWNMOVE;
		if( !RIGHT )
			tempDir = RIGHTMOVE;
		if( !LEFT )
			tempDir = LEFTMOVE;
		if( !UP )
			tempDir = UPMOVE;
		if (tempDir > 0 && tempDir < 5)
		{
			if ((tempDir%2) != (direction%2))
				direction = tempDir;
		}
}  // eo checkPushButtons::

/*** openning ******************************************
Author:		DTsebrii
Date:		08/AUG/2020
Description:Helps user to check the difficulty level 
Input:		char *ptr - pointer to a timer
Return:		None
********************************************************/
void opening (char *ptr)
{
	printf("\033[%d;%dHPRESS UP TO SELECT DIFFICULTY\n", (YCORLIM/2), (XCORLIM/2 - 13));
	printf("\033[%d;%dHAND SELECT TO CHOOSE IT\n", (YCORLIM/2+1), (XCORLIM/2 - 13));
	score = 0;
	printf("\033[0m");
	if( tempDir == UPMOVE )
	{
		( cnt >= 4 ) ? cnt = 1 : cnt++;
		switch (cnt)
		{
			case 1:
				printf("\033[%d;%dH\033[1;30;47m1. EASY  ", (YCORLIM/2 + 2), (XCORLIM/2 - 10));
				printf("\033[0m");
				break;
			case 2:
				printf("\033[%d;%dH\033[1;30;47m2. MEDIUM", (YCORLIM/2 + 2), (XCORLIM/2 - 10));
				printf("\033[0m");
				break;
			case 3:
				printf("\033[%d;%dH\033[1;30;47m3. HARD", (YCORLIM/2 + 2), (XCORLIM/2 - 10));
				printf("\033[0m");
				break;
			default:
				break;
			
		}
	}
	if( tempDir == RIGHTMOVE )
	{
		switch (cnt)
		{
			case 1:
				*ptr = 10;
				gameFlag = TRUE;
				break;
			case 2:
				*ptr = 6;
				gameFlag = TRUE;
				break;
			case 3:
				*ptr = 3;
				gameFlag = TRUE;
				break;
			default:
				break;
			
		}
	}
	tempDir = 0;
}  // eo opening::


//CHECKED
/*** bigFoodInit **********************************************************
Author:		DTsebrii
Date:		09/AUG/2020
Description:Set the big food paramaters
Input:		None
Output:		chank_t bgFdPtr - big food pointer
*********************************************************************************************/
void bigFoodInit ()
{
	char cnt = 0;
	foodAppearance(snake, &bigFood[0]);
	for(cnt = 1; cnt < BIGFOODLEN; cnt++)
	{
		if(cnt%2)
		{
			bigFood[cnt].xCor = bigFood[cnt-1].xCor + 1;
			bigFood[cnt].yCor = bigFood[cnt-1].yCor;
		}
		else
		{
			bigFood[cnt].xCor = bigFood[cnt-1].xCor - 1;
			bigFood[cnt].yCor = bigFood[cnt-1].yCor + 1;
		}
		bigFood[cnt].visibility = FALSE;
	}
}  // eo bigFoodInit::

/*** bigFoodDelete **********************************************************
Author:		DTsebrii
Date:		09/AUG/2020
Description:Delete Big Food from a game
Input:		None
Output:		chank_t bgFdPtr - big food
*********************************************************************************************/
void bigFoodDelete()
{
	char cnt = 0;
	for(cnt = BIGFOODLEN-1; cnt >= 0; cnt--)
	{
		printf("\033[%d;%dH ", bigFood[cnt].yCor, bigFood[cnt].xCor);
		bigFood[cnt].visibility = FALSE;
	}
}  // eo bigFoodDelete::

/*** drawBigFood **********************************************************
Author:		DTsebrii
Date:		09/AUG/2020
Description:Delete Big Food from a game
Input:		None
Output:		chank_t bgFdPtr - big food
*********************************************************************************************/
void drawBigFood()
{
	char cnt = 0;
	bigFoodInit();
	for(cnt = BIGFOODLEN-1; cnt >= 0; cnt--)
	{
		printf("\033[%d;%dH0", bigFood[cnt].yCor, bigFood[cnt].xCor);
		bigFood[cnt].visibility = TRUE;
	}
}  // eo bigFoodDelete::

/*** checkState ******************************************
Author:		DTsebrii
Date:		12/AUG/2020
Description:Check the in-game conditions 
Input:		None
Return:		None
********************************************************/
void checkState(void)
{
	if (foodHit(snake, &food))
	{
		foodFlag = FALSE;
		score++;
		printf("\033[1;%dH%05d", XSCORE, score);  // changes only if target was aimed
		foodFlag = foodAppearance(snake, &food);
		printf("\033[%d;%dH0", food.yCor, food.xCor);
		expandSnake(&snake);
	}
	// Check whatever draw a food or not
	if ( 
		score % 10 == FALSE 
		&& score > 0 
		&& bigFood[0].visibility == FALSE 
		)
	{
		score++;
		drawBigFood();
	}
	// Smoth difficulty rising
	if ( score >= stageLimit )
	{
		stageLimit+=NEXTGEAR;
		( timer <= 1 ) ? timer = 1 : timer--;
	} 
	// Check the collision with bigFood
	for ( cnt = 0; cnt < BIGFOODLEN; cnt++ )
	{
		if (
			snake.body[HEAD].xCor == bigFood[cnt].xCor
			&& snake.body[HEAD].yCor == bigFood[cnt].yCor
			&& bigFood[cnt].visibility == TRUE
			)
		{
			score+=15;
			printf("\033[1;%dH%05d", XSCORE, score);  // changes only if target was aimed
			bigFoodDelete();
			break;
		}
	}
	// Deleting BigFood if time is expired
	if ( bigFoodTimer >= XCORLIM )
	{
		bigFoodTimer = 0;
		printf("\033[1;2HTimer:00");
		bigFoodDelete();
	}
	if ( bigFood[0].visibility )  // Check whatever present bigFood or not
	{
		printf("\033[1;2HTimer:%02d", bigFoodTimer);
		bigFoodTimer++;
	}
}

#pragma interrupt ISR
/*** ISR *******************************************************************************
Author:		CTalbot
Date:		14/MAR/2020
Modifier:	DTsebrii, 23/APR/2020
Description:Reseting TMR0
Input:		None
Output:		None
*********************************************************************************************/
void ISR()
{
	if( TMR0FLAG )
	{
		resetTMR0( LOWBIT, HIGHBIT );
		timeFlag++;
	}
	INTCON |= TWOBITSON;
}  // eo ISR::

// &&& MAIN &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
void main()
{
	int adcSamp = 0;
	initializeSys();
	fieldCreator();
	bigFoodInit();
	timer = 30;
	cnt = 0;
	while( TRUE )
	{
		checkPushButtons();
		if( timeFlag >= timer )
		{
			timeFlag = FALSE;
			if( gameFlag == TRUE )
			{
				deleteSnake(snake);
				checkState();
				movePattern(direction, &snake);
				drawSnake(snake, direction);
				if ( !gameOverCheck(&snake) )
						gameFlag = FALSE;
			}
			if ( gameFlag == FALSE )
			{
				deleteSnake(snake); 
				opening(&timer);
				if ( gameFlag )
				{
					snake = initSnake(snake, food);
					printf("\033[%d;%dH\033[K ", (YCORLIM/2), (XCORLIM/2 - 15));
					printf("\033[%d;%dH#", (YCORLIM/2), XCORLIM);
					printf("\033[%d;%dH\033[K", (YCORLIM/2 + 1), (XCORLIM/2 - 15));
					printf("\033[%d;%dH#", (YCORLIM/2 + 1), XCORLIM);
					printf("\033[%d;%dH\033[K", (YCORLIM/2 + 2), (XCORLIM/2 - 10));
					printf("\033[%d;%dH#", (YCORLIM/2 + 2), XCORLIM);
					foodFlag = foodAppearance(snake, &food);
					printf("\033[%d;%dH0", food.yCor, food.xCor);
					printf("\033[1;%dH%05d", XSCORE, score);  // changes only if target was aimed
					stageLimit = 99; // Restarting difficulty with new game
				}
			}
		}	// eo timeFlag if
	} // eo while
}  // eo main
