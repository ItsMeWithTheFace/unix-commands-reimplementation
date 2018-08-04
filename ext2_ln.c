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


/**
 * Main method for creating a link between 2 files
 * args = (disk_img name, absolute path to file1, absolute path to file2)
 * no flags: make a hard link
 * -s: make a soft/symbolic link
 * ENONENT: Source file doesn't exist
 * EXISTS: link name already exists
 * EISDIR: either argument points to a dir instead of a file
 */
int main (int argc, char **argv) {
    int c;
    int sym_flag = 0;
    struct NamedInode *src_inode = NULL;
    struct NamedInode *dest_inode = NULL;
    struct NamedInode src, dest;

    if (argc < 4) {
        fprintf(stderr, "Usage: ext2_ls [-s] IMAGE_FILE PATH_TO_LINK LINK_LOCATION");
        return ENOENT;
    }

    while ((c = getopt(argc, argv, "s")) != -1) {
        switch(c) {
            case 's':
                sym_flag = 1;
        }
    }

    // Disk setup and finding paths
    initialize_disk(argv[optind]);
    src_inode = traverse_path(argv[optind + 1]);
    src = *src_inode;
    char src_name[EXT2_NAME_LEN];
    strcpy(src_name, src.name);

    dest_inode = traverse_path(argv[optind + 2]);
    dest = *dest_inode;
    char dest_name[EXT2_NAME_LEN];
    strcpy(dest_name, dest.name);

    // Check if either path params are directories or not files
    if (!S_ISREG(src.inode->i_mode)) {
        fprintf(stderr, "You need to specify a file to link\n");
        return EISDIR;
    }

    struct PathTuple dest_location = parse_directory_path(argv[optind + 2]);

    if (sym_flag) {
        struct NamedInode *dir_inode = traverse_path(dest_location.path);
        int new_inode_num = insert_inode(TYPE_LINK);
        struct ext2_inode *new_inode = get_inode(new_inode_num, gd);
        int new_block_num = allocate_block() + 1;
        add_inode_block(new_inode, new_block_num);

        create_new_dir_entry((*dir_inode).inode, new_inode_num, dest_location.dir_name, TYPE_LINK);

        // add path method

    } else {
        // setup a hard link
        create_new_dir_entry(dest.inode, src.inode_num, src_name, TYPE_FILE);
    }

    return 0;
}