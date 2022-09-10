#include "filesystem.h"
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

typedef struct {
    int size;
    int byte_cnt;
    int chr_cnt;
    char print_chr[5];
    off_t part;
} load_bar;

/* Helper for getting file sizes */
off_t get_stream_size(FILE *fp)
{
    struct stat buf;
    int fd = fileno(fp); /* get descriptor of the stream */
    fstat(fd, &buf); /* get file attributes into buf */
    return buf.st_size;
}

/* Helper for extracting file name from path */
off_t get_filename_offset(char *filepath)
{
    off_t off;
    char *c = filepath;

    while (*c != '\0')
    {
        if (*c == SEPARATOR) off = c - filepath + 1;
        c++;
    }
    return off;
}

/* Helper for loading disk header into memory */
void load_disk_hdr(vdisk_t *vdisk_fp, struct disk_header *disk_hdr)
{
    fseek(vdisk_fp, 0, SEEK_SET);
    fread(disk_hdr, sizeof(struct disk_header), 1, vdisk_fp);
}

/* Helper for creating a loading bar and printing its header */
load_bar *load_bar_init(off_t file_size)
{
    load_bar *lb = malloc(sizeof(load_bar));
    int i;

    lb->size = LOAD_BAR_SIZE;
    lb->byte_cnt = 0;
    lb->chr_cnt = 0;
    strcpy(lb->print_chr, LOAD_CHAR);
    lb->part = (file_size < lb->size) ? file_size : file_size / lb->size;

    printf("\n0%% |");
    for (i = 0; i < lb->size; i++) printf(" ");
    printf("| 100%%\n");
    for (i = 0; i < 3; i++) printf(" ");
    printf("|");

    if (lb->part == file_size)
        for (i = 0; i < lb->size - 1; i++)
            printf(lb->print_chr);

    fflush(stdout);

    return lb;
}

/* Helper for incrementing a loading bar */
void *load_bar_increment(load_bar *lb)
{
    if (++lb->byte_cnt % lb->part == 0 && lb->chr_cnt < lb->size)
    {
        printf(lb->print_chr);
        fflush(stdout);
        lb->chr_cnt++;
    }
}

/* Helper for destroying a loading bar */
void load_bar_destroy(load_bar *lb)
{
    printf("|\n");
    fflush(stdout);
    free(lb);
}

/* Helper for copying file from one stream to another */
void file_cp(FILE *source, FILE *dest, off_t file_size, int demo)
{
    char buf;
    off_t cnt = 0;
    load_bar *lb;

    if (demo == DEMO) lb = load_bar_init(file_size);

    /* Read source and save to dest byte by byte
     * until file_size reached or end of stream */
    while (cnt++ < file_size && fread(&buf, sizeof(char), 1, source))
    {
        /* Not end of source, save byte to dest */
        fwrite(&buf, sizeof(char), 1, dest);

        /* Print progress if demo is on */
        if (demo == DEMO) load_bar_increment(lb);
    }

    if (demo == DEMO) load_bar_destroy(lb);
}

int create_disk(const char *file_path, off_t size)
{
    struct disk_header hdr;
    int i;
    size_t res, hdr_size = sizeof(struct disk_header);
    FILE *tmp_fp;
    vdisk_t *fp;

    if (size < hdr_size) return 2; /* Size less than minimum */

    if ((tmp_fp = fopen(file_path, "rb")) != NULL)
    {
        fclose(tmp_fp);
        return 4; /* File already exists */
    }

    fp = fopen(file_path, "wb");
    if (fp == NULL) return 1; /* Couldn't create file under given path */

    hdr.file_count = 0;
    res = hdr_size * fwrite(&hdr, hdr_size, 1, fp);
    for (i = 0; i < size - hdr_size; i++)
    {
        if (fputc(FILL_BYTE, fp) != EOF) res++;
        else break;
    }

    fclose(fp);

    if (res == size) return 0;
    return 3; /* Probably too little space */
}

vdisk_t *open_disk(const char *file_path)
{
    return fopen(file_path, "rb+");
}

int close_disk(vdisk_t *vdisk_fp)
{
    return fclose(vdisk_fp);
}

