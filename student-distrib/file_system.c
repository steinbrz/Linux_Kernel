/* file_system.c - Contains routines to initiate the file system driver and interact*/

#include "file_system.h"
#include "system_calls.h"

#define FS_BLOCK 4096 // There are 4kb or 4096 bytes per block 
#define MAX_DENTRIES 63
#define MEM_LOC_SIZE 64
#define IN_USE 1
#define NOT_IN_USE 0
#define FSOPEN 1
#define FSCLOSED 0
#define FNAMESIZE 32

dentry_t* fs_dentries;
system_stats* fs_stats;

uint8_t* fs_data_blocks;
uint32_t file_position;
uint32_t n_inodes;
uint32_t n_dentries;
uint32_t n_data_blocks;
int32_t fs_is_open = 0;

int32_t current_dentry = 0;




/*
*  fs_init
*	 DESCRIPTION: Initializes pointers to values in the file system
*	 INPUTS: Module that points to the file system
*	 OUTPUTS: Sets fs_dentries, fs_stats, and fs_data_blocks pointers
*	 RETURN VALUE: none
*	 SIDE EFFECTS: Sets statistic values for the file system
*/

void fs_init(module_t module){
	
	dentry_t* fs_dentry_start;
	inode* fs_inode_start;
	uint8_t* fs_data_blocks_start;


	fs_stats = (system_stats*)module.mod_start;

	
	n_dentries = fs_stats->num_dentries;
	n_inodes = fs_stats->num_inodes;
	n_data_blocks = fs_stats->num_data_blocks;

	fs_dentry_start = (dentry_t*)(module.mod_start + MEM_LOC_SIZE);
	fs_inode_start = (inode*)(module.mod_start + FS_BLOCK);
	fs_data_blocks_start = (uint8_t*)(module.mod_start + (n_inodes + 1) * FS_BLOCK);

	fs_dentries = fs_dentry_start;
	fs_inodes = fs_inode_start;
	fs_data_blocks = fs_data_blocks_start;


}

/*
*  fs_load
*	 DESCRIPTION: Loads file system data into user memory
*	 INPUTS: File name and virtual address of the user space
*	 OUTPUTS: none
*	 RETURN VALUE: 0 on success and -1 on failure
*	 SIDE EFFECTS: none
*/


int32_t fs_load(const uint8_t* fname, uint32_t address){

	dentry_t temp_dentry;
	if(fname == NULL)
		return -1;
	int32_t error;
	error = read_dentry_by_name(fname, &temp_dentry);
	if(error == -1){

		return -1;
	}

	if( -1 == read_data(temp_dentry.inode_num, 0, (uint8_t*) 0x08048000, fs_inodes[temp_dentry.inode_num].length)){

		return -1;
	}

	return 0;
}

/*
*  fs_read
*	 DESCRIPTION: Reads file system, particulary used to find entry point in system execute
*	 INPUTS: File name, buffer to write into, offset within the file system, and number of bytes to read
*	 OUTPUTS: Writes to the buffer that is provided as a parameter
*	 RETURN VALUE: Number of bytes read on success and -1 on failure
*	 SIDE EFFECTS: none
*/

int32_t fs_read(const uint8_t* fname, uint8_t * buf, uint32_t offset, uint32_t nbytes){

	dentry_t temp_dentry;
	if(fname == NULL){

		return -1;
	}

	if(  read_dentry_by_name(fname, &temp_dentry) == -1){

		return -1;
	}

	return read_data(temp_dentry.inode_num, offset, buf, nbytes );
}

/*
*  fs_open
*	 DESCRIPTION: Opens file system and calls fs_init
*	 INPUTS: Module that points to the file system
*	 OUTPUTS: none
*	 RETURN VALUE: 0 on success and -1 on failure
*	 SIDE EFFECTS: Sets FSOPEN flag to open
*/

int32_t fs_open(module_t module){

	if(fs_is_open == FSCLOSED){
		fs_init(module);
		fs_is_open = FSOPEN;
		return 0;


	}
	else
		return -1;
}

/*
*  fs_close
*	 DESCRIPTION: Closes file system
*	 INPUTS: none
*	 OUTPUTS: none
*	 RETURN VALUE: 0 on success and -1 on failure
*	 SIDE EFFECTS: Sets FSOPEN flag to closed
*/

int32_t fs_close(){

	if(fs_is_open == FSOPEN){

		fs_is_open = FSCLOSED;
		return 0;
	}
	else
		return -1;
}

