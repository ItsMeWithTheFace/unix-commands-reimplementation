#include <stdio.h>
#include <time.h>
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
unsigned int block_size;

/**
 * Initializes the super block, group descriptor and block size
 * 
**/
void init_fs_essentials() {
    sb = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    gd = (struct ext2_group_desc *) (disk + 2*EXT2_BLOCK_SIZE);
    block_size = EXT2_BLOCK_SIZE << sb->s_log_block_size;
}

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

    // initialize the superblock, group descriptor and other stuff
    init_fs_essentials();

    return fd;
}

unsigned int get_current_time() {
    time_t t;
    struct tm *timeinfo;

    time(&t);
    timeinfo = localtime(&t);
    t = mktime(timeinfo);

    return t;
}

int allocate_inode() {
    unsigned char *block = disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap;

    for (int i = 0; i < (sb->s_inodes_count / (sizeof(unsigned char) * 8)); i++) {
        for (int j = 0; j < 8; j++) {
            if ((block[i] & (1 << j)) == 0) {
                // update the super block and group descriptor values 
                sb->s_free_inodes_count--;
                gd->bg_free_inodes_count--;
                // set that bit to 1
                block[i] |= 1 << j;
                // return the index
                return i * 8 + j + 1;
            }
        }
        printf(" ");
    }

    return 0;
}

int allocate_block() {
    unsigned char *block = disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap;

    lseek(fd, EXT2_BLOCK_OFFSET(gd->bg_block_bitmap), SEEK_SET);
    read(fd, block, EXT2_BLOCK_SIZE);

    for (int i = 0; i < (sb->s_blocks_count / (sizeof(unsigned char) * 8)); i++) {
        for (int j = 0; j < 8; j++) {
            if ((block[i] & (1 << j)) == 0) {
                sb->s_free_blocks_count--;
                gd->bg_free_blocks_count--;
                // set that bit to 1
                block[i] |= 1 << j;
                // return the index
                return i * 8 + j + 1;
            }
        }
    }

    return 0;
}

/**
 * Retrieves the full inode of an inode with the number inode_num.
 * 
**/
struct ext2_inode * get_inode(int fd, int inode_num, const struct ext2_group_desc *group_desc) {
    struct ext2_inode * inode_tbl = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
    struct ext2_inode * in = &inode_tbl[inode_num - 1];

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

int pad_rec_len(char *name) {
    int rec_len = strlen(name) + sizeof(struct ext2_dir_entry_2);
    if ((rec_len % 4) > 0) {
        printf("rec_len here: %i\n", rec_len);
        return rec_len + (4 - (rec_len % 4));
    } else {
        return rec_len;
    }
}


/**
 * Inserts a new inode into memory
**/
int insert_inode(int fd, int inode_type) {

    // find the next available block
    int inode_num = allocate_inode() + 1;
    if (inode_num > 0) {
        struct ext2_inode * inode_tbl = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
        struct ext2_inode * new_inode = &inode_tbl[inode_num - 1];
        memset(new_inode, 0, sizeof(struct ext2_inode));

        new_inode->i_mode |= S_IFDIR;
        new_inode->i_blocks = 0;
        new_inode->i_links_count = 0;
        new_inode->i_size = sizeof(struct ext2_inode);

        int time = get_current_time();
        new_inode->i_atime = time;
        new_inode->i_ctime = time;
        new_inode->i_mtime = time;
        printf("inode_num: %i\n", inode_num);
        printf("inode mode: %i\n", new_inode->i_mode);
        printf("inode size: %i\n", new_inode->i_size);
        printf("inode dtime: %i\n", new_inode->i_dtime);

        printf("is dir? %i\n", S_ISDIR(new_inode->i_mode));


        return inode_num;

    }

    // // find a free inode spot in the inode table
    // // allocate new inode struct
    // // find the first vacant spot in the block node
    // // add in the inode info there
    // // update inode and block bitmaps


    return 0;
}


/**
 * Creates directory inode in directory blocks of dir_inode
**/
int create_new_dir(int fd, struct ext2_inode *dir_inode, int inode_num, char *name) {
    int size;
    unsigned int block_num;
    void *block;

    if ((block = malloc(block_size)) == NULL) {
        perror("Memory error\n");
        exit(1);
    }

    // go through the i_block array to find the allocated spaces
    for (int i = 0; i < 12; i++) {
        if (dir_inode->i_block[i] != 0) {
            printf("finding space in current block: %i\n", dir_inode->i_block[i]);
            block_num = dir_inode->i_block[i];

            struct ext2_dir_entry_2 *de2 = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * block_num);
            size = 0;
            int rec_len = pad_rec_len(name);
            // see if there's any space in each of them
            while(size < dir_inode->i_size) {
                if (de2->inode == 0) {
                        printf("found space\n");
                        de2->inode = inode_num;
                        strcpy(de2->name, name);
                        de2->name_len = strlen(name);
                        de2->file_type = TYPE_DIR;
                        de2->rec_len = rec_len;

                        //dir_inode->size

                        return 1;
                }
                printf("didn't find space, keep looking\n");
                size += de2->rec_len;
                de2 = (void *) de2 + de2->rec_len;
            }
            printf("didn't find shit, time to start making new blocks\n");
        }
    }

    // if not, allocate space
    block_num = allocate_block();
    if (block_num > 0) {
        printf("block_num: %i\n", block_num);
        // create the new dir_entry
        for (int i = 0; i < 12; i++) {
            if (dir_inode->i_block[i] == 0) {
                printf("made a new block\n");
                // set the dir_inode's pointer to this block
                dir_inode->i_block[i] = block_num;
                dir_inode->i_size += EXT2_BLOCK_SIZE;
                dir_inode->i_blocks = dir_inode->i_blocks / (2 << sb->s_log_block_size);

                lseek(fd, EXT2_BLOCK_OFFSET(block_num), SEEK_SET);
                read(fd, block, block_size);

                struct ext2_dir_entry_2 *de2 = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * block_num);

                de2->inode = inode_num;
                strcpy(de2->name, name);
                de2->name_len = strlen(name);
                de2->file_type = TYPE_DIR;
                de2->rec_len = pad_rec_len(name);

                printf("dir_inode blocks: %i\n", dir_inode->i_size);
                printf("dir_inode size: %i\n", dir_inode->i_size);
                printf("size: %i\n", size);
                printf("rec_len: %i\n", pad_rec_len(name));
                printf("de2 rec_len: %i\n", de2->rec_len);
                printf("de2 inode: %i\n", de2->inode);
                return 1;
            }
        }
    }

    return -ENOSPC;
}

// int main(int argc, char **argv) {
//     int fd = get_file_descriptor(argv[1]);
//     struct ext2_inode *root = get_inode(fd, EXT2_ROOT_INO, gd);
//     int node_num = insert_inode(fd, TYPE_DIR);
//     create_new_dir(fd, root, node_num, "hi");

//     return 0;
// }

