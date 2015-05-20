/*
Aleksandr Dobrev
(November 2014)
----------------

____Program Summary____

SCREEN shell command:
sudo screen /dev/ttyUSB0 9600

PINS:
>1.1 & 1.2 = UART
>1.3 = button 
>1.6 = x servo pwm
>2.2 = y servo pwm
>2.3 = laser
*/
/*
Xservo:
Degree to Timer Correspondence
0  : 700
90 : 1600
180: 2500 

Yservo: 
Degree to Timer Correspondence
0  : 600
90 : 1500
180: 2400 
*/

//#include <msp430.h>
#include "msp430g2553.h"
#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>
#include <libemb/shell/shell.h>

#define CPU_CLOCK       1000000 	
#define PWM_FREQUENCY   50 	//pwm frequency	 
#define MIN_DUTYCYCLE   600 // The minimum duty cycle of servo
#define MAX_DUTYCYCLE   2500 // The maximum duty cycle

 
/*******************************************
			PROTOTYPES & GLOBALS
*******************************************/
int shell_cmd_help(shell_cmd_args *args);
int shell_cmd_move(shell_cmd_args *args);
int shell_cmd_laser(shell_cmd_args *args); //xor with current laser output on/off
int shell_cmd_xsweep(shell_cmd_args *args);
int shell_cmd_ysweep(shell_cmd_args *args);
int shell_cmd_blank(shell_cmd_args *args); //blank function

unsigned int i = 0;	
unsigned int PWM_PRD     = (CPU_CLOCK / PWM_FREQUENCY);  // PWM Period = clock/freq    
unsigned int cntr = 0; //global counter value for servo movements
unsigned int tempc = 0;

/*******************************************
			SHELL COMMANDS STRUCT
*******************************************/
shell_cmds my_shell_cmds = {
	.count 	= 6,
	.cmds	= {
		{
			.cmd  = "help",
			.desc	= "list available commands",
			.func	= shell_cmd_help
		},
		{
			.cmd  = "move",
			.desc	= "Move to a exact position. Arguements: [x val] [y val]",
			.func	= shell_cmd_move
		},
		{
			.cmd  = "laser",
			.desc = "Enable/Disable Laser",
			.func = shell_cmd_laser
		},
		{
			.cmd  = "xsweep",
			.desc = "Patrol horizontal axis. 180 degree sweep. Left->Right->Left",
			.func = shell_cmd_xsweep
		},
		{
			.cmd  = "ysweep",
			.desc = "Patrol vertical axis. 180 degree sweep. Up->Down->Up",
			.func = shell_cmd_ysweep
		},
		{
			.cmd  = "blank",
			.desc = "blank function",
			.func = shell_cmd_blank
		}
	}
};

/*******************************************
			SHELL CALLBACK HANDLERS
*******************************************/
int shell_cmd_help(shell_cmd_args *args) {
	int k;

	for(k = 0; k < my_shell_cmds.count; k++) {
		cio_printf("%s: %s\n\r", my_shell_cmds.cmds[k].cmd, my_shell_cmds.cmds[k].desc);
	}

	return 0;
}

int shell_cmd_move(shell_cmd_args *args) {
	unsigned int x = shell_parse_int(args->args[0].val); //parse x cordinate
	unsigned int y = shell_parse_int(args->args[1].val); //parse y cordinate


	//See if a valid degree is choosen
	if((x>MAX_DUTYCYCLE || x<MIN_DUTYCYCLE) || (y>(MAX_DUTYCYCLE/2) || y<MIN_DUTYCYCLE)) {
		//cio_printf("Invalid position. Please input an integer between 600 and 2600\n\r");
		//return 0;
	} 
	TA0CCR1 = x;
    TA1CCR1 = y;

	cio_print("Move Successful.\n\r");

	return 0;
}

int shell_cmd_laser(shell_cmd_args *args) {
	//XOR current state of laser output pin
	//when low acts as ground to complete circuit
	//If 2.3 DIR is 0 then laser is on

	P2DIR ^= BIT3; //toggle

	if(~(P2DIR & BIT3)){
		cio_printf("Laser disabled.\n\r");
	} else{
		cio_printf("Laser enabled.\n\r");
	}
	__delay_cycles(50);
	return 0;
}

int shell_cmd_xsweep(shell_cmd_args *args) {

        // Move forward toward the maximum position
        for (cntr = MIN_DUTYCYCLE; cntr < MAX_DUTYCYCLE; cntr++) {
            TA0CCR1 = cntr;
            __delay_cycles(2000);
        }   
        // Move backward toward the manimum step value
        for (cntr = MAX_DUTYCYCLE; cntr > MIN_DUTYCYCLE; cntr--) {
            TA0CCR1 = cntr;
            __delay_cycles(2000);
        }   

	cio_printf("Horizontal patrol complete.\n\r");

	return 0;
}

