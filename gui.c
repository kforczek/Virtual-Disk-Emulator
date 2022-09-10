#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Helper for getting one char and ingoring rest of the input */
char get_one_char()
{
    char rest, c = getchar();

    if (c != '\n')
        while (rest = getchar(), rest != '\n' && rest != EOF); /* Ignore any following input */

    return c;
}

/* Helper for removing trailing newline from input */
void str_trim(char *buf)
{
    char *c = buf;
    while (*c != '\0') c++; /* Find end of string */

    /* Replace trailing newline or EOF with \0 */
    while (--c >= buf && (*c == '\n' || *c == EOF)) *c = '\0';
}

/* Helper for getting original file index
 * from the user in a friendly way */
int input_index(vdisk_t *vdisk_fp)
{
    char c;
    do
    {
        printf("How do you want to select the file?\n");
        printf("1 - Choose from the list\n");
        printf("2 - Type its name\n> ");
        c = get_one_char();
    }
    while (c != '1' && c != '2');

    if (c == '1')
    {
        int i, cnt;
        char i_raw[6];
        do
        {
            cnt = gui_list_files(vdisk_fp); /* Note that print/input index is i+1 */
            printf("Choose index: > ");

            fgets(i_raw, 6, stdin);
            str_trim(i_raw);
            i = strtol(i_raw, NULL, 10);
        }
        while (--i < 0 || i >= cnt);

        return i;
    }
    else
    {
        char file_name[MAX_FNAME_LENGTH];

        printf("Name of the file: > ");
        fgets(file_name, MAX_FNAME_LENGTH, stdin);
        str_trim(file_name);

        return get_file_index(vdisk_fp, file_name);
    }
}

int main_menu()
{
    char c = ' ';
    vdisk_t *vdisk_fp = NULL;

    printf("SOI Lab 6 - Filesystem\n");
    while (c != CHR_EXIT)
    {
        int repeat;

        printf("MAIN MENU\n\n");
        printf("%c - Create virtual disk\n", CHR_CREATE_DISK);
        printf("%c - Open virtual disk\n", CHR_OPEN);
        printf("%c - Copy file to virtual disk\n", CHR_COPY_FILE);
        printf("%c - Get file from virtual disk\n", CHR_GET_FILE);
        printf("%c - Delete file from virtual disk\n", CHR_DEL_FILE);
        printf("%c - List files on virtual disk\n", CHR_LIST_FILES);
        printf("%c - Print memory info\n", CHR_MEM_INFO);
        printf("%c - Defragment virtual disk\n", CHR_DEFRAGMENT);
        printf("%c - Delete virtual disk\n", CHR_DEL_DISK);
        printf("%c - Exit program\n\n", CHR_EXIT);

        do {
            repeat = 0;
            printf("Make a choice: > ");

            c = get_one_char();

            switch (tolower(c)) {
                case CHR_CREATE_DISK:
                    gui_create_disk();
                    break;
                case CHR_OPEN:
                    if (vdisk_fp != NULL)
                        close_disk(vdisk_fp);
                    vdisk_fp = gui_open_disk();
                    break;
                case CHR_COPY_FILE:
                    gui_put_file(vdisk_fp);
                    break;
                case CHR_GET_FILE:
                    gui_get_file(vdisk_fp);
                    break;
                case CHR_DEL_FILE:
                    gui_delete_file(vdisk_fp);
                    break;
                case CHR_LIST_FILES:
                    gui_list_files(vdisk_fp);
                    break;
                case CHR_MEM_INFO:
                    if (vdisk_fp == NULL)
                        printf("Disk must be opened first!\n");
                    else
                        gui_mem_info(vdisk_fp);
                    break;
                case CHR_DEFRAGMENT:
                    gui_defragment(vdisk_fp);
                    break;
                case CHR_DEL_DISK:
                    if (vdisk_fp != NULL) {
                        close_disk(vdisk_fp);
                        vdisk_fp = NULL;
                    }
                    gui_delete_disk();
                    break;
                case CHR_EXIT:
                    if (vdisk_fp != NULL) {
                        close_disk(vdisk_fp);
                        vdisk_fp = NULL;
                    }
                    printf("Goodbye!\n");
                    break;
                default:
                    printf("This is not a correct choice. Try again.\n");
                    repeat = 1;
                    break;
            }
        }
        while (repeat);

        if (c != CHR_EXIT) {
            printf("Press ENTER to continue\n");
            get_one_char();
        }
    }
    return 0;
}

