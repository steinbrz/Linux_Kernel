// keyboard.c - Definitions of Functions for keyboard

#include "keyboard.h"
#include "i8259.h"
#include "terminal.h"
#include "lib.h"
#include "system_calls.h"
#include "scheduling.h"


#define ENABLE_KEYBOARD 0xF4
#define ENABLE_CLOCKLINE 0xAE
#define	INPUT_BUF_MASK 0x02
#define SET_SCAN_CODE 0xF0

//Variables used to check non-printable keys
uint32_t caps = 0;
uint32_t shift = 0;
uint32_t ctrl = 0;
volatile uint32_t alt = 0;
volatile uint32_t f1_key = 0;
volatile uint32_t f2_key = 0;
volatile uint32_t f3_key = 0;


//char buffer[SIZE_OF_BUFFER];  //Buffer for input/output to terminal
 //For index of buffer

// Array of terminal structs to keep data for each terminal separate
terminal_t terminals[3];



static unsigned char keyboard[128] = {   //Keys on the keyboard
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	0, 'q', 'w', 'e', 'r', 't', 'y', 'u' ,'i', 'o', 'p', '[', ']', '\n',
	0 /*L_CTRL*/, 'a', 's', 'd', 'f', 'g', 'h' ,'j', 'k', 'l', ';', '\'', 
	'`', 0 /*L_shift*/, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',
	'/', 0 /*R_shift*/, '*', 0 /*L_ALT*/, ' ', 0 /*CAPS lock*/
};

static unsigned char shift_keyboard[128] = {  //Keys on the keyboard if shift was pressed
	0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U' ,'I', 'O', 'P', '{', '}', '\n',
	0 /*L_CTRL*/, 'A', 'S', 'D', 'F', 'G', 'H' ,'J', 'K', 'L', ':', '\"', 
	'~', 0 /*L_shift*/, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>',
	'?', 0 /*R_shift*/, '*', 0 /*L_ALT*/, ' ', 0 /*CAPS lock*/
};

/* keyboard_init
 * DESCRIPTION - Initialize the keyboard by zeroing the terminal buffers,
 *				 writing the correct values to the keyboard registers, etc.
 * INPUT: none
 * OUTPUT: none
 * no return value
 * SIDE EFFECTS: After it's called, the OS can access the keyboard
 */
void keyboard_init (void)
{
	asm("cli");
	volatile uint8_t status_reg;
	status_reg = inb (KEYBOARD_CONTROLLER);  //Reading the status register
	while (status_reg & INPUT_BUF_MASK)    //Input Buffer is full (bit 1). Waiting for clearence
	{}
	outb(ENABLE_KEYBOARD, KEYBOARD_ENCODER);  //Enable Keyboard
	while (status_reg & INPUT_BUF_MASK)    //Input Buffer is full (bit 1). Waiting for clearence
	{}
	outb(ENABLE_CLOCKLINE, KEYBOARD_CONTROLLER);  //Enable Keyboard ClockLine
	clear_buffer();

	terminals[0].buf_idx = 0;
	terminals[1].buf_idx = 0;
	terminals[2].buf_idx = 0;
	clear_screen();
	key_flag = 0;
	write_flag = 1;
	terminals[0].allow_read = 0;
	terminals[1].allow_read = 0;
	terminals[2].allow_read = 0;
	//cursor_y = 0;
	enable_irq(1);		//Enabling keyboard interrupts
	asm("sti");
}

/* keyboard_interrupt_handler
 * DESCRIPTION - Handles key presses. Processes all modifier keys.
 * 				 Only writes if the output buffer is not full.
 * INPUTS: none
 * OUTPUTS: writes the characters to the active process's terminal
 * SIDE EFFECTS: Appends characters to terminal buffer, can toggle modifier
 *				 states (e.g. CAPS LOCK)
 */
