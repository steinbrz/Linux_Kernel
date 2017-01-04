/* system_calls.c - 
 * vim:ts=4 noexpandtab
 */

#include "syscall_linkage.h"
#include "lib.h"
#include "types.h"
#include "x86_desc.h"
#include "system_calls.h"
#include "file_system.h"
#include "paging.h"
#include "terminal.h"
#include "scheduling.h"

PCB_struct shell_PCB;
PCB_struct* current_block;
int SHELL_EXISTS = 0;
PCB_struct * control_block2;
uint8_t process_mask = 0x00;


//uint32_t open_processes = 0;

//Jump tables for read/write/open/close system calls
function_table file_jtable = { file_read, file_write, file_open, file_close};
function_table terminal_jtable = {terminal_read, terminal_write, terminal_open, terminal_close};
function_table rtc_jtable = {rtc_read, rtc_write, rtc_open, rtc_close};
function_table directory_jtable = {dir_read, dir_write, dir_open, dir_close};


/* sys_execute
 * DESCRIPTION: attempts to load and execute a new program, handing off the processor to the new program until it terminates
 * INPUTS: command - space-separated sequence of words, first word being the file name of the program to be executed
 * OUTPUTS: none
 * RETURN VALUES: -1 on failure, 256 if the program dies by an exception, 
 *				  or a value in the range 0 to 255 if the program executes a halt
 * SIDE EFFECTS: loads and executes the program
 */
int32_t sys_execute(const uint8_t * command)
{
	cli();
	uint8_t  file_name[32] = "";
	uint8_t  cmd_args[128] = "";
	int i;
	int j;
	int IS_TERM_SHELL;
	i = 0;
	j = 0;
	int32_t process_num;
	int32_t file_error;
	int32_t command_read;
	command_read = 0;

	int32_t cmd_length;

	if(command == NULL)
	{
		return -1;
	}
	

	cmd_length = strlen((int8_t*)command);

	
	for(i = 0; i < cmd_length; i++){

		if(command_read == 0){

			if( command[i] != ' '){

				file_name[i] = command[i];
			}

			else{

				command_read = 1;
				file_name[i] = '\0';
			}
		}

		else{

			if(command[i] != '\0' && i < 128){

				cmd_args[j] = command[i];
				j++;


			}
			else{

				cmd_args[j] = '\0';
			}
		}


	}

	if(strncmp((int8_t*)command, (int8_t*)"shell", 5) == 0 && strncmp((int8_t*)cmd_args, (int8_t*)"terminal", 8) == 0)
	{
		IS_TERM_SHELL = 1;
	}
	else
	{
		IS_TERM_SHELL = 0;
	}

/*check if an exit command is called*/

	if(strncmp((int8_t*)file_name, "exit", 4) == 0){

		return sys_halt(69);
	}

	int32_t bitmask;
	bitmask = 0x01;

	process_num = -1;


	// Allocate for open process, must support six processes
	for(i = 0; i < 6; i++){

		if((bitmask & ~process_mask)){

			process_mask |= bitmask;
			process_num = i;
			break;
		}

		else{

			bitmask = bitmask << 1;
		}



	}

	if(process_num == -1){

		return -1;
	}

	//set up paging
	page_allocator(process_num);

    //file loader
	file_error = fs_load((uint8_t*)file_name, IMAGE_MEM);

	//check
	if(file_error != 0)
	{
		page_allocator(0);
		bitmask = 0x01;
		process_mask &= ~(bitmask << process_num);
		return -1;

	}
	
	// PCB should be at top of stack, stack grows towards it
	//btw SS0 is set in the kernel.c so I don't think we need to alter it
	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (process_num + 2)));

	control_block->process_ID = process_num;

	strcpy((int8_t*)control_block->args,(int8_t*)cmd_args);

	//add process to task_list
	add_task_list_entry(process_num);


	// File_descriptor array is defined at bottom, we need to initialize values for it
	for(i = 0; i < 8; i++){
		control_block->fd_array[i].jtable = NULL;
		control_block->fd_array[i].inode_ptr = 0;			//////fd_array replaced by file_array
		control_block->fd_array[i].position = 0;
		control_block->fd_array[i].flags = NOT_IN_USE;
	}
	
	

	asm("\t movl %%esp, %0" : "=r"(control_block->old_esp));
	asm("\t movl %%ebp, %0" : "=r"(control_block->old_ebp));
	control_block->curr_esp = control_block->old_esp;
	control_block->curr_ebp = control_block->old_ebp;

	if(IS_TERM_SHELL == 1){

		
		control_block->parent_exists = 0;
		control_block->child_exists = 0;
		control_block->parent_pcb = NULL;
		// open_processes = 0x01;
		control_block->process_ID = process_num;
		control_block->terminal = get_active_terminal();
		executing_process = process_num;
		control_block->terminal_shell = 1;

	 	
			
	}
	else{

		control_block->parent_pcb = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
		control_block->child_exists = 0;
		control_block->parent_exists = 1;
		control_block->parent_pcb->child_pcb = control_block;
		control_block->parent_pcb->child_exists = 1;
		control_block->terminal = control_block->parent_pcb->terminal;
		// open_processes = 2;
		control_block->process_ID = process_num;
		executing_process = process_num;
	}


	//context switch
	//write to TSS
	tss.esp0 = KERNEL_MEM_BOTTOM - (KB8) * (process_num + 1) - 4;
	//tss.ebp = KERNEL_MEM_BOTTOM - (KB8) * (process_num + 1) - 4;	
	uint8_t buf[4];
	fs_read((uint8_t*)file_name, buf, 24, 4);

	uint32_t temp;
	temp = 0;
	//int32_t ret_bytes;
	control_block2 = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	

	temp |= buf[0];
	temp |= buf[1] << 8;
	temp |= buf[2] << 16;
	temp |= buf[3] << 24;

	stdin();
	stdout();

	sti();

	go_to_user_mode(temp);

	asm("halt_program:");

	return 0;
}