void gui_create_disk()
{
    char file_path[MAX_PATH_LENGTH], size_raw[20];
    off_t size;

    printf("The disk will be saved in a file of a given size.\n");
    printf("Provide a path to the disk file: > ");

    fgets(file_path, MAX_PATH_LENGTH, stdin);
    str_trim(file_path);

    printf("Size of the disk (in bytes): > ");
    fgets(size_raw, 20, stdin);
    str_trim(size_raw);
    size = strtol(size_raw, NULL, 10);

    printf("\nCreating disk... ");
    fflush(stdout);

    switch (create_disk(file_path, size))
    {
        case 0:
            printf("Disk created!\n");
            break;
        case 1:
            printf("Error: is the path correct?\n");
            break;
        case 2:
            printf("Error: given size less than minimum\n");
            break;
        case 3:
            printf("Error: is there enough space?\n");
            break;
        case 4:
            printf("Error: file with a given name already exists\n");
    }
}

vdisk_t *gui_open_disk()
{
    char disk_path[MAX_PATH_LENGTH];
    vdisk_t *vdisk_fp;

    printf("Path to the disk: > ");
    fgets(disk_path, MAX_PATH_LENGTH, stdin);
    str_trim(disk_path);

    printf("\nOpening disk... ");
    fflush(stdout);

    vdisk_fp = open_disk(disk_path);

    if (vdisk_fp == NULL) printf("Failed: is the path correct?\n");
    else printf("Disk opened!\n");

    return vdisk_fp;
}

void gui_put_file(vdisk_t *vdisk_fp)
{
    char file_path[MAX_PATH_LENGTH];

    if (vdisk_fp == NULL)
    {
        printf("Disk must be opened first!\n");
        return;
    }

    printf("Path to the file to be copied: > ");
    fgets(file_path, MAX_PATH_LENGTH, stdin);
    str_trim(file_path);

    printf("\nCopying file...\n");
    fflush(stdout);

    switch (put_file(vdisk_fp, file_path))
    {
        case 0:
            printf("File copied!\n");
            break;
        case 1:
            printf("Error: insufficient space on disk\n");
            break;
        case 2:
            printf("Error: file limit reached on the disk\n");
            break;
        case 3:
            printf("Error: is the path correct?\n");
            break;
        case 4:
            printf("Error: file with the same name already exists on the disk\n");
            break;
    }
}

void gui_get_file(vdisk_t *vdisk_fp)
{
    char folder_path[MAX_PATH_LENGTH];
    int index;

    if (vdisk_fp == NULL)
    {
        printf("Disk must be opened first!\n");
        return;
    }

    index = input_index(vdisk_fp);

    printf("Input path to the folder where file will be stored.\n> ");
    fgets(folder_path, MAX_PATH_LENGTH, stdin);
    str_trim(folder_path);

    printf("Copying file... ");
    fflush(stdout);

    switch (get_file(vdisk_fp, index, folder_path))
    {
        case 0:
            printf("File copied!\n");
            break;
        case 1:
            printf("Error: file with this name already exists in the given folder\n");
            break;
        case 2:
            printf("Error: file index is incorrect\n");
            break;
        case 3:
            printf("Error: provided path is incorrect\n");
            break;
    }
}

void gui_delete_file(vdisk_t *vdisk_fp)
{
    char conf, rest;
    int index;

    if (vdisk_fp == NULL)
    {
        printf("Disk must be opened first!\n");
        return;
    }

    index = input_index(vdisk_fp);

    printf("Are you sure you want to delete the file from the virtual disk? This cannot be undone. (y/n) > ");
    conf = get_one_char();
    if (tolower(conf) != 'y') {
        printf("OK.\n");
        return;
    }

    printf("Deleting file... ");
    fflush(stdout);

    if (delete_file(vdisk_fp, index) == 0) printf("File deleted.\n");
    else printf("Error: file with a given name doesn't exist\n");
}

