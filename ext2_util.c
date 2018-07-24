#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2_util.h"

unsigned char *disk;
int fd;
struct ext2_super_block *sb;
struct ext2_group_desc *gd;

int get_file_descriptor(const char *image) {
    fd = open(image, O_RDWR);

    if (fd < 0) {
        perror("open");
        exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    return fd;
}

struct ext2_inode * get_inode(int fd, int inode_num, const struct ext2_group_desc *group_desc) {
    struct ext2_inode *in = malloc(sizeof(struct ext2_inode));
    unsigned char *buffer = malloc(EXT2_BLOCK_SIZE);

    lseek(fd, EXT2_BLOCK_OFFSET(group_desc->bg_inode_table) + (inode_num-1)*sizeof(struct ext2_inode), SEEK_SET);
    read(fd, buffer, sizeof(struct ext2_inode));

    in = (struct ext2_inode *) buffer;

    return in;
}

struct ext2_inode * find_in_dir(int fd, struct ext2_inode *dir_inode, char *file_name) {
    void *block;
    char curr_file[EXT2_NAME_LEN];
    sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    unsigned int block_size = EXT2_BLOCK_SIZE << sb->s_log_block_size;

    if ((block = malloc(block_size)) == NULL) { /* allocate memory for the data block */
        perror("Memory error\n");
        exit(1);
    }

    lseek(fd, EXT2_BLOCK_OFFSET(dir_inode->i_block[0]), SEEK_SET);
    read(fd, block, block_size);

    struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *) (block);

    while(dir_entry->inode) {
        memcpy(curr_file, dir_entry->name, dir_entry->name_len);
        curr_file[dir_entry->name_len] = 0;
        if (strcasecmp(file_name, curr_file) == 0) {
            free(block);
            return get_inode(fd, dir_entry->inode, gd);
        }
        dir_entry = (void *) dir_entry + dir_entry->rec_len;
    }

    free(block);
    return NULL;
}

struct ext2_inode * traverse_path(int fd, char *path) {
    gd = (struct ext2_group_desc *) (disk + 2*EXT2_BLOCK_SIZE);
    struct ext2_inode *curr_inode = get_inode(fd, EXT2_ROOT_INO, gd);

    char *file_name = strtok(path, "/");
    while (file_name) {
        // check if curr_inode is a directory, else return error
        if (!S_ISDIR(curr_inode->i_mode)) {
            fprintf(stderr, "No such file or directory\n");
            return NULL;
        }
        // search for inode with file_name and that becomes curr_inode
        curr_inode = find_in_dir(fd, curr_inode, file_name);
        if (curr_inode == NULL) {
            fprintf(stderr, "No such file or directory\n");
            return NULL;
        }
        file_name = strtok(NULL, "/");
    }
    // If we're out of the loop, we entered a directory and finished traversing; print out contents
    // /home/usr/file

    return curr_inode;
}
