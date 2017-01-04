

#ifndef _SYSCALL_LINK_H
#define _SYSCALL_LINK_H

#include "types.h"
#include "lib.h"
#include "x86_desc.h"
#include "i8259.h"

int32_t system_call(void);
int32_t set_terminal_shell(uint32_t temp);

void go_to_user_mode(uint32_t temp);

void end_program(void);

#endif /* _SYSCALL_LINK_H */
