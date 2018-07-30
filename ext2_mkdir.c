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


struct PathTuple * parse_directory_path(char *path) {
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

    struct PathTuple pt = {new_parsed_path, new_dir_name};
    struct PathTuple * pt_p = &pt;

    free(parsed_path);
    free(new_parsed_path);

    return pt_p;
}


int main(int argc, char **argv) {
    char h[] = "//d/a//asd/12/d/asd/";

    struct PathTuple *s = parse_directory_path(h);
    printf("%s %s\n", s->dir_name, s->path);

    return 0;
}

