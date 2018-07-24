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

/**
 * Opens an image, maps the disk data into disk and returns the file descriptor
 * pointing to the open image.
 * 
**/
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

/**
 * Retrieves the full inode of an inode with the number inode_num.
 * 
**/
struct ext2_inode * get_inode(int fd, int inode_num, const struct ext2_group_desc *group_desc) {
    struct ext2_inode *in = malloc(sizeof(struct ext2_inode));
    unsigned char *buffer = malloc(EXT2_BLOCK_SIZE);

    lseek(fd, EXT2_BLOCK_OFFSET(group_desc->bg_inode_table) + (inode_num-1)*sizeof(struct ext2_inode), SEEK_SET);
    read(fd, buffer, sizeof(struct ext2_inode));

    in = (struct ext2_inode *) buffer;

    return in;
}

/**
 * Given a directory inode (an inode where S_ISDIR(inode->i_mode) == 1), finds
 * the inode with name == file_name and returns it. Returns NULL if not found.
 * 
**/
struct ext2_inode * find_in_dir(int fd, struct ext2_inode *dir_inode, char *file_name) {
    void *block;
    char curr_file[EXT2_NAME_LEN];
    unsigned int size = 0;

    sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    unsigned int block_size = EXT2_BLOCK_SIZE << sb->s_log_block_size;

    if ((block = malloc(block_size)) == NULL) { /* allocate memory for the data block */
        perror("Memory error\n");
        exit(1);
    }

    lseek(fd, EXT2_BLOCK_OFFSET(dir_inode->i_block[0]), SEEK_SET);
    read(fd, block, block_size);

    struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *) (block);

    while((size < dir_inode->i_size) && dir_entry->inode) {
        memcpy(curr_file, dir_entry->name, dir_entry->name_len);
        curr_file[dir_entry->name_len] = 0;
        if (strcasecmp(file_name, curr_file) == 0) {
            free(block);
            return get_inode(fd, dir_entry->inode, gd);
        }

        dir_entry = (void *) dir_entry + dir_entry->rec_len;
        size += dir_entry->rec_len;
    }

    free(block);
    return NULL;
}

/**
 * Traverses through an absolute path in an image. Returns the last
 * inode that was met in the traversal along with the name. Returns NULL
 * for invalid paths.
 * 
**/
struct NamedInode * traverse_path(int fd, char *path) {
    gd = (struct ext2_group_desc *) (disk + 2*EXT2_BLOCK_SIZE);
    struct ext2_inode *curr_inode = get_inode(fd, EXT2_ROOT_INO, gd);

    char *prev_file_name;
    char *file_name = strtok(path, "/");
    while (file_name) {
        // check if curr_inode is a directory, else return error
        if (!S_ISDIR(curr_inode->i_mode)) {
            fprintf(stderr, "No such file or directory\n");
            return NULL;
        }
        // search for inode with file_name and that becomes curr_inode
        curr_inode = find_in_dir(fd, curr_inode, file_name);
        if (!curr_inode) {
            fprintf(stderr, "No such file or directory\n");
            return NULL;
        }
        // save the old name for back-tracking purposes
        prev_file_name = file_name;
        file_name = strtok(NULL, "/");
    }
    // If we're out of the loop, we entered a directory/file and finished traversing;

    struct NamedInode ni = {prev_file_name, curr_inode};
    struct NamedInode * ni_p = &ni;
    return ni_p;
}