int gui_list_files(vdisk_t *vdisk_fp)
{
    struct file_list list;
    int i;

    if (vdisk_fp == NULL)
    {
        printf("Disk must be opened first!\n");
        return -1;
    }

    get_file_list(vdisk_fp, &list);

    printf("\nFile count: %d\n\n", list.file_count);

    /* Print table header */
    printf("INDEX |");
    for (i = 0; i < (MAX_FNAME_LENGTH-7)/2; i++) printf(" ");
    printf("FILE NAME");
    for (i = 0; i < ((MAX_FNAME_LENGTH-7)/2 + (MAX_FNAME_LENGTH-1)%2); i++) printf(" ");
    printf("| SIZE (B)\n");


    for (i = 0; i < list.file_count; i++) {
        printf("%5d |", i+1);
        printf(" %*s |", MAX_FNAME_LENGTH, list.files[i].file_name);
        printf(" %ld\n", list.files[i].file_size);
    }

    printf("\n");

    return list.file_count;
}

void gui_mem_info(vdisk_t *vdisk_fp)
{
    struct region_info *regions;
    int reg_cnt, i;
    off_t free_sp = 0, disk_size = get_disk_size(vdisk_fp), free_ratio;

    /* Allocate enough memory for the regions info */
    regions = malloc(sizeof(struct region_info) * max_reg_cnt(vdisk_fp));

    /* Load all region structs into array */
    reg_cnt = get_mem_info(vdisk_fp, regions);

    printf("Region list\n\n");

    /* Print table header */
    printf("  ADDRESS  |    SIZE (B)    |  TYPE\n");

    /* Output every region */
    for (i = 0; i < reg_cnt; i++)
    {
        printf(" %9lx |", regions[i].offset);
        printf(" %14ld | ", regions[i].size);
        if (regions[i].purpose == REG_FREE)
        {
            printf("Free space\n");
            free_sp += regions[i].size;
        }
        else if (regions[i].purpose == REG_DISKHDR)
            printf("Disk header\n");
        else if (regions[i].purpose == REG_FILEHDR)
            printf("File header\n");
        else
            printf("File data\n");
    }

    free_ratio = free_sp * 100 / disk_size;

    /* Print statistics */
    printf("\nTotal disk size: %ld B\n", disk_size);
    printf("Occupied space: %ld B (%ld %%)\n", disk_size-free_sp, 100-free_ratio);
    printf("Free space: %ld B (%ld %%)\n\n", free_sp, free_ratio);

    free(regions);
}

void gui_defragment(vdisk_t *vdisk_fp)
{
    char c;

    if (vdisk_fp == NULL)
    {
        printf("Disk must be opened first!\n");
        return;
    }

    printf("Confirm defragmentation? (y/n) > ");
    c = tolower(get_one_char());
    if (c != 'y')
    {
        printf("OK.\n");
        return;
    }

    printf("\nDefragmenting... ");
    fflush(stdout);

    switch (defragment(vdisk_fp, DEMO))
    {
        case 0:
            printf("Defragmentation successful!\n");
            break;
        case 1:
            printf("Error: unable to defragment disk\n");
            break;
    }
}

void gui_delete_disk()
{
    char disk_path[MAX_PATH_LENGTH], conf, rest;

    printf("Path to the disk: > ");
    fgets(disk_path, MAX_PATH_LENGTH, stdin);
    str_trim(disk_path);

    printf("\nAre you sure you want to delete this disk file? This operation cannot be undone. (y/n) > ");
    conf = get_one_char();
    if (tolower(conf) != 'y') {
        printf("OK.\n");
        return;
    }

    printf("\nDeleting file... ");
    fflush(stdout);

    if (delete_disk(disk_path) == 0)
        printf("File deleted!\n");
    else printf("Error: cannot delete file\n");
}
