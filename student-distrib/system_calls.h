/* system_calls.h -
 * vim:ts=4 noexpandtab
 */
#ifndef _SYSTEM_CALLS_H
#define _SYSTEM_CALLS_H

#include "file_system.h"
#include "terminal.h"
#include "types.h"
#include "lib.h"
#include "rtc.h"
 
volatile int32_t executing_process;
#define VIDEO 0xB8000
#define KERNEL_MEM_BOTTOM 0x800000
#define NOT_IN_USE 0
#define IMAGE_MEM 0x08048000
#define _128MB 0x08000000
#define _132MB 0x08400000
#define KB8 0x8000
#define KB4_Shift 12
#define IN_USE 1



typedef struct function_table{

	int32_t (*read)(int32_t fd, uint8_t * buf, int32_t nbytes);
	int32_t (*write)(int32_t fd, const void * buf, int32_t nbytes);
	int32_t (*open)(const uint8_t* filename);
	int32_t (*close)(int32_t fd); 

}function_table;


typedef struct file_descriptor{

	function_table* jtable;
	uint32_t inode_ptr;
	uint32_t position;
	uint32_t flags;
} file_descriptor;


/*PCB structure keeps track of its parent, children, 
process number, associated file descriptor array, what terminal
it is associated with, and previous values of esp and ebp used for task
switching*/

typedef struct PCB_struct{
	file_descriptor fd_array[8];
	uint32_t process_ID;
	uint32_t signals;

	uint8_t args[128];
	uint32_t parent_exists;
	uint32_t child_exists;
	uint32_t terminal_shell;

	uint32_t old_esp;
	uint32_t old_ebp;
	uint32_t curr_esp;
	uint32_t curr_ebp;
	uint32_t kernel_stack;
	uint32_t terminal;
	struct PCB_struct* parent_pcb;
	struct PCB_struct* child_pcb;
}PCB_struct;


volatile PCB_struct* executing_control_block;




int32_t sys_execute(const uint8_t * command);

int32_t sys_halt(uint8_t status);

int32_t sys_read(int32_t fd, void* buf, int32_t nbytes);


int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t sys_open(const uint8_t* filename);

int32_t sys_close(int32_t fd);

int32_t sys_getargs (uint8_t* buf, int32_t nbytes);

int32_t sys_vidmap (uint8_t ** screen_start);

int32_t sys_set_handler (int32_t signum, void* handler_address);

int32_t sys_sigreturn (void);

void stdin();

void stdout();

void allocate_video(int32_t task);

void boot();

PCB_struct * get_process_pcb( int32_t process);

void set_executing_process(uint32_t new_process);

uint32_t get_executing_process();

int32_t get_process_terminal();




#endif /* _SYSTEMCALLS_H */
