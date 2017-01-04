/* terminal.c - Functions to interact with terminal
 * vim:ts=4 noexpandtab
 */
#include "terminal.h"
#include "lib.h"
#include "keyboard.h"
#include "system_calls.h"
#include "paging.h"

#define SIZE_OF_BUFFER 128			// max size of terminal text buffer

int TERMINAL_IS_OPEN = 0;			// Is a terminal open
uint32_t new_lines[NUM_ROWS];

volatile int active_terminal = 0;

terminal_t terminals[3];

static char* video_mem = (char *)VIDEO;

/* terminal_open
 * DESCRIPTION: make sure terminal is not already open for current process,
 *				then clear the terminal and allow access to it
 * INPUTS: filename - used for compatibility with other foo_open (not used)
 * OUTPUTS: none
 * RETURN VALUE: -1 - terminal already open; 0 - success
 * SIDE EFFECTS: clears buffer, enables terminal access for calling process
 */
int32_t terminal_open(const uint8_t* filename)
{
	/* check if terminal is already in use */
	if(TERMINAL_IS_OPEN)
		return -1;
	int i;
	for (i=0; i<NUM_ROWS; i++)
		new_lines[i] = 0;
	terminal_clear();		//clear terminal to start
	//update_cursor (0,0);
	return 0;
}

/* terminal_close
 * DESCRIPTION: closes the terminal for caller
 * INPUTS: fd - used for compatibility for other close function (not used)
 * OUTPUTS: TERMINAL_IS_OPEN = 0
 * RETURN VALUE: 0 - success; 1 - failure (terminal not open)
 * SIDE EFFECTS: none
 */
int32_t terminal_close(int32_t fd)
{	
	/* Check if terminal is open, close if it is.  */
	if(TERMINAL_IS_OPEN)
	{
		TERMINAL_IS_OPEN = 0;
		return 0;
	}
	return -1;
}

/* terminal_read
 * DESCRIPTION: read data from terminal buffer
 * INPUTS: fd - file descriptor index (not used)
 *		   buf - destination address
 *		   nbytes - number of bytes to be read
 * OUTPUTS: writes text to memory at location [buf]
 * SIDE EFFECTS: none
 * RETURN VALUE: number of bytes written 
 */
int32_t terminal_read (int32_t fd, uint8_t* buf, int32_t nbytes)
{
	asm("sti");
	int buflen=0;
	while(terminals[get_process_terminal()].allow_read == 0);
	asm("cli");
	buflen = strlen((int8_t*)terminals[active_terminal].keyboard_buffer);
	
	memcpy((void*)buf, (void*)terminals[active_terminal].keyboard_buffer, SIZE_OF_BUFFER);
	if (buflen >= SIZE_OF_BUFFER)
		buflen = SIZE_OF_BUFFER-1;
	buf[buflen] = '\n';
	buflen++;
	
	terminals[active_terminal].allow_read = 0;
	asm("sti");
	
	return buflen;
}

/* terminal_write function, writes data to the terminal
 * DESCRIPTION: writes data from buffer into terminal's screen
 * INPUTS: fd - file descriptor (not used)
 *		   buf - pointer to start of buffer to copy from
 *		   nbytes - number of bytes to be copied
 * OUTPUTS: displays text to screen of the current terminal
 * RETURN VALUE: 0 - success; -1 - failure
 * SIDE EFFECTS: none
 */
int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes)
{
	int i;
	int length;
	uint8_t* temp;

	asm("cli"); // Do not allow interrupts

	// don't try to write an empty buffer
	if(buf == NULL){
		return -1;
	}
	int t;
	t = get_process_terminal();
	if(terminals[t].screen_y >= 24)
  		scroll();
	temp = (uint8_t*)buf;
	length = strlen((int8_t*)temp);
	for(i = 0; i < SIZE_OF_BUFFER && i < nbytes; i++){
		putc(temp[i]);
	}

	/*if(strncmp("391OS", (int8_t*)buf, 5) == 0)
		update_cursor(0);*/

    asm("sti");

	return 0;
}

/* terminal_clear
 * DESCRIPTION: erases caller's terminal, resets cursor position to (0,0)
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: blank screen, cursor at top left position
 */
void terminal_clear()
{
	int i;
	clear();
	// erase all lines of previous written text
	for (i=0; i<NUM_ROWS; i++)
		new_lines[i] = 0;
	// reset cursor
	terminals[get_active_terminal()].screen_x = 0;
	terminals[get_active_terminal()].screen_y = 0;
}

/* update_cursor (currently not used, visual cursor is instead disabled)
 * DESCRIPTION: update the VGA blinking cursor for active terminal
 * INPUTS: offset - amount to move cursor
 * OUTPUTS: writes to vga registers
 * RETURN VALUES: none
 * SIDE EFFECTS: changes vga registers, cursor position
 */