/*
*  read_dentry_by_name
*	 DESCRIPTION: Looks for directory entry associated with the file name
*	 INPUTS: A string of the file name and a pointer to a dentry structure
*	 OUTPUTS: Copies correct dentry to the dentry pointer passed
*	 RETURN VALUE: 0 on success and -1 on failure
*	 SIDE EFFECTS: None
*/



int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){

	int i, j;

	uint32_t length;

	
	length = strlen((char*)fname);

	

	for( i = 0; i < n_dentries; i++){

		if(length == strlen((char*)fs_dentries[i].f_name)){

			if(0 == strncmp((int8_t*)fname, (int8_t*)fs_dentries[i].f_name, length)){
				for( j = 0; j < FNAMESIZE; j++){

				dentry->f_name[j] = fs_dentries[i].f_name[j];

				}
				dentry->f_type = fs_dentries[i].f_type;
				dentry->inode_num = fs_dentries[i].inode_num;
				return 0;
			}
		}


		
	}

	return -1;


}

/*
*  read_dentry_by_index
*	 DESCRIPTION: Looks for directory entry associated with the directory entry index
*	 INPUTS: The directory entry index and a pointer to a dentry structure
*	 OUTPUTS: Copies correct dentry to the dentry pointer passed
*	 RETURN VALUE: 0 on success and -1 on failure
*	 SIDE EFFECTS: None
*/


int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
	int i;

	if(index < n_dentries){
				for(i = 0; i < FNAMESIZE; i++){
				dentry->f_name[i] = fs_dentries[index].f_name[i];
			}
				dentry->f_type = fs_dentries[index].f_type;
				dentry->inode_num = fs_dentries[index].inode_num;
				return 0;

	}

	else
		return -1;




}

/*
*  read_data
*	 DESCRIPTION: Reads the data for a file provided by a particular inode
*	 INPUTS: Index to the correct inode, offset within the file_system, a buffer to write to, and the amount of bytes to copy
*	 OUTPUTS: Writes file data to the passed buffer
*	 RETURN VALUE: Number of bytes read on success and -1 on failure
*	 SIDE EFFECTS: None
*/

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){

	int i;
	i = 0;

	

	uint32_t current; /*used to keep track of current data block*/
	uint32_t current_idx; /* keeps track of current data block # specified in inode*/
	uint32_t inode_length; /* used to keep track of the length of the file in bites*/
	uint32_t ret_bytes; /*keep track of the number of bytes that are copied to the buffer*/
	uint32_t offset_in_block; /*Offset within the block*/
	

	/*check if inode # exists*/

	if(inode >= n_inodes){

		return -1;
	}
	//finds number of bytes in fyle
	inode_length = fs_inodes[inode].length; 

	/* Check if the offset is out of bounds*/

	if(offset > inode_length){

		return 0;

	}

	
	/*calculate the data block and location within in the block to start at*/
	current = (offset / FS_BLOCK);
	offset_in_block = (offset % FS_BLOCK); 

	/* copy bytes of data blocks to buffer*/

	current_idx = (FS_BLOCK * fs_inodes[inode].data_block_idx[current]) + offset_in_block;
	
	
	if((length > (FS_BLOCK - offset_in_block)) && ((FS_BLOCK - offset_in_block + offset) <= inode_length)){
		memcpy((void*)(buf), (void*)(fs_data_blocks + current_idx), FS_BLOCK - offset_in_block);
		ret_bytes = FS_BLOCK - offset_in_block;
	}

	else if((length == FS_BLOCK - offset_in_block) && (length < (inode_length - offset))){

		memcpy((void*)(buf), (void*)(fs_data_blocks + current_idx), length);
		return length;
	}


	else if((length < (inode_length - offset)) && ((length + offset_in_block) < FS_BLOCK)){
		memcpy((void*)(buf), (void*)(fs_data_blocks + current_idx), length);
		return length;
	}

	else{
		memcpy((void*)(buf), (void*)(fs_data_blocks + current_idx), inode_length - offset);
		return (inode_length - offset);
	}

	current++;
	current_idx = (FS_BLOCK * fs_inodes[inode].data_block_idx[current]);

	while(ret_bytes < length){


		if( (length > FS_BLOCK + ret_bytes) && (inode_length > FS_BLOCK + ret_bytes)){
		memcpy((void*)(buf + ret_bytes), (void*)(fs_data_blocks + current_idx), FS_BLOCK);
		ret_bytes += FS_BLOCK;
		current++;
		current_idx = (FS_BLOCK * fs_inodes[inode].data_block_idx[current]);

		}
		else if ((length + offset) < inode_length  && (length - ret_bytes) < FS_BLOCK){
		memcpy((void*)(buf + ret_bytes), (void*)(fs_data_blocks + current_idx), length - ret_bytes);
		return length;


		}
		else{
		memcpy((void*)(buf + ret_bytes), (void*)(fs_data_blocks + current_idx), inode_length - ret_bytes - offset);
		return (inode_length - offset);


		}
	}

	return ret_bytes;


}

