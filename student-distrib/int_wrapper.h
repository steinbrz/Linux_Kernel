/*int_wrapper.h - header file for int_wrapper.S*/

#include "x86_desc.h"


#ifndef _INT_WRAPPER_H
#define _INT_WRAPPER_H_

extern void zero_divide_handler();
extern void rtc_handler();
extern void keyboard_handler();
extern void nmi_handler();
extern void page_fault_handler();
extern void general_protection_handler();
extern void stacksegfault_handler();
extern void bound_handler();
extern void no_segment_handler();
extern void overflow_handler();
extern void breakpoint_handler();
extern void pit_handler();


#endif /* _INT_WRAPPER_H_ */

