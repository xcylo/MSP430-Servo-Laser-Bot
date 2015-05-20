#include <msp430.h>
#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>
#include <libemb/shell/shell.h>

/*******************************************
			PROTOTYPES & GLOBALS
*******************************************/
int shell_cmd_help(shell_cmd_args *args);
int shell_cmd_move(shell_cmd_args *args);
int shell_cmd_laser(shell_cmd_args *args); //xor with current laser output on/off
int shell_cmd_cont(shell_cmd_args *args);
int shell_cmd_setn(shell_cmd_args *args);
int shell_cmd_sped(shell_cmd_args *args);

unsigned int i = 0;											// num mask assumes P1.0 - P1.6

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
			.desc	= "Move to a exact position. Arguements: [x degree] [y degree]",
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
			.func = shell_cmd_cont
		},
		{
			.cmd  = "ysweep",
			.desc = "Patrol vertical axis. 180 degree sweep. Up->Down->Up",
			.func = shell_cmd_setn
		},
		{
			.cmd  = "speed",
			.desc = "set timer speed",
			.func = shell_cmd_sped
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
	if((x>180 || x<0) || (y>180 || y<0)) {
		cio_printf("Invalid position. Please input an integer between 0 and 180\n\r");
		return 0;
	} 

	TA0CCR1 = servodegree[x];
    TA1CCR1= servodegree[y];

	cio_print("Move Successful.\n\r");

	return 0;
}

int shell_cmd_laser(shell_cmd_args *args) {

	//XOR current state of laser output pin

	cio_printf("Laser toggled.\n\r");

	return 0;
}

int shell_cmd_xsweep(shell_cmd_args *args) {

        // Move forward toward the maximum position
        for (i = 0; i < 180; i++) {
            TA0CCR1 = servodegree[i];
            __delay_cycles(2000);
        }   
        // Move backward toward the manimum step value
        for (i = 180; i > 0; i--) {
            TA0CCR1 = sennnnnnnnnnnnnnnrvodegree[i];
            __delay_cycles(2000);
        }   

	cio_printf("Horizontal patrol complete.\n\r");

	return 0;
}

int shell_cmd_ysweep(shell_cmd_args *args) {

        // Move forward toward the maximum position
        for (i = 0; i < 180; i++) {
            TA1CCR1 = servodegree[i];
            __delay_cycles(2000);
        }   
        // Move backward toward the manimum step value
        for (i = 180; i > 0; i--) {
            TA1CCR1 = servodegree[i];
            __delay_cycles(2000);
        }   

	cio_printf("Vertical patrol complete.\n\r");

	return 0;
}

int shell_cmd_sped(shell_cmd_args *args) {

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
	WDTCTL  = WDTPW + WDTHOLD;								// Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL  = CALDCO_1MHZ;

	// BCSCTL3 = (BCSCTL3 & ~(BIT4+BIT5)) | LFXT1S_2;		// Disable xtal

	serial_init(9600);

	P1DIR = 0xFF;											// Set P1 as output
	P2DIR = BIT0;											// Set P2.0 as output
	P1OUT = 0;												// Display initially blank
	P2OUT = 0;

	//TA0CCR0 = 0x7FFF;										// Count limit 32767, 1 second

	TA0CCTL0 = 0x10;										// Enable counter interrupts

	TA0CTL = TASSEL_1 + MC_1;								// Timer A 0 with ACLK @ 32KHz, count UP

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

/*******************************************
			INTERRUPTS
*******************************************/
#pragma vector=TIMER0_A0_VECTOR
  __interrupt void Timer0_A0 (void) {

  	P1OUT ^= BIT0 | BIT6;
}