/* sys_halt
 * DESCRIPTION:  terminates a process, returning the specified value to its parent process
 * INPUTS: status
 * OUTPUTS: none
 * RETURN VALUES: should never return to caller
 * SIDE EFFECTS: halts the program
 */
int32_t sys_halt(uint8_t status)
{
	//restore parents stack pointers

	cli();
	uint32_t esp_old;
	uint32_t ebp_old;

	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	esp_old = control_block->old_esp;
	ebp_old = control_block->old_ebp;

	if(control_block->terminal_shell == 1){

		uint8_t buf[4];
		if( -1 == fs_read((uint8_t*)"shell", buf, 24, 4)){
			return -1;
		}

		uint32_t temp;
		temp = 0;


		temp |= buf[0];
		temp |= buf[1] << 8;
		temp |= buf[2] << 16;
		temp |= buf[3] << 24;
		sti();

		go_to_user_mode(temp);

	}

	/*tss.esp0 = control_block->old_esp;
	tss.ebp = control_block->old_ebp;*/

	//control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	int i = 0;
	for(i = 0; i < 8; i++)
	{
		if(control_block->fd_array[i].flags != NOT_IN_USE)
		{
			sys_close(i);
		}
	}

	uint8_t bitmask;
	bitmask = 0x01;
	// Mark process as closed
//	open_processes--;
//	open_processes = (~(bitmask << control_block->process_ID) && open_processes);

	process_mask &= ~(bitmask << control_block->process_ID);
	control_block->parent_pcb->child_exists = 0;
	control_block->parent_pcb->child_pcb = NULL;

	tss.esp0 = KERNEL_MEM_BOTTOM - (KB8) * (control_block->parent_pcb->process_ID + 1) - 4;
	//tss.ebp = ebp_old;

	//remove halting process from task list
	delete_task_list_entry(executing_process);


	int process_num;
	process_num = control_block->parent_pcb->process_ID;			//for now, will always return to shell
	executing_process = process_num;
	page_allocator(process_num);

	tss.esp0 = KERNEL_MEM_BOTTOM - (KB8) * (control_block->parent_pcb->process_ID + 1) - 4;
	//only return 8 bits
	int32_t returnBits;
	returnBits = (status & 0x000000FF);
	/*asm volatile("movl %%eax, %%esp 	\n\
				  movl %%edx, %%eax     \n\
				  movl %%ebx, %%ebp     \n\
		"
		:
		: "a" (esp_old), "b"(ebp_old), "d" (returnBits)
		);*/
	asm volatile("movl %0, %%esp" :: "g"(esp_old));
	asm volatile("movl %0, %%ebp" :: "g"(ebp_old));
	asm volatile("pushl %0" :: "g"(returnBits));
	asm volatile("popl %eax");
	sti();
	asm volatile("jmp halt_program");
	return 0;
}


