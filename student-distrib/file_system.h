/* file_system.h - header file for file_system.c*/

#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "x86_desc.h"
#include "lib.h"
#include "types.h"
#include "multiboot.h"



typedef struct{
	
	uint32_t length; // length of a particular file in bytes
	uint32_t data_block_idx[1023]; // holds a number reference to a particular data block

}inode;

typedef struct{
	
	uint8_t f_name[32];
	uint32_t f_type;
	uint32_t inode_num;
	uint8_t reserved[24];

} dentry_t;


typedef struct{
	
	uint32_t num_dentries;
	uint32_t num_inodes;
	uint32_t num_data_blocks;
	uint8_t reserved[52];
} system_stats;


inode* fs_inodes;
	


extern void fs_init(module_t module);

extern int32_t fs_load(const uint8_t* fname, uint32_t address);

extern int32_t fs_open(module_t module);

extern int32_t fs_read(const uint8_t* fname, uint8_t * buf, uint32_t offset, uint32_t nbytes);



extern int32_t fs_close();

extern int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
extern int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

extern int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
extern int32_t file_open(const uint8_t* filename);
extern int32_t file_read(int32_t fd, uint8_t* buf, int32_t nbytes);



extern int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

extern int32_t file_close(int32_t fd);

extern int32_t dir_open( const uint8_t* filename);


extern int32_t dir_read(int32_t fd, uint8_t * buf, int32_t nbytes);



extern int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

extern int32_t dir_close(int32_t fd);

#endif /* _FILE_SYSTEM_H */

