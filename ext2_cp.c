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
 * Checks if a file exists
**/
int check_file_exists(const char *file_name) {
    struct stat st;
    stat(file_name, &st);
    return S_ISREG(st.st_mode);
}

/**
 * Retrieves the contents of a file with file_name
**/
char * get_file_contents(char *file_name) {
    FILE *file = NULL;
    if (check_file_exists(file_name))
        file = fopen(file_name, "r");
    else
        fprintf(stderr, "Invalid path to file\n");
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

/**
 * Gets the file name from an absolute path
**/
char * get_input_file_name(char *abs_path) {
    char *file_name;
    char *token = strtok(abs_path, "/");

    while (token) {
        file_name = token;

        token = strtok(NULL, "/");
    }

    return file_name;
}

int main(int argc, char **argv) {
    struct NamedInode *dir_p = NULL;
    struct NamedInode dir;

    if (argc < 4) {
        fprintf(stderr, "Usage: ext2_cp IMAGE_FILE PATH_TO_FILE DIR_IN_IMAGE\n");
        return 1;
    }

    initialize_disk(argv[1]);

    // Check if the file exists
    char *file_name = get_input_file_name(argv[2]);
    struct PathTuple location = parse_directory_path(argv[3]);
    dir_p = traverse_path(location.path);
    dir = *dir_p;

    char *contents = get_file_contents(argv[2]);

    if (check_exists(dir.inode, location.dir_name)) {
        struct NamedInode exist_inode = *(find_in_dir(dir.inode, location.dir_name));
        if (exist_inode.inode) {
            if (!S_ISDIR(exist_inode.inode->i_mode)) {
                // it's not a dir
                fprintf(stderr, "File already exists\n");
                return EEXIST;
            }

            if (check_exists(exist_inode.inode, file_name)) {
                fprintf(stderr, "File already exists\n");
                return EEXIST;
            }

            int inode_num = insert_inode(TYPE_FILE);
            struct ext2_inode *inode = get_inode(inode_num, gd);
            create_new_dir_entry(exist_inode.inode, inode_num, file_name, TYPE_FILE);
            transfer_contents(contents, inode);
        }
    } else {
        // otherwise make a file with that name and copy into it
        int inode_num = insert_inode(TYPE_FILE);
        struct ext2_inode *inode = get_inode(inode_num, gd);
        create_new_dir_entry(dir.inode, inode_num, location.dir_name, TYPE_FILE);
        transfer_contents(contents, inode);
    }

    return 0;
}