/* sys_read
 * DESCRIPTION: reads data from the keyboard, a file, device (RTC), or directory
 * INPUTS: fd - file descriptor
 *		   buf - buffer to be filled with bytes read
 *		   nbytes - number of bytes to be read
 * OUTPUTS: puts the read data into buf as output
 * RETURN VALUES: returns the number of bytes read, or 0 to indicate eof. -1 for failure
 * SIDE EFFECTS: none
 */
int32_t sys_read(int32_t fd, void* buf, int32_t nbytes){
	
	// Check that the file directory exists, the buffer isn't null, and it's not stdout
	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	if(fd > 7 || fd < 0)
		return -1;

	if((control_block->fd_array[fd].flags == NOT_IN_USE) || buf == NULL || fd == 1){

		return -1;
	}


	 return  control_block->fd_array[fd].jtable->read(fd, (uint8_t*) buf, nbytes);
}


/* sys_write
 * DESCRIPTION: writes data to the terminal or to a device (RTC)
 * INPUTS: fd - file descriptor
 *		   buf - buffer filled with bytes to be written
 *		   nbytes - number of bytes to be written
 * OUTPUTS: none
 * RETURN VALUES: 0 for success, -1 for failure
 * SIDE EFFECTS: in case of terminal, all data is displayed to the screen
 *				 in caes of RTC, sets rate of periodic interrupts
 */
int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes){
	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	if(fd > 7 || fd < 0)
		return -1;
	
	if((control_block->fd_array[fd].flags == NOT_IN_USE) || buf == NULL || fd == 0){

		return -1;
	}

	return control_block->fd_array[fd].jtable->write(fd, buf, nbytes);
}


/* sys_open
 * DESCRIPTION: provides access to the file system
 * INPUTS: filename - name of file to get access to
 * OUTPUTS: none
 * RETURN VALUES: index of file descriptor array, -1 for failure
 * SIDE EFFECTS: allocates an unused file descriptor and 
 *				 sets up any data necessary to handle the given type of file
 */
int32_t sys_open(const uint8_t* filename){

	dentry_t temp_dentry;
	int i;
	int32_t file_type;
	int32_t name_length;
	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	

	if(filename == NULL){

		return -1;
	}


	if(read_dentry_by_name(filename,&temp_dentry) == -1){

		return -1;
		}

	name_length = strlen((int8_t*)temp_dentry.f_name);

	for(i = 2; i < 8; i++){

		if(control_block->fd_array[i].flags == NOT_IN_USE){

			file_type = temp_dentry.f_type;
	
			// File type is rtc
			if(temp_dentry.f_type == 0){

				control_block->fd_array[i].jtable = &(rtc_jtable);
				
				control_block->fd_array[i].flags = IN_USE;
				return i;
			}
			// file type is a directory
			else if(temp_dentry.f_type == 1){

				control_block->fd_array[i].jtable = &(directory_jtable);
				
				control_block->fd_array[i].flags = IN_USE;
				control_block->fd_array[i].position = 0;
				return i;
			}
			// file type is a regular file
			else if(temp_dentry.f_type == 2){

				control_block->fd_array[i].jtable = &(file_jtable);
				control_block->fd_array[i].inode_ptr = temp_dentry.inode_num;
				control_block->fd_array[i].flags = IN_USE;
				control_block->fd_array[i].position = 0;
				return i;
			}
		}
	}

	return -1;


}


/* sys_close
 * DESCRIPTION: closes the specified file descriptor 
 * INPUTS: fd - file descriptor index of file to be closed
 * OUTPUTS: none
 * RETURN VALUES: 0 for success, -1 for failure
 * SIDE EFFECTS: makes it available for return from later calls to open
 */
int32_t sys_close(int32_t fd){
	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	if(control_block->fd_array[fd].flags == NOT_IN_USE){

		return -1;
	}
	if(fd > 7 || fd < 2)
		return -1;
	if(control_block->fd_array[fd].flags == NOT_IN_USE){

		return -1;
	}

	control_block->fd_array[fd].flags = NOT_IN_USE;

	return 0;
}


