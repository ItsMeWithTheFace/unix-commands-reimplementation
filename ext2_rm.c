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
 * Main method for removing a file
 * args: (disk_image, absolute path to file or link)
 * ENONENT: Source file does not exist
 * EISDIR: Path to file or link is actually a directory
 */
int main(int argc, char **argv){
    struct NamedInode *src_inode = NULL;
    struct NamedInode src;

    if (argc < 3) {
        fprintf(stderr, "Usage: ext2_rm IMAGE_FILE PATH_TO_FILE\n");
        return 1;
    }

    initialize_disk(argv[1]);

    src_inode = traverse_path(argv[2]);
    src = *src_inode;

    struct PathTuple location = parse_directory_path(argv[2]);

    // target inode can't be a directory
    if (S_ISDIR(src.inode->i_mode)) {
        return EISDIR;
    }

    // Decrement ref count
    src.inode->i_links_count--;

    // If ref count is now 0
    if (src.inode->i_links_count == 0) {
        // Set dtime to current time
        src.inode->i_dtime = get_current_time();

        // update bitmaps
        free_inode(src.inode_num);

        for (int i = 0; i < 12; i++) {
            if (src.inode->i_block[i] != 0) {
                free_block(src.inode->i_block[i]);
            }
        }

        // set inode to zero (remove from the directory entry)
        struct NamedInode *dir_ni = traverse_path(location.path);
        
        remove_dir_entry((*dir_ni).inode, location.dir_name);

    }

    return 0;
}
