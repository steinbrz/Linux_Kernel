# int_wrappers.S - holds wrappers in order to call functions

# Following lines describe all wrappers below
# push all general use registers to the stack
# disable interrupts
# call the interrupt handler
# pop values from the stack
# enable interrupts
# return to wherever code was before the interrupt

.global rtc_handler 
.global keyboard_handler
.global nmi_handler
.global page_fault_handler
.global zero_divide_handler
.global bound_handler
.global stacksegfault_handler
.global no_segment_handler
.global general_protection_handler
.global overflow_handler
.global breakpoint_handler
.global pit_handler

rtc_handler:
	pushal
	cli
	call rtc_interrupt_handler
	popal
	sti 
	iret


keyboard_handler:
	pushal
	cli
	call keyboard_interrupt_handler
	popal
	sti 
	iret

zero_divide_handler:
	pushal
	cli
	call DO_ZERO_DIVIDE
	popal
	sti
	iret

nmi_handler:
	pushal
	cli
	call DO_NMI
	popal
	sti
	iret

breakpoint_handler:
	pushal
	cli
	call DO_BREAKPOINT
	popal
	sti
	iret
	
page_fault_handler:
	pushal
	cli
	call DO_PAGEFAULT
	popal
	sti
	iret
	
general_protection_handler:
	pushal
	cli
	call DO_PROTECTFAULT
	popal
	sti
	iret

stacksegfault_handler:
	pushal
	cli
	call DO_STACKSEGFAULT
	popal
	sti
	iret

bound_handler:
	pushal
	cli
	call DO_BOUND
	popal
	sti
	iret

overflow_handler:
	pushal
	cli
	call DO_OVERFLOW
	popal
	sti
	iret
	
no_segment_handler:
	pushal
	cli
	call DO_NOSEGMENT
	popal
	sti
	iret

pit_handler:
	pushal
	cli
	call pit_interrupt_handler
	popal
	sti
	iret

