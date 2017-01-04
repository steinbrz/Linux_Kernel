
#define ASM     1
#include "x86_desc.h"

.global system_call
.global go_to_user_mode
.global set_terminal_shell
.global end_program



# system_call
#  DESCRIPTION: saves reg values on stack and calls correct system_call
#		when returning, restores regs and executes IRET to return to user code
#  INPUTS: none
#  OUTPUTS: none
#  RETURN values: -1 if failure
#  SIDE EFFECTS: none

system_call:
	pushw %es
	pushw %ds
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	
	cmpl $1, %eax
	jb invalid_call

	cmpl $10, %eax
	ja invalid_call

	call *syscall_table(,%eax,4)

#	cmpl $-1, %eax
#	je invalid_call
return_to:
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popw %ds
	popw %es
	iret

end_program:
	jmp return_to

invalid_call:
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popw %ds
	popw %es
	movl $-1, %eax
	iret

	syscall_table:
	.long 0x00, sys_halt, sys_execute , sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap, sys_set_handler, sys_sigreturn
	#push artifical IRET context to stack
	#source http://www.jamesmolloy.co.uk/tutorial_html/10.-User%20Mode.html 
	#stack prior to IRET
	#		SS
	#		ESP
	#		EFLAGS
	#		CS
	#		EIP

# go_to_user_mode
#  DESCRIPTION: called after execute to move to user code, sets up user stack
#  INPUTS: none
#  OUTPUTS: none
#  RETURN values: -1 if failure
#  SIDE EFFECTS: sets up new user stack

go_to_user_mode:

cli

movl 4(%esp), %ebx;
movl $USER_DS, %eax;		
	
movw %ax, %ds	
movw %ax, %es	
movw %ax, %fs	
movw %ax, %gs	
							

pushl %eax		
pushl $0x83ffffc		#User Stack Address
			
pushf				
popl %eax			
orl  $0x200, %eax	
pushl %eax			
							
pushl $USER_CS		
pushl %ebx			
sti
iret	

# set_terminal_shell:	

# cli
# movl 4(%esp), %ebx;
# movl $USER_DS, %eax;		
	
# movw %ax, %ds	
# movw %ax, %es	
# movw %ax, %fs	
# movw %ax, %gs	
							

# pushl %eax		
# pushl $0x83ffffc		#User Stack Address
			
# pushf				
# #popl %eax			
# #orl  $0x200, %eax	
# #pushl %eax			
							
# pushl $USER_CS		
# pushl %ebx			
# sti		
