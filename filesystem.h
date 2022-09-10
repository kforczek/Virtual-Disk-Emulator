#ifndef SOILAB6_FILESYSTEM_H
#define SOILAB6_FILESYSTEM_H

#include <stdio.h>
#include <sys/types.h>

#define FILL_BYTE '0'
#define MAX_FILES 20
#define MAX_FNAME_LENGTH 30

#define DEMO 1
#define NO_DEMO 0
#define LOAD_BAR_SIZE 20
#define LOAD_CHAR "."

typedef FILE vdisk_t;
typedef int reg_t;

enum reg_t
{
    REG_FREE,
    REG_DISKHDR,
    REG_FILEHDR,
    REG_FILEDATA
};

struct disk_header
{
    short file_count;
    off_t file_offsets[MAX_FILES];
};

struct file_header
{
    off_t file_size; /* with header */
    char file_name[MAX_FNAME_LENGTH+1];
};

struct file_list
{
    int file_count;
    struct file_header files[MAX_FILES];
};

struct region_info
{
    off_t offset, size;
    reg_t purpose;
};


/* Create virtual disk as a file defined by file_path,
 * of given size in bytes */
int create_disk(const char *file_path, off_t size);


/* Open the disk and get pointer to it */
vdisk_t *open_disk(const char *file_path);


/* Close disk of given pointer */
int close_disk(vdisk_t *vdisk_fp);


/* Copy file from given path into
 * virtual disk pointed to by vdisk_fp */
int put_file(vdisk_t *vdisk_fp, const char *file_path);


/* Get file called file_name from virtual disk to dest_path */
int get_file(vdisk_t *vdisk_fp, int file_index, const char *dest_path);


/* Returns index of the file called file_name
 * on the disk or -1 if non-existent */
int get_file_index(vdisk_t *vdisk_fp, const char *file_name);


/* Loads file list structure pointed to by list_ptr
 * with data from virtual disk pointed to by vdisk_fp.
 * The file headers are in original order, hence an index
 * of the files array from file_list structure is equal to
 * file_index argument in get_file() and delete_file() methods. */
int get_file_list(vdisk_t *vdisk_fp, struct file_list *list_ptr);


/* Returns maximum region count that can be expected
 * to be used by get_mem_info() */
int max_reg_cnt(vdisk_t *vdisk_fp);


/* Saves info about existing memory regions in array pointed to
 * by regions_ptr and returns number of these regions.
 * To allocate enough memory in advance, see max_reg_cnt(). */
int get_mem_info(vdisk_t *vdisk_fp, struct region_info *regions_ptr);


/* Returns size of the virtual disk */
off_t get_disk_size(vdisk_t *vdisk_fp);


/* Delete file from the disk using its index */
int delete_file(vdisk_t *vdisk_fp, int file_index);


/* Delete the disk file */
int delete_disk(const char *file_path);


/* Defragment the disk */
int defragment(vdisk_t *vdisk_fp, int demo);


#endif /* SOILAB6_FILESYSTEM_H */
