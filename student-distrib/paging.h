/*paging.h - header for files used to implement paging
 * vim:ts=4 noexpandtab
 */

#ifndef _PAGING_H
#define _PAGING_H


#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "types.h"
#include "paging_help.h"


#define VIDEO 0xB8000
#define PROGRAM_IDX 0x20
#define VID_MAP_IDX 0x21
#define MAX_ENTRIES 1024 /*1024 Number of pages in a directory or max directory entries*/
#define KB_ALIGN 0x1000  /* Used to align to 4KB*/
/* Set up page directory and table structures*/

#define KB4OFFSET 12
#define MB4OFFSET 22

/* Masks */

#define SET_READ_WRITE 0x00000002
#define SET_PRESENT 0x00000001
#define SET_SIZE 0x00000080
#define SET_USER_SUPERVISOR 0x00000004
#define SET_GLOBAL 0x00000100
#define SET_VID (SET_PRESENT | SET_READ_WRITE | SET_USER_SUPERVISOR)

#define VIDEO_IDX (VIDEO / 0x1000)




// extern page_table_4KB page_table[MAX_ENTRIES] __attribute__((aligned(KB_ALIGN)));

extern unsigned int page_table[MAX_ENTRIES] __attribute__((aligned(KB_ALIGN)));

extern unsigned int page_directory[MAX_ENTRIES] __attribute__((aligned(KB_ALIGN)));



extern void init_paging();

extern void page_allocator(int32_t p_num);

extern void page_vid_map(int32_t terminal);

void reset_vid_map(int32_t terminal);

#endif /* _PAGING_H */