void keyboard_interrupt_handler (void)
{
	asm("cli");
	uint8_t encoder_reg;
	uint8_t status_reg;
	keyPressed = 0;
	status_reg = inb (KEYBOARD_CONTROLLER);  //Reading the status register
	if (status_reg & 0x01)   //Output Buffer is full (bit 0) (keyboard ready to be read)
	{
		encoder_reg = inb(KEYBOARD_ENCODER);  //Read scan code

		if (encoder_reg == 0x2A || encoder_reg == 0x36)	//0x2A and 0x36: Shift pressed
			shift = 1;
		if (encoder_reg == 0xAA || encoder_reg == 0xB6)	//0xAA and 0xB6: Shift Let go
			shift = 0;

		if (encoder_reg == 0x1D)	//0x1D: Ctrl pressed
			ctrl = 1;
		if (encoder_reg == 0x9D)	//0x9D: Ctrl Let go
			ctrl = 0;
		if(encoder_reg == ALT_PRESSED)
			alt = 1;
		if(encoder_reg == ALT_RELEASED)
			alt = 0;
		

		// Handle terminal switching keys
		if(encoder_reg == F1_PRESSED)
			f1_key = 1;
		if(encoder_reg == F2_PRESSED)
			f2_key = 1;
		if(encoder_reg == F3_PRESSED)
			f3_key = 1;
		if(encoder_reg == F1_RELEASED)
			f1_key = 0;
		if(encoder_reg == F2_RELEASED)
			f2_key = 0;
		if(encoder_reg ==  F3_RELEASED)
			f3_key = 0;
			
		if(alt == 1){
			if(f1_key == 1){
				change_terminal(0);
				f1_key = 0;

				goto key_end;
				}
			if(f2_key == 1){
				send_eoi(1);
				change_terminal(1);
				f2_key = 0;
				if(terminals[1].shell_exists == 0){
					terminals[1].shell_exists = 1;
					send_eoi(1);
					sti();
					sys_execute((uint8_t*)"shell terminal");
				}
				goto key_end;
					
			}

				
			if(f3_key == 1){
				send_eoi(1);
				change_terminal(2);
				f3_key = 0;
				// If terminal 3 has not been started yet, execute shell
				if(terminals[2].shell_exists ==  0){
					terminals[2].shell_exists = 1;
					send_eoi(1);
					sti();
					sys_execute((uint8_t*)"shell terminal");
					
				}
				goto key_end;
			}
	}
		
	if (encoder_reg >= 0x80)	//Break codes (other than shift) are larger than 0x80
		goto key_end;

	if (encoder_reg == 0x3A)	//0x3A: Caps Lock pressed
		caps++;

	if (ctrl == 1 && keyboard[encoder_reg] == 'l')  //Clear Screen
	{
		terminal_clear();
		clear_buffer();
		goto key_end;
	}

	if (keyboard[encoder_reg] >= 97 && keyboard[encoder_reg] <= 122)  //Letters
	{
		if ((caps % 2 == 0 && shift == 0) || (caps % 2 == 1 && shift == 1))
			keyPressed = keyboard[encoder_reg];
		else
			keyPressed = (keyboard[encoder_reg] - 32);
	}

	else if (keyboard[encoder_reg] != 0)  //Other valid charecters (numbers and symbols)
	{
		if (shift == 0)
			keyPressed = keyboard[encoder_reg];
		else
			keyPressed = (shift_keyboard[encoder_reg]);
	}

	if (keyPressed == '\b' && terminals[get_active_terminal()].buf_idx != 0)   //Backspace and not at beginning of line
	{
		if (terminals[get_active_terminal()].buf_idx != 0)  
		{
			terminals[get_active_terminal()].keyboard_buffer[terminals[get_active_terminal()].buf_idx-1] = '\0';             //Removing charecter
			
			store_keyboard_location();
			keyboard_write(0, terminals[get_active_terminal()].keyboard_buffer, terminals[get_active_terminal()].buf_idx);   //and reprinting buffer
			
			update_keyboard_location();

			terminals[get_active_terminal()].buf_idx--;

			update_cursor(terminals[get_active_terminal()].buf_idx);
			//update_cursor(terminals[get_active_terminal()].buf_idx);
			key_flag++;
		}
	}

	else if (keyPressed == '\b')
		goto key_end;
	
	else if (keyPressed == '\n')
	{
		terminals[get_active_terminal()].keyboard_buffer[terminals[get_active_terminal()].buf_idx] = '\0';
		memcpy((void*)read_buffer,(void*) terminals[get_active_terminal()].keyboard_buffer, 1024);
		terminals[get_active_terminal()].allow_read = 1;

		clear_buffer();
		keyboard_write(0, (void *)&keyPressed, 1);
	}

		else if (keyPressed != 0 && terminals[get_active_terminal()].buf_idx < SIZE_OF_BUFFER-8)
		{
		terminals[get_active_terminal()].keyboard_buffer[terminals[get_active_terminal()].buf_idx] = keyPressed;
		terminals[get_active_terminal()].buf_idx++;
	
		key_flag++;
		store_keyboard_location();
		keyboard_write(0, terminals[get_active_terminal()].keyboard_buffer, terminals[get_active_terminal()].buf_idx);  //Writing buffer to terminal
		update_keyboard_location();
		//update_cursor(buf_ind);
		}
	}
key_end:
	send_eoi(1);
	asm("sti");
}

/* clear_buffer
 * DESCRIPTION: erases the active terminal's buffer, filling it with spaces
 *				and reseting the cursor position to the beginning
 * INPUTS: none
 * OUTPUTS: outputs space characters to every elements in the terminal's buffer
 * SIDE EFFECTS: See DESCRIPTION.
 */
void clear_buffer()
{
	int i, j;
	j = get_active_terminal();
	for (i=0; i<SIZE_OF_BUFFER; i++)
		buffer[j][i] = ' ';  //Fills buffer with spaces
	// Reset cursor position to beginning of buffer
	terminals[get_active_terminal()].buf_idx = 0;
}

/* keyboard_write
 * DESCRIPTION: echo key presses to the current terminal's video memory
 * INPUTS: int32_t fd - file descriptor index (not used)
		   const void* buf - contains the text to be copied
		   int32_t nbytes - number of characters to be copied
 * OUTPUTS: writes text to video memory using kputc
 * SIDE EFFECTS: calls kputc() to do actual write
 */
int32_t keyboard_write(int32_t fd, const void* buf, int32_t nbytes)
{
	int i;
	int length;
	uint8_t* temp;

	// Don't write an empty buffer
	if(buf == NULL){
		return -1;
	}

	int t;
	t = get_process_terminal();
	if(terminals[t].screen_y >= 24)
  		scroll();
	

	temp = (uint8_t*)buf;
	length = strlen((int8_t*)temp);
	for(i = 0; i < nbytes; i++){
		kputc(temp[i]);
	}

	return 0;
}