int put_file(vdisk_t *vdisk_fp, const char *file_path)
{
    const size_t DISK_HDR_SIZE = sizeof(struct disk_header);
    const size_t FILE_HDR_SIZE = sizeof(struct file_header);

    FILE *org_fp;
    off_t vdisk_size, org_size, total_org_size;
    off_t curr_off, next_off, newfile_off, space, total_space = 0;
    struct disk_header disk_hdr;
    struct file_header file_hdr, newfile_hdr;
    char *filename;
    int i, j;

    /* Open the given file */
    org_fp = fopen(file_path, "rb");
    if (org_fp == NULL) return 3; /* Error opening file */

    filename = malloc(sizeof(char) * strlen(file_path));

    vdisk_size = get_stream_size(vdisk_fp);
    org_size = get_stream_size(org_fp);
    total_org_size = org_size + FILE_HDR_SIZE;

    /* Load disk header into memory */
    load_disk_hdr(vdisk_fp, &disk_hdr);

    /* Extract filename from the path */
    strcpy(filename, file_path);
    strcpy(filename, filename + get_filename_offset(filename));

    /* Truncate filename if too long */
    if (strlen(filename) > MAX_FNAME_LENGTH) filename[MAX_FNAME_LENGTH] = '\0';

    /* How many files are on the disk? */
    if (disk_hdr.file_count == 0)
    {
        /* If no files, all space is free expect the header */
        space = vdisk_size - DISK_HDR_SIZE;
        newfile_off = DISK_HDR_SIZE; /* Right after the header */
        i = -1; /* Preceding is just the header */
    }
    else if (disk_hdr.file_count >= MAX_FILES)
    {
        /* If all slots filled, return error */
        fclose(org_fp);
        free(filename);
        return 2; /* File limit reached */
    }
    else
    {
        /* Look for first space that is big enough */
        for (i = 0; i < disk_hdr.file_count; i++) {
            curr_off = disk_hdr.file_offsets[i];
            /* Read file header for i-th file */
            fseek(vdisk_fp, curr_off, SEEK_SET);
            fread(&file_hdr, FILE_HDR_SIZE, 1, vdisk_fp);

            if (strcmp(file_hdr.file_name, filename) == 0)
            {
                fclose(org_fp);
                free(filename);
                return 4; /* Name reserved */
            }

            /* Get where the next file starts */
            /* If this is the last file, next offset is equal to the size of entire disk - 1 */
            next_off = (i != disk_hdr.file_count - 1) ? disk_hdr.file_offsets[i + 1] : vdisk_size-1;

            /* Calculate free space between consecutive files */
            space = next_off - (curr_off + file_hdr.file_size);

            total_space += space;
            if (space >= total_org_size) /* Choose this space */
            {
                /* Chosen space begins after the end of i-th file */
                newfile_off = curr_off + file_hdr.file_size;
                break;
            }
        }
    }

    if (space < total_org_size) {

        fclose(org_fp); /* Close the given file */

        if (total_space < total_org_size) {
            free(filename);
            return 1; /* Insufficient space on disk */
        }
        else {
            defragment(vdisk_fp, NO_DEMO); /* Defragmentation will help */
            free(filename);
            return put_file(vdisk_fp, file_path); /* Try again */
        }
    }

    /* Create header for the new file */
    newfile_hdr.file_size = total_org_size;
    strcpy(newfile_hdr.file_name, filename);

    /* Set the stream pointer to the begining of this space */
    fseek(vdisk_fp, newfile_off, SEEK_SET);

    /* Save header in the beginning of free space */
    fwrite(&newfile_hdr, FILE_HDR_SIZE, 1, vdisk_fp);

    /* Save actual file after the header */
    file_cp(org_fp, vdisk_fp, org_size, DEMO);

    /* Update disk header */
    if (i+1 < disk_hdr.file_count)
    {
        /* This is not the last file, every other
         * entry after i in the array has to be moved */
        for (j = disk_hdr.file_count-1; j > i; j--)
            disk_hdr.file_offsets[j+1] = disk_hdr.file_offsets[j];
    }
    disk_hdr.file_offsets[i+1] = newfile_off;
    disk_hdr.file_count++;
    fseek(vdisk_fp, 0, SEEK_SET);
    fwrite(&disk_hdr, DISK_HDR_SIZE, 1, vdisk_fp);

    /* Close the given file */
    fclose(org_fp);

    free(filename);

    return 0;
}