/* sys_getargs
 * DESCRIPTION: reads the program’s command line arguments into a user-level buffer
 * INPUTS: buf: program's command line buffer
 		   nbytes: number of bytes
 * OUTPUTS: none
 * RETURN VALUES: 0 for success, -1 for failure
 * SIDE EFFECTS: arguments copied into user space
 */
int32_t sys_getargs (uint8_t* buf, int32_t nbytes){
	int arg_length;
	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));

	arg_length = strlen((int8_t*)control_block->args);

	if(buf == NULL){

		return -1;
	}
	
	if(arg_length > nbytes)		//if args don't fit in buffer, return -1
		return -1;
	

	strcpy((int8_t*)buf, (int8_t*)control_block->args);
	return 0;
}


/* sys_vidmap
 * DESCRIPTION: maps the text-mode video memory into user space at a pre-set virtual address
 * INPUTS: screen_start: location provided by the caller
 * OUTPUTS: none
 * RETURN VALUES: 0 for success, -1 for failure
 * SIDE EFFECTS: adds another page mapping for the program (4 kB page)
 */
int32_t sys_vidmap (uint8_t ** screen_start){

	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8*(executing_process + 2)));

	if((uint32_t)screen_start < _128MB || (uint32_t)screen_start > _132MB){

		return -1;
	}

	switch(get_active_terminal()){

		case 0:
			*screen_start = (uint8_t*)VIDEO1;
			break;
			
		case 1:
			*screen_start = (uint8_t*)VIDEO2;
			break; 
			
		case 2: 
			*screen_start = (uint8_t*)VIDEO3;
			break;
			
		default:
			break;
	}

	return 0;

}


/* sys_set_handler
 * DESCRIPTION: changes the default action taken when a signal is received
 * INPUTS: signum: specifies which signal’s handler to change
 *		   handler_address: points to a user-level function to be run
 * OUTPUTS: none
 * RETURN VALUES: 0 for success, -1 for failure
 * SIDE EFFECTS: none
 */
int32_t sys_set_handler (int32_t signum, void* handler_address){
	return 0;
}


/* sys_sigreturn
 * DESCRIPTION: should copy the hardware context that was on the user-level stack back onto the processor
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUES: 0 for success, -1 for failure
 * SIDE EFFECTS: none
 */
int32_t sys_sigreturn (void){
	return 0;
}


/* stdin
 * DESCRIPTION: opens terminal read
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUES: none
 * SIDE EFFECTS: none
 */
void stdin()
{	
	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	control_block->fd_array[0].flags = IN_USE;
	control_block->fd_array[0].jtable = &(terminal_jtable);

}


/* stdout
 * DESCRIPTION: opens terminal write
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUES: none
 * SIDE EFFECTS: none
 */
void stdout()
{
	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));
	control_block->fd_array[1].flags = IN_USE;
	control_block->fd_array[1].jtable = &(terminal_jtable);
}


/* boot
 * DESCRIPTION: boots the initial system by calling execute
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUES: none
 * SIDE EFFECTS: starts the initial shell
 */
void boot(){
sys_execute((uint8_t*)"shell terminal");
}


/* get_process_pcb
 * DESCRIPTION: return a process's PCB
 * INPUTS: process: process who's PCB is required
 * OUTPUTS: none
 * RETURN VALUES: pointer to PCB struct for the process
 * SIDE EFFECTS: none
 */
PCB_struct * get_process_pcb(int32_t process){

	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (process + 2)));

	return control_block;
}


/* get_executing_process
 * DESCRIPTION: returns currently executing process
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUES: currently executing process
 * SIDE EFFECTS: none
 */
uint32_t get_executing_process(){

	return executing_process;
}


/* set_executing_process
 * DESCRIPTION: changed the currently executing process
 * INPUTS: process to be executed
 * OUTPUTS: none
 * RETURN VALUES: none
 * SIDE EFFECTS: currently executing process changes
 */
void set_executing_process(uint32_t new_process){

	executing_process = new_process;
}


/* get_process_terminal
 * DESCRIPTION: return a process's terminal
 * INPUTS: process: process who's terminal is required
 * OUTPUTS: none
 * RETURN VALUES: index of the terminal of the process
 * SIDE EFFECTS: none
 */
int32_t get_process_terminal(){

	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));

	return control_block->terminal;
}
