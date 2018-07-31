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


/**
 * Splits an absolute path and returns a PathTuple. This contains
 * the absolute path of the directory we want to insert into
 * (PathTuple.path) and the name of the new directory (PathTuple.dir_name)
**/
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
        // check if it already exists
        if (check_exists(dir.inode, location.dir_name) > 0) {
            fprintf(stderr, "File exists\n");
            return EEXIST;
        }

        // successfully got to the destination, make the dir inode
        int inode_num = insert_inode(TYPE_DIR);
        if (inode_num > 0) {
            // put the new inode in the parent's data block
            create_new_dir_entry(dir.inode, inode_num, location.dir_name, TYPE_DIR);
            // make the . and .. entries in the data blocks for new dir
            struct ext2_inode *new_inode = get_inode(inode_num, gd);
            create_new_dir_entry(new_inode, inode_num, ".", TYPE_DIR);
            // the parent directory is stored in the "path" attribute of PathTuple
            create_new_dir_entry(new_inode, dir.inode_num, "..", TYPE_DIR);
        } else {
            return ENOSPC;
        }
    } else {
        return ENOENT;
    }

    return 0;
}

