#paging_help.S - helper function to set register values for paging



.global set_registers

#set_registers
# decription: sets registers to values needed for paging
# input: page_dir_addr
# output: sets various registers
# return: none
set_registers:

	pushl %ebp
	movl %esp, %ebp

	movl 8(%ebp), %eax
	movl %eax, %cr3

	movl %cr4, %eax
	orl $0x00000090, %eax
	movl %eax, %cr4



	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0

	movl %ebp, %esp
	popl %ebp

	ret

