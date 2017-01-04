/* terminal.h - Functions to interact with terminal
 * vim:ts=4 noexpandtab
 */

#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"

#define SIZE_OF_BUFFER 128


volatile char keyPressed;  //Pressed key
volatile int key_flag;

//extern int32_t active_terminal;

typedef struct terminal_t{
	int32_t screen_x;
	int32_t screen_y;
	int32_t k_location_x;
	int32_t k_location_y;
	char* vid_mem;
	uint8_t keyboard_buffer[1024];
	int32_t buf_idx;
	uint32_t shell_exists;
	uint32_t allow_read;
} terminal_t;

extern terminal_t terminals[3];

// Basic terminal (file) operations
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);
int32_t terminal_read (int32_t fd, uint8_t* buf, int32_t nbytes);
int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes);
void terminal_clear();

// update the cursor position
void update_cursor(int offset);
// disable blinking cursor
void disable_cursor(void);
// Scroll when bottom of screen is reached
void scroll(void);

// return the number (0, 1, 2) of the active terminal
int32_t get_active_terminal(void);
void set_active_terminal(int32_t t);
void change_terminal(int32_t);
void set_terminal_memory();
void clear_screen(void);
void update_keyboard_location(void);
void store_keyboard_location(void);

#endif /* _TERMINAL_H */
