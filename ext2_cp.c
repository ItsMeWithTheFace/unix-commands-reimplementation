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
 * Find the directory inode in image
 * Just open up the image using open or some shit
 * create new inode block in directory
 * set all of its values
 * read own file line by line into the inode blocks
**/


char * get_file_contents(char *file_name) {
    FILE *file = fopen(file_name, "r");
    char *contents = 0;
    long length;

    if (file) {
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        contents = malloc(length);

        if (contents)
            fread(contents, 1, length, file);

        fclose(file);

        if (contents)
            return contents;
        else
            fprintf(stderr, "Error with reading file\n");
    }

    exit(ENOENT);
}

int transfer_contents(char *file_name, struct ext2_inode *file_inode) {
    int i = 0;
    int curr_block_num;
    char *contents = get_file_contents(file_name);
    char *block;

    while(contents[i] != '\0') {
        curr_block_num = allocate_block();
        block = (char *) (disk + EXT2_BLOCK_SIZE * curr_block_num);
        add_inode_block(file_inode, curr_block_num);

        for (i = 0; i < EXT2_BLOCK_SIZE; i++) {
            if (contents[i] != '\0')
                block[i] = contents[i];
            else
                return 1;
        }
    }

    block[i] = '\0';
    return 0;



    // don't read line by line. just get 1 big ass string
    // copy n characters in that string to memory
    // memory = (disk + ext2blocksize * block)
    // all the way until memory = memory + 1024
    // how tf do we know how much to copy? how much is n characters?

}





int main(int argc, char **argv){
    struct NamedInode *dir_p = NULL;
    struct NamedInode dir;

    //Check if the file exists
    initialize_disk(argv[1]);
    dir_p = traverse_path(argv[3]);
    dir = *dir_p;
    int inode_num = insert_inode(TYPE_FILE);
    struct ext2_inode *inode = get_inode(inode_num, gd);
    printf("hey\n");
    create_new_dir_entry(dir.inode, inode_num, "hi", TYPE_FILE);
    printf("bye\n");
    transfer_contents(argv[2], inode);

    return 0;
}