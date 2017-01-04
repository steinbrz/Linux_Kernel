/* scheduling.c - Useful macros for debugging
 * vim:ts=4 noexpandtab
 */

#include "scheduling.h"
#include "syscall_linkage.h"
#include "system_calls.h"
 
#define CHANNEL0 0x40
#define CMD_REG 0x43
#define PIT_INPUT 0x30
#define FREQ_DIVISOR 59659

#define GARBAGE 1234
int32_t task_list[6] = {GARBAGE, GARBAGE, GARBAGE, GARBAGE, GARBAGE, GARBAGE};
int32_t number_of_processes = 0;

/* 
 * task_switcher
 *   DESCRIPTION: task_switcher function, context switches from old_process to new_process
 *   INPUTS: old_process, new_process
 *   OUTPUTS: Changes executing process to new process, changes tss.esp0 for new executing process
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */

void task_switcher(int32_t old_process, int32_t new_process)
{
	uint32_t new_esp;
	uint32_t new_ebp;


	


	PCB_struct * old_control_block;
	old_control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (old_process + 2)));
	PCB_struct * new_control_block;
	new_control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (new_process + 2)));



	

	/* Store old_process's esp and ebp*/
	asm("\t movl %%esp, %0" : "=r"(old_control_block->curr_esp));
	asm("\t movl %%ebp, %0" : "=r"(old_control_block->curr_ebp));
	
	page_allocator(new_process);
	/* redirect esp and ebp to new_process */
	new_esp = new_control_block->curr_esp;
	//tss.esp0 = new_esp;
	new_ebp = new_control_block->curr_ebp;
	// tss.ebp = new_ebp;
	tss.esp0 = KERNEL_MEM_BOTTOM - (KB8) * (new_process + 1) - 4;

	executing_process = new_process;
	executing_control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));

	//reset PIT
	outb((FREQ_DIVISOR & 0xFF), CHANNEL0);
	outb((FREQ_DIVISOR >> 8), CHANNEL0);

 //    asm("sti");
	// go_to_user_mode(new_esp);

	asm volatile("movl %0, %%esp ;" :: "g"(new_esp)); 	
	asm volatile("movl %0, %%ebp ;" :: "g"(new_ebp));	
	// asm volatile("jmp switchProgram");
	//asm volatile("leave");
	//asm volatile("ret");		  
	

	return;
}

/* 
 * pit_init
 *   DESCRIPTION: pit_init function, initializes PIT and enables its IRQ entry
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: IRQ 0 is enabled
 */

void pit_init(void){

	outb(PIT_INPUT, CMD_REG);
	outb((FREQ_DIVISOR & 0xFF), CHANNEL0);
	outb((FREQ_DIVISOR >> 8), CHANNEL0);
	enable_irq(0);
}

/* 
 * pit_interrupt_handler
 *   DESCRIPTION: functions as pit interrupt handler, calls scheduler function to switch processes
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void pit_interrupt_handler(void){
	asm("cli");
	send_eoi(0);
	scheduler();
	asm("switchProgram:");
	
	asm("sti");
}

/* 
 * scheduler
 *   DESCRIPTION: scheduler function, called by pit interrupt, checks if process needs to be switched for multitasking, calls task switcher if necessary
 *   INPUTS: none
 *   OUTPUTS: reorders task_list
 *   RETURN VALUE: none
 *   SIDE EFFECTS: processes are switched if needed
 */
void scheduler()
{
	int i;
	int j;
	int32_t temp;
	int32_t new_process = 0;

	
	if(number_of_processes == 1){

	PCB_struct * old_control_block;
	old_control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process+ 2)));
	PCB_struct * new_control_block;
	new_control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (new_process + 2)));

	asm("\t movl %%esp, %0" : "=r"(old_control_block->curr_esp));
	asm("\t movl %%ebp, %0" : "=r"(old_control_block->curr_ebp));
	
	outb((FREQ_DIVISOR & 0xFF), CHANNEL0);
	outb((FREQ_DIVISOR >> 8), CHANNEL0);
	sti();

	return;


	
	}	

	//check_task_list();		//function to check task_list for correctness

	while(1){
		new_process = task_list[0];
		if(new_process == GARBAGE)
		{
			//reset PIT
			outb((FREQ_DIVISOR & 0xFF), CHANNEL0);
			outb((FREQ_DIVISOR >> 8), CHANNEL0);
			asm("sti");
			return;
		}
		//fix task_list, remove new task from beginning and append to end
		for(i = 0; i < number_of_processes; i++)
		{
			temp = task_list[i];
			j = i;
			if(j == 0)
				continue;
			else
				j--;
			// j--;
			// j %= 7;
			task_list[j] = temp;
		}
		task_list[number_of_processes - 1] = new_process;


		if(new_process == executing_process){

		outb((FREQ_DIVISOR & 0xFF), CHANNEL0);
		outb((FREQ_DIVISOR >> 8), CHANNEL0);
		sti();


		return;
		}

		


		PCB_struct * new_control_block;
		new_control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (new_process + 2)));
		//only schedule tasks which don't have children

		
		if(new_control_block->child_exists == 0)
			break;
	}



	task_switcher(executing_process, new_process);

	return; 
}

/* 
 * check_task_list
 *   DESCRIPTION: check task list function which is called to check and reorder tasks in task list,
 *				should have tasks in order starting at position 0 with no empty spots
 *   INPUTS: none
 *   OUTPUTS: reorders task_list
 *   RETURN VALUE: none
 *   SIDE EFFECTS: processes are switched if needed
 */

void check_task_list()
{
	int i;
	int j;
	int unused_entry;
	for( i = 0; i < 5; i++)
	{
		//if task_list entry isn't full, scan list to fill it
		if(task_list[i] < 0 || task_list[i] > 5)
		{
			unused_entry = i;
			//find next entry filled with a process
			for( j = unused_entry; j < 6; j++)
			{
				if(task_list[j] >= 0 && task_list[j] <= 5)
				{
					task_list[unused_entry] = task_list[j];
					task_list[j] = GARBAGE;
					break;
				}
			}
		}
	}
	return;
}

/* 
 * add_task_list_entry 
 *   DESCRIPTION: Called to add a new process into the task list 
 *   INPUTS: process_num
 *   OUTPUTS: modifies task_list
 *   RETURN VALUE: returns -1
 *   SIDE EFFECTS: none
 */
int32_t add_task_list_entry(int32_t process_num)
{
	//check_task_list();
	if(number_of_processes > 5){
		return -1;
	}
	int i;
	for(i = 0; i < 7; i++)
	{
		if(task_list[i] == GARBAGE)
		{
			task_list[i] = process_num;
			break;
		}
	}

	//task_list[number_of_processes] = process_num;
	number_of_processes++;
	return -1;
}

/* 
 * add_task_list_entry 
 *   DESCRIPTION: Called to delete a halted process from the task list 
 *   INPUTS: process_num
 *   OUTPUTS: modifies task_list
 *   RETURN VALUE: none
 *   SIDE EFFECTS: calls check task list to reorder task list
 */
 void delete_task_list_entry(int32_t process_num)
{
	int i;
	for(i = 0; i < 7; i++)
	{
		if(task_list[i] == process_num)
		{
			task_list[i] = GARBAGE;
			break;
		}
	}
	number_of_processes--;
	check_task_list();
}