int get_file(vdisk_t *vdisk_fp, int file_index, const char *dest_path)
{
    struct disk_header disk_hdr;
    struct file_header file_hdr;
    char full_path[50], last_char, separator[2] = { SEPARATOR, '\0' };
    off_t file_off;
    FILE *dest_fp;

    /* Load disk header into memory */
    load_disk_hdr(vdisk_fp, &disk_hdr);

    if (file_index < 0 || file_index >= disk_hdr.file_count)
        return 2; /* Index out of bounds */

    /* Load file header into memory */
    file_off = disk_hdr.file_offsets[file_index];
    fseek(vdisk_fp, file_off, SEEK_SET);
    fread(&file_hdr, sizeof(struct file_header), 1, vdisk_fp);

    /* Make full path by appending file name to folder path */
    strcpy(full_path, dest_path);
    last_char = dest_path[strlen(dest_path)-1];
    if (last_char != SEPARATOR) strcat(full_path, separator);
    strcat(full_path, file_hdr.file_name);

    if ((dest_fp = fopen(full_path, "rb")) != NULL)
    {
        fclose(dest_fp);
        return 1; /* File already exists */
    }

    /* Open destination stream */
    dest_fp = fopen(full_path, "wb");
    if (dest_fp == NULL) return 3; /* Failed to create file (incorrect path) */

    /* Copy file to destination */
    file_cp(vdisk_fp, dest_fp, file_hdr.file_size - sizeof(struct file_header), DEMO);

    /* Close destination stream */
    fclose(dest_fp);

    return 0;
}

int get_file_index(vdisk_t *vdisk_fp, const char *file_name)
{
    struct disk_header disk_hdr;
    struct file_header file_hdr;
    int i, file_i = -1;

    /* Load disk header into memory */
    load_disk_hdr(vdisk_fp, &disk_hdr);

    /* Find file with a given name */
    for (i = 0; i < disk_hdr.file_count; i++)
    {
        /* Load i-th file header into memory */
        fseek(vdisk_fp, disk_hdr.file_offsets[i], SEEK_SET);
        fread(&file_hdr, sizeof(struct file_header), 1, vdisk_fp);

        /* If file found, save its index and exit loop */
        if (strcmp(file_hdr.file_name, file_name) == 0)
        {
            file_i = i;
            break;
        }
    }

    if (file_i < 0) return -1; /* File with such name doesn't exist */
    return i;
}

int get_file_list(vdisk_t *vdisk_fp, struct file_list *list_ptr)
{
    struct disk_header disk_hdr;
    int i;

    /* Load disk header into memory */
    load_disk_hdr(vdisk_fp, &disk_hdr);

    /* Save data into file_list structure */
    list_ptr->file_count = disk_hdr.file_count;
    for (i = 0; i < disk_hdr.file_count; i++)
    {
        fseek(vdisk_fp, disk_hdr.file_offsets[i], SEEK_SET);
        fread(list_ptr->files+i, sizeof(struct file_header), 1, vdisk_fp);
    }

    return 0;
}

int max_reg_cnt(vdisk_t *vdisk_fp)
{
    struct disk_header disk_hdr;

    /* Load disk header into memory */
    load_disk_hdr(vdisk_fp, &disk_hdr);

    return 3 * disk_hdr.file_count + 2;
}

int get_mem_info(vdisk_t *vdisk_fp, struct region_info *regions_ptr)
{
    struct disk_header disk_hdr;
    struct file_header file_hdr;
    int i, rg_cnt = 0;
    size_t DISK_HDR_SIZE = sizeof(struct disk_header);
    size_t FILE_HDR_SIZE = sizeof(struct file_header);
    off_t next_off, vdisk_size = get_stream_size(vdisk_fp);

    /* Load disk header into memory */
    load_disk_hdr(vdisk_fp, &disk_hdr);

    /* First region is always disk header, save its info */
    regions_ptr[rg_cnt].offset = 0;
    regions_ptr[rg_cnt].size = DISK_HDR_SIZE;
    regions_ptr[rg_cnt].purpose = REG_DISKHDR;

    rg_cnt++;

    /* If there is free space before first file, save its info */
    next_off = DISK_HDR_SIZE;
    if ((disk_hdr.file_count == 0 && next_off != vdisk_size) ||
        (disk_hdr.file_count > 0 && next_off != disk_hdr.file_offsets[0]))
    {
        regions_ptr[rg_cnt].offset = next_off;
        regions_ptr[rg_cnt].size =
                (disk_hdr.file_count == 0) ?
                    vdisk_size - next_off :
                    disk_hdr.file_offsets[0] - next_off;
        regions_ptr[rg_cnt].purpose = REG_FREE;

        rg_cnt++;
    }

    /* Go through all files, save every file as 2 regions
     * and check if it's followed by free space */
    for (i = 0; i < disk_hdr.file_count; i++)
    {
        off_t offset = disk_hdr.file_offsets[i];

        /* Load file header */
        fseek(vdisk_fp, offset, SEEK_SET);
        fread(&file_hdr, FILE_HDR_SIZE, 1, vdisk_fp);

        /* Save file header info */
        regions_ptr[rg_cnt].offset = offset;
        regions_ptr[rg_cnt].size = FILE_HDR_SIZE;
        regions_ptr[rg_cnt].purpose = REG_FILEHDR;

        rg_cnt++;

        /* Save file data info */
        regions_ptr[rg_cnt].offset = offset + FILE_HDR_SIZE;
        regions_ptr[rg_cnt].size = file_hdr.file_size - FILE_HDR_SIZE;
        regions_ptr[rg_cnt].purpose = REG_FILEDATA;

        rg_cnt++;

        /* Save trailing free region (if any) */
        next_off = (i != disk_hdr.file_count-1 ?
                    disk_hdr.file_offsets[i+1] :
                    vdisk_size);
        if (offset + file_hdr.file_size != next_off)
        {
            regions_ptr[rg_cnt].offset = offset + file_hdr.file_size;
            regions_ptr[rg_cnt].size = next_off - regions_ptr[rg_cnt].offset;
            regions_ptr[rg_cnt].purpose = REG_FREE;

            rg_cnt++;
        }
    }

    return rg_cnt;
}

