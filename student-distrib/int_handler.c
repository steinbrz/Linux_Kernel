/*int_handler.c - Function to initialize idt array*/

#include "int_handler.h"
#include "i8259.h"
#include "syscall_linkage.h"
#include "system_calls.h"

/* DO_ERROR macro
 * DESCRIPTION: generic interrupt handlers. disable interrupts, print which exception
 *				 occurred, then halt the process which triggered the interrupt
 * INPUTS: name, str
 * OUTPUTS: prints exception
 * RETURN VALUE: none
 * SIDE EFFECTS: halts function which faulted
 */
#define DO_ERROR(name, str);			\
void DO_##name() 						\
{										\
	asm("cli");							\
	printf("%s exception\n", #str);		\
	sys_halt(0);						\
}										\

DO_ERROR(ZERO_DIVIDE, "Divide by Zero");
DO_ERROR(NMI, "Non-maskable interrupt");
DO_ERROR(BREAKPOINT, "Breakpoint");
DO_ERROR(OVERFLOW, "Overflow");
DO_ERROR(BOUND, "Boundary");
DO_ERROR(NOSEGMENT, "No Stack Segment");
DO_ERROR(STACKSEGFAULT, "Stack Segfault");
DO_ERROR(PROTECTFAULT, "General Protection");
DO_ERROR(PAGEFAULT, "Page fault");

/* init_idt_array
 * DESCRIPTION: initializes the IDT array, setting default values for the
 *				idt array and create entries for exceptions, device
 *				interrupts,	and an entry for system calls.
 * INPUTS: none
 * OUTPUTS: a correct idt array
 * RETURN VALUE: none
 * SIDE EFFECTS: creates IDT array
 */
void init_idt_array()
{	
	int i;

	for( i = 0; i < NUM_VEC; i++){
		idt[i].seg_selector = KERNEL_CS;

		idt[i].reserved4 = 0x00;
		if (i<32)		//Exceptions
			idt[i].reserved3 = 0x1;
		else
			idt[i].reserved3 = 0x0;
		idt[i].reserved2 = 0x1;
		idt[i].reserved1 = 0x1;
		idt[i].size = 0x1;
		idt[i].reserved0 = 0x0;
		if (i == 0x80)
			idt[i].dpl = 0x3;
		else
			idt[i].dpl = 0x0;
		idt[i].present = 0x1;
	}


	SET_IDT_ENTRY(idt[0], zero_divide_handler);
	SET_IDT_ENTRY(idt[2], nmi_handler);
	SET_IDT_ENTRY(idt[3], breakpoint_handler);
	SET_IDT_ENTRY(idt[4], overflow_handler);
	SET_IDT_ENTRY(idt[5], bound_handler);

	SET_IDT_ENTRY(idt[11], no_segment_handler);
	SET_IDT_ENTRY(idt[12], stacksegfault_handler);
	SET_IDT_ENTRY(idt[13], general_protection_handler);
	SET_IDT_ENTRY(idt[14], page_fault_handler);

	SET_IDT_ENTRY(idt[0x20], pit_handler);
	SET_IDT_ENTRY(idt[0x21], keyboard_handler);
	SET_IDT_ENTRY(idt[0x28], rtc_handler);
	SET_IDT_ENTRY(idt[0x80], system_call);

}
