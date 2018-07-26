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
 * This program takes three command line arguments. The first is the name of 
 * an ext2 formatted virtual disk. The second is the path to a file on your native 
 * operating system, and the third is an absolute path on your ext2 formatted disk. 
 * The program should work like cp, copying the file on your native file system onto 
 * the specified location on the disk. If the specified file or target location does 
 * not exist, then your program should return the appropriate error (ENOENT).
**/

int main(int argc, char **argv){
    //Check if the file exists
    if (access(argv[2], F_OK) == -1){
        return ENOENT;
    } else {
        int fd = get_file_descriptor(argv[1]);
        struct NamedInode *inode = traverse_path(fd, argv[3]);
        struct ext2_inode *newInode;
        if (inode){
            //inode->inode-> = 
        } else {
            return ENOENT;
        }
    }

    return 0;
}