/*
*  file_open
*	 DESCRIPTION: Not used, files are opened in system_calls.c
*	 INPUTS: File name
*	 OUTPUTS: none
*	 RETURN VALUE: -1
*	 SIDE EFFECTS: None
*/


int32_t file_open(const uint8_t* filename){

	return -1;


}

/*
*  file_read
*	 DESCRIPTION: Reads the data of a particular file given by the file directory
*	 INPUTS: File direcotry array index, buffer to write to, and number of bytes read
*	 OUTPUTS: Copies file data to passed buffer
*	 RETURN VALUE: Number of bytes read on success and -1 on failure
*	 SIDE EFFECTS: None
*/

int32_t file_read(int32_t fd, uint8_t* buf, int32_t nbytes){


	//dentry_t temp_dentry;

	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));

	int32_t ret_bytes;

	if(control_block->fd_array[fd].inode_ptr == -1){

		return -1;
	}

	if( control_block->fd_array[fd].position == fs_inodes[control_block->fd_array[fd].inode_ptr].length){

		return 0;
	}


	ret_bytes = read_data(control_block->fd_array[fd].inode_ptr, control_block->fd_array[fd].position, buf, nbytes);

	control_block->fd_array[fd].position += ret_bytes;


	return ret_bytes;





}

/*
*  file_write
*	 DESCRIPTION: Files are read only so file_write returns -1
*	 INPUTS: File directory array index, character buffer, and bytes to write
*	 OUTPUTS: none
*	 RETURN VALUE: -1
*	 SIDE EFFECTS: None
*/

int32_t file_write(int32_t fd, const void* buf, int32_t nbytes){

	return -1;
}

/*
*  file_write
*	 DESCRIPTION: Files are close in the sys_close call in system_calls.c
*	 INPUTS: File directory array index
*	 OUTPUTS: none
*	 RETURN VALUE: -1
*	 SIDE EFFECTS: None
*/

int32_t file_close(int32_t fd){

	return -1;
}
/*
*  dir_open
*	 DESCRIPTION: Direcotry is opened in sys_open call in system_calls.c
*	 INPUTS: Character buffer of file name
*	 OUTPUTS: none
*	 RETURN VALUE: -1
*	 SIDE EFFECTS: None
*/
int32_t dir_open(const uint8_t* filename){



	return -1;



}


/*
*  dir_read
*	 DESCRIPTION: Reads successive directory entries
*	 INPUTS: File directory array index, character buffer, and bytes to write
*	 OUTPUTS: none
*	 RETURN VALUE: Bytes read on success and -1 on failure
*	 SIDE EFFECTS: None
*/



int32_t dir_read(int32_t fd, uint8_t* buf, int32_t nbytes){

	
	dentry_t temp_dentry;

	PCB_struct * control_block;
	control_block = (PCB_struct*)((KERNEL_MEM_BOTTOM) - (KB8 * (executing_process + 2)));

	if(control_block->fd_array[fd].position >= n_dentries){

		return 0;
	}


	read_dentry_by_index(control_block->fd_array[fd].position, &temp_dentry);




	memcpy((void*)(buf), (void*)(&temp_dentry), FNAMESIZE);

	control_block->fd_array[fd].position++;	

	
	return strlen((int8_t*)buf);

}

/*
*  dir_write
*	 DESCRIPTION: Directories are read only so file_write returns -1
*	 INPUTS: File directory array index, character buffer, and bytes to write
*	 OUTPUTS: none
*	 RETURN VALUE: -1
*	 SIDE EFFECTS: None
*/

int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes){

	return -1;
}

/*
*  dir_close
*	 DESCRIPTION: Directories are closed by sys_close in system_calls.c
*	 INPUTS: File directory array index
*	 OUTPUTS: none
*	 RETURN VALUE: -1
*	 SIDE EFFECTS: None
*/

int32_t dir_close(int32_t fd){

return -1;


}






