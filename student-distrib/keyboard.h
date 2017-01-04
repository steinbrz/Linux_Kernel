// keyboard.h - Declarations of Functions for keyboard
 
#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "lib.h"


// Ports that each PIC sits on
#define KEYBOARD_ENCODER 0x60
#define KEYBOARD_CONTROLLER  0x64
#define ALT_PRESSED 0x38
#define ALT_RELEASED 0xB8
#define F1_PRESSED 0x3B
#define F2_PRESSED 0x3C
#define F3_PRESSED 0x3D
#define F1_RELEASED 0xBB
#define F2_RELEASED 0xBC
#define F3_RELEASED 0xBD

char buffer[3][1024];
char read_buffer[1024];
int read_flag;
int write_flag;


// Initialize Keyboard
void keyboard_init(void);

// Handle Keyboard Interrupts
void keyboard_interrupt_handler (void);

//Clears the buffer
void clear_buffer();

// Writes text to the screen
int32_t keyboard_write(int32_t fd, const void* buf, int32_t nbytes);
#endif /* _KEYBOARD_H */
