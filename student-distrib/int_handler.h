/* int_handler.h - Declarations for generic interrupt handlers,
 * idt initializations, and such.
 * vim:ts=4 noexpandtab
 */

#include "x86_desc.h"
#include "lib.h"
#include "int_wrapper.h"
#include "keyboard.h"
#include "rtc.h"
#include "i8259.h"
#include "syscall_linkage.h"
#include "scheduling.h"


#ifndef _INT_HANDLER_H
#define _INT_HANDLER_H

extern void init_idt_array();

void DO_RTC_INT();

#endif /* _INT_HANDLER_H */
