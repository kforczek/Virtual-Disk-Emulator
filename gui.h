#ifndef SOILAB6_GUI_H
#define SOILAB6_GUI_H

#include "filesystem.h"

#define MAX_PATH_LENGTH 50

#define CHR_CREATE_DISK '1'
#define CHR_OPEN '2'
#define CHR_COPY_FILE '3'
#define CHR_GET_FILE '4'
#define CHR_DEL_FILE '5'
#define CHR_LIST_FILES '6'
#define CHR_MEM_INFO '7'
#define CHR_DEFRAGMENT '8'
#define CHR_DEL_DISK '9'
#define CHR_EXIT 'e'

/* Prints main menu of the GUI
 * and handles selected operations
 * in a loop until exit option
 * is chosen */
int main_menu();

/* Handles virtual disk creation */
void gui_create_disk();

/* Handles opening virtual disk,
 * returns a pointer to the stream */
vdisk_t *gui_open_disk();

/* Handles copying file to the virtual disk */
void gui_put_file(vdisk_t *vdisk_fp);

/* Handles getting file from the virtual disk */
void gui_get_file(vdisk_t *vdisk_fp);

/* Handles deleting file from the virtual disk */
void gui_delete_file(vdisk_t *vdisk_fp);

/* Prints all files on the virtual disk
 * with pretty indexes (print index is
 * original index + 1). Returns file count. */
int gui_list_files(vdisk_t *vdisk_fp);

/* Handles displaying information about
 * memory regions of the virtual disk */
void gui_mem_info(vdisk_t *vdisk_fp);

/* Handles defragmentation of the virtual disk */
void gui_defragment(vdisk_t *vdisk_fp);

/* Handles deleting virtual disk */
void gui_delete_disk();

#endif /* SOILAB6_GUI_H */
