#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ext2.h"
#include "ext2_mkdir.h"
#include "ext2_util.h"


struct PathTuple parse_directory_path(char *path) {
    char *new_dir_name = malloc(strlen(path) + 1);
    char *parsed_path = malloc(strlen(path) + 1);

    char *token = strtok(path, "/");

    while (token) {
        strcat(parsed_path, "/");
        strcat(parsed_path, token);
        strcpy(new_dir_name, token);
        token = strtok(NULL, "/");
    }

    char *new_parsed_path = malloc(strlen(parsed_path) - strlen(new_dir_name));
    strncpy(new_parsed_path, parsed_path, strlen(parsed_path) - strlen(new_dir_name) - 1);

    if (strcmp(new_parsed_path, "") == 0) {
        strcpy(new_parsed_path, "/");
    }

    struct PathTuple pt = {new_parsed_path, new_dir_name};

    return pt;
}


int main(int argc, char **argv) {
    struct NamedInode *dir_p = NULL;
    struct NamedInode dir;  // need to turn it into struct to maintain persistent data

    initialize_disk(argv[1]);
    struct PathTuple location = parse_directory_path(argv[2]);
    dir_p = traverse_path(location.path);
    dir = *dir_p;
    if (dir_p) {
        int inode_num = insert_inode(TYPE_DIR);
        if (inode_num > 0) {
            //struct ext2_inode *new_inode = get_inode(EXT2_ROOT_INO, gd);
            create_new_dir_entry(dir.inode, inode_num, location.dir_name, TYPE_DIR);
            struct ext2_inode *new_inode = get_inode(inode_num, gd);
            create_new_dir_entry(new_inode, inode_num, ".", TYPE_DIR);
            //create_new_dir_entry(new_inode, inode_num, ".", TYPE_DIR);
        }
    } else {
        return ENOSPC;
    }

    return 0;
}

