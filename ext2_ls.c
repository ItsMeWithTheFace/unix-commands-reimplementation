#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ext2.h"
#include "helper.h"

/**
 * Prints the contents of a directory or just the name of a file/symlink.
 * Can specify whether to not print folders beginning with dots (.) using
 * the dot_flag parameter.
**/
void read_dir_contents(struct NamedInode *ni, int dot_flag) {
    struct ext2_dir_entry_2 *dir_entry = NULL;
    struct NamedInode ni_d = *ni;
    int block_num;
    char curr_file[EXT2_NAME_LEN];
    int size = 0;

    for (int i = 0; i < 12; i++) {
        // if a block is occupied, read it
        if (ni_d.inode->i_block[i] != 0) {
            block_num = ni_d.inode->i_block[i];

            if (S_ISREG(ni_d.inode->i_mode) || S_ISLNK(ni_d.inode->i_mode)) {
                printf("%s\n", ni->name);
            } else if (S_ISDIR(ni_d.inode->i_mode)) {
                dir_entry = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * block_num);

                while((size < ni_d.inode->i_size)) {
                    if (dir_entry->inode) {
                        memcpy(curr_file, dir_entry->name, dir_entry->name_len);
                        curr_file[dir_entry->name_len] = 0;

                        if (strncmp(".", curr_file, 1) == 0) {  // ignore dot directories unless dot_flag
                            if (dot_flag == 1)
                                printf("%s\n", curr_file);
                        } else {
                            printf("%s\n", curr_file);
                        }
                    }

                    size += dir_entry->rec_len;
                    dir_entry = (void *) dir_entry + dir_entry->rec_len;
                }
            }
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

    initialize_disk(argv[optind]);
    struct NamedInode *inode = traverse_path(argv[optind + 1]);
    read_dir_contents(inode, dot_flag);

    return 0;
}
