/* rtc.c - Functions to interact with RTC chip
 * vim:ts=4 noexpandtab
 */

#include "rtc.h"
#include "lib.h"
#include "i8259.h"

volatile int RTC_IS_OPEN = 0;
volatile int rtc_int_flag = 0;

/* 
 * rtc_init
 *   DESCRIPTION: Initialize the RTC chip to time 0 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets RTC to initial interrupt speed of 2Hz
 */

void
rtc_init(void)
{
    asm("cli");

    outb(0x8A, RTC_REG_NUM_PORT);
    inb(RTC_CMOS_PORT);

    outb(0x8A, RTC_REG_NUM_PORT);
    outb(0x2F, RTC_CMOS_PORT);

    outb(0x8B, RTC_REG_NUM_PORT);
    char b = inb(RTC_CMOS_PORT);

    outb(0x8B, RTC_REG_NUM_PORT);
    outb(b | 0x40, RTC_CMOS_PORT);

    outb(0x0C, RTC_REG_NUM_PORT);
    inb(RTC_CMOS_PORT);
    enable_irq(8);          //enable RTC at irq 8
    asm("sti");
}
/* 
 * rtc_interrupt_handler
 *   DESCRIPTION: Serves as the interrupt handler for RTC
 *   INPUTS: none
 *   OUTPUTS: Sets rtc_int_flag to 1
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 
 */
void rtc_interrupt_handler()
{
    outb(0x0C, RTC_REG_NUM_PORT);
    inb(RTC_CMOS_PORT);
    //test_interrupts();
    //printf("RTC Interrupt\n");
    rtc_int_flag = 1;
    send_eoi(8);
    asm("sti");
}

/* Beginning of RTC System Calls */
/* 
 * rtc_open
 *   DESCRIPTION: Serves as the RTC Open system call, enforces only one user at a time can open RTC
 *   INPUTS: filename
 *   OUTPUTS: Sets flag RTC_IS_OPEN to 1
 *   RETURN VALUE: -1 if error, 0 if success
 *   SIDE EFFECTS: rtc is initialized by calling rtc_init
 */
int32_t rtc_open(const uint8_t* filename)
{
    /* if rtc isn't open, set to open; if already open return unsuccessful */
    if(RTC_IS_OPEN == 0)
    {
        rtc_init();             //initialize RTC to 2 Hz
        RTC_IS_OPEN = 1;
        return 0;
    }
    else
        return -1;
}
/* 
 * rtc_close
 *   DESCRIPTION: Serves as the RTC Close system call, closes RTC if open
 *   INPUTS: fd
 *   OUTPUTS: Sets flag RTC_IS_OPEN to 0
 *   RETURN VALUE: -1 if error, 0 if success
 *   SIDE EFFECTS: none
 */
int32_t rtc_close(int32_t fd)
{
    if(RTC_IS_OPEN == 1)
    {
        RTC_IS_OPEN = 0;
        return 0;
    }
    else
        return -1;
}


 /* 
 * rtc_read
 *   DESCRIPTION: rtc_read function, reads data from the RTC
 *      Wait until after RTC interrupt is generated before returning 
 *      (set a flag and wait for interrupt handler to clear it)
 *   INPUTS: fd, buf, nbytes
 *   OUTPUTS: none
 *   RETURN VALUE: 0 when RTC interrupt occurs
 *   SIDE EFFECTS: none
 */
int32_t rtc_read (int32_t fd, uint8_t* buf, int32_t nbytes)
{
    asm("sti");
    rtc_int_flag = 0;
    while(rtc_int_flag == 0){};
    return 0;

}

/* 
 * rtc_write
 *   DESCRIPTION: rtc_write function, writes data to the RTC
 *      accepts 4-byte integer specifying interrupt rate in Hz
 *      sets periodic interrupts accordingly
 *      can only generate interrupts at power of 2 rate (check), and limit to max 1024 Hz
 *      Frequency is set in lower 4 bits of register A
 *   INPUTS: fd, buf, nbytes
 *   OUTPUTS: Sets RTC interrupt speed
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes)
{
    asm("cli");
    char out_bits = 0x00;
    int32_t frequency;
    frequency = *((int32_t*)buf);
    /* check input value to choose bits which should be put in reg A */
    if(frequency == 0)
        out_bits = 0x0;
    else if(frequency == 2)
        out_bits = 0xF;
    else if(frequency == 4)
        out_bits = 0xE;
    else if(frequency == 8)
        out_bits = 0xD;
    else if(frequency == 16)
        out_bits = 0xC;
    else if(frequency == 32)
        out_bits = 0xB;
    else if(frequency == 64)
        out_bits = 0xA;
    else if(frequency == 128)
        out_bits = 0x9;
    else if(frequency == 256)
        out_bits = 0x8;
    else if(frequency == 512)
        out_bits = 0x7;
    else if(frequency == 1024)
        out_bits = 0x6;
    else
        return -1;
    
    char regA_prev = 0;
    outb(0x8A, RTC_REG_NUM_PORT);
    regA_prev = inb(RTC_CMOS_PORT);
    regA_prev &= 0xF0;              //set low 4 bits, while leaving upper 4 bits
    out_bits += regA_prev;
    outb(0x8A, RTC_REG_NUM_PORT);
    outb(out_bits, RTC_CMOS_PORT);  //set reg A with new interrupt rate
    asm("sti");
    return 0;
}

/* 
 * test_rtc
 *   DESCRIPTION: function to test other RTC functions
 *   INPUTS: none
 *   OUTPUTS: prints to the screen multiple times
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
// void test_rtc()
// {
//     /* test RTC read function */
//     /* Start at 2 Hz */
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest\n");
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     /* Increase Frequency to 8 Hz */
//     rtc_write(0, 8, 0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest\n");
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     rtc_read(0,0,0);
//     /* Increase Frequency to 16 Hz */
//     rtc_write(0, 16, 0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest ");
//     rtc_read(0,0,0);
//     printf("RTCtest\n");
//     rtc_read(0,0,0);

// }