off_t get_disk_size(vdisk_t *vdisk_fp)
{
    return get_stream_size(vdisk_fp);
}

int delete_file(vdisk_t *vdisk_fp, int file_index)
{
    struct disk_header disk_hdr;
    int i;

    /* Load disk header into memory */
    load_disk_hdr(vdisk_fp, &disk_hdr);

    if (file_index < 0 || file_index >= disk_hdr.file_count)
        return 1; /* Index out of bounds */

    /* Modify and save disk header */
    if (file_index != --disk_hdr.file_count)
    {
        /* This was not the last file,
         * entries in the array must be moved */
        for (i = file_index + 1; i <= disk_hdr.file_count; i++)
            disk_hdr.file_offsets[i-1] = disk_hdr.file_offsets[i];
    }
    fseek(vdisk_fp, 0, SEEK_SET);
    fwrite(&disk_hdr, sizeof(struct disk_header), 1, vdisk_fp);

    return 0;
}

int delete_disk(const char *file_path)
{
    return remove(file_path);
}

int defragment(vdisk_t *vdisk_fp, int demo)
{
    struct disk_header disk_hdr;
    int i;
    off_t best_off = sizeof(struct disk_header);

    load_disk_hdr(vdisk_fp, &disk_hdr);

    for (i = 0; i < disk_hdr.file_count; i++)
    {
        struct file_header file_hdr;
        off_t curr_off = disk_hdr.file_offsets[i];
        if (curr_off != best_off)
        {
            /* Free space between files, move the file */
            char buf;
            off_t moved = 0;
            load_bar *lb;

            /* Load file header */
            fseek(vdisk_fp, curr_off, SEEK_SET);
            fread(&file_hdr, sizeof(struct file_header), 1, vdisk_fp);

            if (demo == DEMO)
            {
                printf("\nMoving file \"%s\"...\n", file_hdr.file_name);
                lb = load_bar_init(file_hdr.file_size);
            }

            /* Go back to beginning of the file and move it byte by byte*/
            fseek(vdisk_fp, curr_off, SEEK_SET);
            while (moved < file_hdr.file_size && fread(&buf, sizeof(char), 1, vdisk_fp))
            {
                /* Go to new location for the byte */
                fseek(vdisk_fp, best_off+moved, SEEK_SET);

                /* Save byte */
                fwrite(&buf, sizeof(char), 1, vdisk_fp);

                moved++;

                if (demo == DEMO) load_bar_increment(lb);

                /* Go to old location to get next byte */
                fseek(vdisk_fp, curr_off+moved, SEEK_SET);
            }

            if (demo == DEMO) load_bar_destroy(lb);

            if (moved != file_hdr.file_size) return 1; /* Error occurred */

            if (demo == DEMO) printf("File \"%s\" moved successfully.\n", file_hdr.file_name);

            /* Save new offset in the disk header */
            disk_hdr.file_offsets[i] = best_off;
        }

        /* Set best_off for next file */
        best_off += file_hdr.file_size;
    }

    /* Save updated file header */
    fseek(vdisk_fp, 0, SEEK_SET);
    fwrite(&disk_hdr, sizeof(struct disk_header), 1, vdisk_fp);

    return 0;
}
