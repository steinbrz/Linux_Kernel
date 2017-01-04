/* paging.c - c file used to implement paging
 * vim:ts=4 noexpandtab
 */
#include "paging.h"
#include "lib.h"

unsigned int page_table[MAX_ENTRIES] __attribute__((aligned(KB_ALIGN)));
unsigned int video_table[MAX_ENTRIES] __attribute__((aligned(KB_ALIGN)));
unsigned int page_directory[MAX_ENTRIES] __attribute__((aligned(KB_ALIGN)));

/* init_paging
 * DESCRIPTION: sets up the page table, page directory, and video memory pages
 * INPUTS: none
 * OUTPUTS: sets registers with values to enable paging
 * RETURN VALUE: none
 * SIDE EFFECTS: Paging will be set up, with addresses stored in the correct registers
 */
void init_paging()
{
		int i;
	    unsigned int temp;
    	unsigned int* page_dir_addr;

	    for( i = 0; i < MAX_ENTRIES; i++){
	    	page_table[i] = SET_READ_WRITE;
	    	page_table[i] = page_table[i] | i << KB4OFFSET;
	    	video_table[i] = SET_READ_WRITE;
	    	video_table[i] = video_table[i] | i << KB4OFFSET;
	    }

	    page_table[VIDEO_IDX] = SET_PRESENT | SET_USER_SUPERVISOR | page_table[VIDEO_IDX];

	    video_table[VIDEO1_IDX] = SET_VID | VIDEO;
	    video_table[VIDEO2_IDX] = SET_VID | VIDEO2_PHYS;
	    video_table[VIDEO3_IDX] = SET_VID | VIDEO3_PHYS;

	    temp = (int)page_table;

	    page_directory[0] = SET_PRESENT | SET_READ_WRITE | SET_USER_SUPERVISOR;
	    page_directory[0] = page_directory[0] | (temp & 0xFFFFF000);

	    
	    page_directory[1] = temp | SET_GLOBAL | SET_SIZE | SET_PRESENT | SET_READ_WRITE | 1 << MB4OFFSET;

	    page_directory[VIDEO_DIRECTORY_IDX] = ((int)video_table & 0xFFFFF000) | SET_USER_SUPERVISOR | SET_READ_WRITE | SET_PRESENT;

	    page_dir_addr = (unsigned int*)page_directory;

	    set_registers(page_dir_addr);

}

/* page_allocator
 * DESCRIPTION: maps process's virtual memory to a certain page based off the process number
 * INPUTS: p_num - process number to be set
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: changes page_directory entry to hold the new process's page
 */
void page_allocator(int32_t p_num){

		unsigned int* page_dir_addr;  

	    page_directory[PROGRAM_IDX] = 0x00000000;
	    page_directory[PROGRAM_IDX] = page_directory[PROGRAM_IDX] | SET_SIZE | SET_PRESENT | SET_USER_SUPERVISOR| SET_READ_WRITE | (p_num + 2) << MB4OFFSET;

	    //page_helper(page_directory);

	    page_dir_addr = (unsigned int*)page_directory;

	    set_registers(page_dir_addr);
}


/* page_vid_map
 * DESCRIPTION: maps the given terminal's video page to be the active video memory
 * INPUTS: terminal - 0, 1, or 2 depending on which terminal is now active
 * OUTPUTS: video_table has 2 flags set for the correct entry
 * RETURN VALUE: none
 * SIDE EFFECTS: changes active video page
 */
void page_vid_map(int32_t terminal){

    
 
	unsigned int* page_dir_addr; 


	switch(terminal){
		case 0:
			video_table[VIDEO1_IDX] = VIDEO | SET_VID;
			break;
		case 1:
			video_table[VIDEO2_IDX] = VIDEO | SET_VID; 
			break;
		case 2: 
			video_table[VIDEO3_IDX] = VIDEO | SET_VID;
			break;
		default:
			break;
	}


	page_dir_addr = (unsigned int*)page_directory;

	set_registers(page_dir_addr);
	




}

/* reset_vid_map
 * DESCRIPTION: reset all the virtual video memory pages
 * INPUTS: terminal - terminal index number (0, 1, or 2)
 * OUTPUTS: video table entries have a flag reset
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
void reset_vid_map(int32_t terminal){


			
			

	video_table[VIDEO1_IDX] = VIDEO1_PHYS | SET_VID;
	video_table[VIDEO2_IDX] = VIDEO2_PHYS | SET_VID;
	video_table[VIDEO3_IDX] = VIDEO3_PHYS | SET_VID;

}