void update_cursor(int offset)
{	int32_t i;
	i = get_active_terminal();
	uint8_t pos = offset + (terminals[i].screen_y * NUM_COLS) + terminals[i].screen_x;
	// cursor LOW port to vga
	outb(0x0F, 0x3D4);
	outb((uint8_t)(pos&0xFF), 0x3D5);
	// cursor HIGH port to vga
	outb(0x0E, 0x3D4);
	outb((uint8_t)((pos>>8)&0xFF), 0x3D5);
}

/* cursor_disable
 * DESCRIPTION: disables the hardware cursor by writing to vga registers
 * INPUTS: none
 * OUTPUTS: writes to the vga control registers
 * RETURN VALUES: none
 * SIDE EFFECTS: blinking cursor will not appear on screen
 */
void cursor_disable(void)
{
	outb(0x0A, 0x3D4);
	outb(0x20, 0x3D5);
}

/* scroll
 * DESCRIPTION: scrolls lines of text up the screen when the current terminal
 * 				line reaches the bottom.
 * INPUTS: none
 * OUTPUTS: shifts text in video memory up the equivalent of 1 line.
 * RETURN VALUES: none
 * SIDE EFFECTS: text moved off the top of the screen is gone.
 */
void scroll(void)
{
	int i, j, k, t;
	t = get_process_terminal();
	uint8_t character, attribute;
	if(terminals[t].screen_y == 24){

		for(j = 0; j < NUM_ROWS - 1; j++)
		{
			for(i = 0; i < NUM_COLS; i++)
			{
				character = *(uint8_t *)(terminals[t].vid_mem + ((NUM_COLS*(j + 1) + i) << 1));
        		attribute = *(uint8_t *)(terminals[t].vid_mem + ((NUM_COLS*(j + 1) + i) << 1) + 1);

        		*(uint8_t *)(terminals[t].vid_mem + ((NUM_COLS*(j) + i) << 1)) = character;
        		*(uint8_t *)(terminals[t].vid_mem + ((NUM_COLS*(j) + i) << 1) + 1) = attribute;
			}
		}

		for( k = 0; k < NUM_COLS; k++)
		{
			*(uint8_t *)(terminals[t].vid_mem + ((NUM_COLS*(terminals[t].screen_y) + k) << 1)) = ' ';
			*(uint8_t *)(terminals[t].vid_mem + ((NUM_COLS*(terminals[t].screen_y) + k) << 1) + 1) = ATTRIB;
		}
		terminals[t].screen_y--;
		terminals[t].screen_x = 0;

	}
}

/* get_active_terminal
 * DESCRIPTION: literally just what it says
 * INPUT: none
 * OUTPUT: active terminal number
 * RETURN VALUE: either 0, 1, or 2.
 */
int32_t get_active_terminal(void)
{
	return (active_terminal);
}

/* set_active_terminal
 * DESCRIPTION: sets active terminal to value passed in t
 * INPUTS: t - number of new active terminal
 * OUTPUTS: active_terminal becomes t
 * RETURN VALUE: none
 * SIDE EFFECTS: changes which terminal keyboard and display will use
 */
void set_active_terminal(int32_t t)
{
	active_terminal = t;
}

/* change_terminal
 * DESCRIPTION: changes which video memory is used, sets active_terminal to t,
 *				copies terminal's video memory to the screen
 * INPUTS: t - number of new terminal
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: erases screen and replaces with content from new terminal,
 *				 maps terminals video memory to global video memory location
 *				 sets active terminal to t
 */
void change_terminal(int32_t t)
{
	cli();
	if(get_active_terminal() == t){

		return;
	}
	reset_vid_map(get_active_terminal());
	memcpy((void*)terminals[get_active_terminal()].vid_mem, (void*)video_mem, 0x1000);
	memcpy((void*)video_mem, (void*)terminals[t].vid_mem, 0x1000);
	page_vid_map(t);
	active_terminal = t;
	sti();
}

/* clear_screen
 * DESCRIPTION: erase the screen's contents
 * INPUTS: none
 * OUTPUTS: sets terminal screen position to (0, 0)
 * RETURN VALUE: none
 * SIDE EFFECTS: clears the screen, pretty self explanatory
 */
void clear_screen(void){
	int i;
	i = get_active_terminal();

	clear();						// clear text from screen
	terminals[i].screen_x = 0;		// set x position to 0
	terminals[i].screen_y = 0;		// set y position to 0
}

/* store_keyboard_location
 * DESCRIPTION: copies terminal cursor position to keyboard location
 * INPUTS: none
 * OUTPUTS: changes two fields in terminal_t struct
 * RETURN VALUE: none
 * SIDE EFFECTS: none other than what is said in the description
 */
void store_keyboard_location(void){

	int32_t i;
	i = get_active_terminal();

	terminals[i].k_location_x = terminals[i].screen_x;
	terminals[i].k_location_y = terminals[i].screen_y;

}

/* update_keyboard_location
 * DESCRIPTION: exactly the opposite of store_keyboard_location
 * INPUTS: none
 * OUTPUTS: changes two fields in terminal_t struct
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void update_keyboard_location(void){
	int32_t i;
	i = get_active_terminal();
	terminals[i].screen_x = terminals[i].k_location_x;
	terminals[i].screen_y = terminals[i].k_location_y;
}

