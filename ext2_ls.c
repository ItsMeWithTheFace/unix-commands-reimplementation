#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ext2.h"
#include "ext2_util.h"


void read_dir_contents(int fd, const struct ext2_inode *inode, int dot_flag) {
    void *block;
    char curr_file[EXT2_NAME_LEN];

    if ((block = malloc(EXT2_BLOCK_SIZE)) == NULL) { /* allocate memory for the data block */
        perror("Memory error\n");
        exit(1);
    }

    lseek(fd, EXT2_BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
    read(fd, block, EXT2_BLOCK_SIZE);

    struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *) (block);

    if (S_ISREG(inode->i_mode) || S_ISLNK(inode->i_mode)){
        memcpy(curr_file, dir_entry->name, dir_entry->name_len);
        curr_file[dir_entry->name_len] = 0;
        printf("%s\n", curr_file);
        dir_entry = (void *) dir_entry + dir_entry->rec_len;
    } else if (S_ISDIR(inode->i_mode)) {
        while(dir_entry->inode) {
            memcpy(curr_file, dir_entry->name, dir_entry->name_len);
            curr_file[dir_entry->name_len] = 0;
            if (strncmp(".", curr_file, 1) == 0) {
                if (dot_flag == 0)
                    printf("%s\n", curr_file);
            } else {
                printf("%s\n", curr_file);
            }

            dir_entry = (void *) dir_entry + dir_entry->rec_len;
        }
    }
}

int main(int argc, char **argv) {
    int c;
    int dot_flag = 0;
    if (argc < 2) {
        fprintf(stderr, "Usage: ext2_ls [-a] IMAGE_FILE ABSOLUTE_PATH\n");
        return 1;
    }

    while ((c = getopt(argc, argv, "a")) != -1) {
        switch(c) {
            case 'a':
                dot_flag = 1;
        }
    }

    int fd = get_file_descriptor(argv[optind]);
    struct ext2_inode *inode = traverse_path(fd, argv[optind + 1]);
    if (inode) {
        read_dir_contents(fd, inode, dot_flag);
    }  
    else
        return 1;

    return 0;
}
