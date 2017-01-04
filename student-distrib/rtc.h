/* rtc.h - Defines used in interactions with the RTC chip
 * vim:ts=4 noexpandtab
 */

#ifndef _RTC_H
#define _RTC_H

#include "types.h"

/* Ports that each PIC sits on */
#define RTC_REG_NUM_PORT 0x70
#define RTC_CMOS_PORT  0x71

/* Externally-visible functions */

/* Initialize RTC */
void rtc_init(void);
void rtc_interrupt_handler();
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_read (int32_t fd, uint8_t* buf, int32_t nbytes);
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes);
void test_rtc();


#endif /* _RTC_H */
