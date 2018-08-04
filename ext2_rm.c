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
#include <utils.h>

/**
 * Main method for removing a file
 * args: (disk_image, absolute path to file or link)
 * ENONENT: Source file does not exist
 * EISDIR: Path to file or link is actually a directory
 */
int main(int argc, char **argv){
    //Variable setup
    struct NamedInode *src_inode = NULL;
    struct NamedInode src;
    //Initialize the disk with given disk image
    initialize_disk(argv[optind]);
    //ENONENT is handled in traverse_path()
    src_inode = traverse_path(argv[optind + 1]);
    src = *src_inode;
    //ISDIR check
    if (S_ISDIR(src.inode->i_mode)){
        return EISDIR;
    }
    //Decrement ref count
    src.inode->i_links_count--;
    //Set dtime to current time
    src.inode->i_dtime = get_current_time;
    //If ref count is now 0
    if (src.inode->i_links_count == 0){
        //update bitmap
        set_inode_to_zero(src.inode_num, gd);
    }
    //Remove dir entries
    return 0;
}
