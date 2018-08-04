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
    
    return 0;
}
