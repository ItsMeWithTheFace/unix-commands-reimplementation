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
int main (int argc, char **argv){
    //Disk setup
    initialize_disk(argv[1]);
    struct NamedInode *inode_from_path1 = traverse_path(argv[2]);
    struct NamedInode *inode_from_path2 = traverse_path(argv[3]);
    //Allocate space for the inode
    int inode_num = allocate_inode();
    //Check if either path params are directories or not files
    if (S_ISDIR((inode_from_path1->inode)->i_mode) || S_ISDIR((inode_from_path2->inode)->i_mode)){
        return EISDIR;
    } else if (inode_num > 0){
        struct ext2_inode *link_inode = get_inode(inode_num, gd);
        
    }
}