int shell_cmd_ysweep(shell_cmd_args *args) {

        // Move forward toward the maximum position
        for (cntr = MIN_DUTYCYCLE; cntr < (MAX_DUTYCYCLE)/2; cntr++) {
            TA1CCR1 = cntr;
            __delay_cycles(2000);
        }   
        // Move backward toward the manimum step value
        for (cntr = ((MAX_DUTYCYCLE)/2); cntr > MIN_DUTYCYCLE; cntr--) {
            TA1CCR1 = cntr;
            __delay_cycles(2000);
        }   

	cio_printf("Vertical patrol complete.\n\r");

	return 0;
}

int shell_cmd_blank(shell_cmd_args *args) {
        // Move forward toward the maximum position
        for (cntr = MIN_DUTYCYCLE; cntr < (MAX_DUTYCYCLE)/2; cntr++) {
            TA1CCR1 = cntr;
            TA0CCR1 = cntr;
            __delay_cycles(2000);
        }   
        // Move backward toward the manimum step value
        for (cntr = (MAX_DUTYCYCLE)/2; cntr > MIN_DUTYCYCLE; cntr--) {
            TA1CCR1 = cntr;
            TA0CCR1 = cntr;
            __delay_cycles(2000);
        }  


	//BLANK FUNCTION
	return 0;
}

int shell_process(char *cmd_line) {
     return shell_process_cmds(&my_shell_cmds, cmd_line);
}

/*******************************************
			INITIALIZE
*******************************************/
void main(void) {


   	 //Basic prep
    WDTCTL  = WDTPW + WDTHOLD;     //Stop Watchdog TImer
    BCSCTL1 = CALBC1_1MHZ;         //Clocks
    DCOCTL  = CALDCO_1MHZ;
    P1REN = BIT3; //turn on pull-resistor on 0b0001000
	P1OUT |= BIT3; //powers pin3 (button)
	P1IE = BIT3;	//enables p1 interrupt on pin 1.3
	P1IES |= BIT3;	//sets interrupt on high to low
	P1IFG &= ~BIT3;	//clears interrupt flag

		//Prep 1st PWM channel  (X AXIS)
    TA0CCTL1 = OUTMOD_7;            // TA0CCR1 reset/set
    TA0CTL  = TASSEL_2 + MC_1;     // SMCLK + upmode
    //upmode(MC_1): the Timer repeatedly counts from 0 to the value set in register TACCR0)
    TACCR0  = PWM_PRD-1;        //set pwm period
    TACCR1  = 0;                   //set TA0CCR1 PWM Duty Cycle
    P1DIR |= BIT6; //Pin 1.6 set as output
    P1SEL |= BIT6; //Set Pin 1.6 for pwm use

  	  //Prep 2nd PWM channel (Y AXIS)
    TA1CCTL1    = OUTMOD_7; //set/reset mode
    TA1CTL     = TASSEL_2 + MC_1; //SMCLK + upmode
    TA1CCR0     = PWM_PRD-1; //set pwm period
    TA1CCR1     = 0;   //set TA0CCR1_ pwm duty cycle
    P2DIR       |= BIT2 | BIT3;  //set as output
    P2OUT		|= BIT2;
    P2SEL       |= BIT2;  //Set pin 2.2 for pwm use

	serial_init(9600); // set serial clockrate

	__enable_interrupt();									// Enable global interrupts

/*******************************************
			MAIN LOOP
*******************************************/
	for(;;) {
		int j = 0;											// Char array counter
		char cmd_line[255] = {0};							// Init empty array

		cio_print((char *) "$ ");							// Display prompt
		char c = cio_getc();								// Wait for a character
		while(c != '\r') {									// until return sent then ...
			if(c == 0x08) {									//   was it the delete key?
				if(j != 0) {								//   cursor NOT at start?
					cmd_line[--j] = 0; cio_printc(0x08); cio_printc(' '); cio_printc(0x08);
				}											//   delete key logic
			} else {										// otherwise ...
				cmd_line[j++] = c; cio_printc(c);			//   echo received char
			}
			c = cio_getc();									// Wait for another
		}

		cio_print((char *) "\n\n\r");						// Delimit command result

		switch(shell_process(cmd_line))						// Execute specified shell command
		{													// and handle any errors
			case SHELL_PROCESS_ERR_CMD_UNKN:
				cio_print((char *) "ERROR, unknown command given\n\r");
				break;
			case SHELL_PROCESS_ERR_ARGS_LEN:
				cio_print((char *) "ERROR, an arguement is too lengthy\n\r");
				break;
			case SHELL_PROCESS_ERR_ARGS_MAX:
				cio_print((char *) "ERROR, too many arguements given\n\r");
				break;
			default:
				break;
		}

		cio_print((char *) "\n");							// Delimit before prompt
	}
}

/*
Port 1 Interrupt (used for 1.3 switch)
*/
#pragma vector=PORT1_VECTOR
  __interrupt void LasTog (void) {
/* DEBUG STUFF
  	tempc += 100;
  	TA0CCR1 = tempc;
  	cio_printf("Duty: %u",tempc);
*/

	P2OUT ^= BIT3; //toggle laser

	__delay_cycles(50);

	while(!(BIT3 & P1IN)){} //solves deboucing issue
	__delay_cycles(40000);

	P1IFG &= ~BIT3; //clears interrupt flag
	}

