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

int fd;
unsigned char *disk;
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
void initialize_disk(const char *image) {
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

    printf("gd: %i\n", gd->bg_free_blocks_count);
}

/**
 * Maps file types to their bitwise equivalent (or something) so that they
 * can be used to set inode modes
**/
unsigned short get_inode_type(int file_type) {
    switch(file_type) {
        case TYPE_FILE:
            return S_IFREG;
        case TYPE_LINK:
            return S_IFLNK;
        case TYPE_DIR:
            return S_IFDIR;
        default:
            return 0000000;
    }
}

/**
 * Gets the current UNIX timestamp
**/
unsigned int get_current_time() {
    time_t t;
    struct tm *timeinfo;

    time(&t);
    timeinfo = localtime(&t);
    t = mktime(timeinfo);

    return t;
}

/**
 * Finds an unallocated inode in the inode bitmap, returns its
 * position and sets it to in-use
**/
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

                return i * 8 + j;
            }
        }
    }

    return 0;
}

/**
 * Finds an unallocated block in the block bitmap, returns its
 * position and sets it to in-use
**/
int allocate_block() {
    unsigned char *block = disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap;

    for (int i = 0; i < (sb->s_blocks_count / (sizeof(unsigned char) * 8)); i++) {
        for (int j = 0; j < 8; j++) {
            if ((block[i] & (1 << j)) == 0) {
                // update the super block and group descriptor values
                sb->s_free_blocks_count--;
                gd->bg_free_blocks_count--;
                // set that bit to 1
                block[i] |= 1 << j;

                return i * 8 + j;
            }
        }
    }

    return 0;
}

/**
 * Retrieves the full inode of an inode with the number inode_num.
 * 
**/
struct ext2_inode * get_inode(int inode_num, const struct ext2_group_desc *group_desc) {
    struct ext2_inode * inode_tbl = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
    struct ext2_inode * in = &inode_tbl[inode_num - 1];

    return in;
}

/**
 * Given a directory inode (an inode where S_ISDIR(inode->i_mode) == 1), finds
 * the inode with name == file_name and returns it. Returns NULL if not found.
 * 
**/
struct ext2_inode * find_in_dir(struct ext2_inode *dir_inode, char *file_name) {
    char curr_file[EXT2_NAME_LEN];
    unsigned int size = 0;

    for (int i = 0; i < 12; i++) {
        if (dir_inode->i_block[i] > 0) {
            // found a used block, go to it
            int block_num = dir_inode->i_block[i];
            struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * block_num);
            // go through contents and see if name matches what we're looking for
            while((size < dir_inode->i_size) && dir_entry->inode) {
                memcpy(curr_file, dir_entry->name, dir_entry->name_len);
                curr_file[dir_entry->name_len] = 0;
                if (strcasecmp(file_name, curr_file) == 0) {
                    return get_inode(dir_entry->inode, gd);
                }

                dir_entry = (void *) dir_entry + dir_entry->rec_len;
                size += dir_entry->rec_len;
            }
        }
    }

    return NULL;
}

/**
 * Traverses through an absolute path in an image. Returns the last
 * inode that was met in the traversal along with the name. Returns NULL
 * for invalid paths.
**/
struct NamedInode * traverse_path(char *path) {
    struct ext2_inode *curr_inode = get_inode(EXT2_ROOT_INO, gd);

    char *prev_file_name;
    char *file_name = strtok(path, "/");
    while (file_name) {
        // check if curr_inode is a directory, else return error
        if (!S_ISDIR(curr_inode->i_mode)) {
            fprintf(stderr, "No such file or directory\n");
            return NULL;
        }
        // search for inode with file_name and that becomes curr_inode
        curr_inode = find_in_dir(curr_inode, file_name);
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

/**
 * Pads the rec_len to become multiple of 4 to align on 4 byte boundaries
**/
int pad_rec_len(char *name) {
    int rec_len = strlen(name) + sizeof(struct ext2_dir_entry_2);
    if ((rec_len % 4) > 0) {
        return rec_len + (4 - (rec_len % 4));
    } else {
        return rec_len;
    }
}

/**
 * Inserts a new inode into memory
**/
int insert_inode(int inode_type) {
    // find the next available block
    int inode_num = allocate_inode() + 1;
    if (inode_num > 0) {
        struct ext2_inode * inode_tbl = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
        struct ext2_inode * new_inode = &inode_tbl[inode_num - 1];
        memset(new_inode, 0, sizeof(struct ext2_inode));

        new_inode->i_mode |= get_inode_type(inode_type);
        new_inode->i_blocks = 0;
        new_inode->i_links_count = 0;
        new_inode->i_size = sizeof(struct ext2_inode);

        int time = get_current_time();
        new_inode->i_atime = time;
        new_inode->i_ctime = time;
        new_inode->i_mtime = time;

        return inode_num;
    }

    return 0;
}

int get_parent_inode(struct ext2_inode * inode) {
    return 0;
}

/**
 * Returns a dir_entry with the values set to the parameter's values
**/
struct ext2_dir_entry_2 * initialize_dir_entry(struct ext2_dir_entry_2 * de2, int inode_num, char *name, int file_type, int rec_len) {
    de2->inode = inode_num;
    de2->name_len = strlen(name);
    de2->file_type = file_type;
    strcpy(de2->name, name);
    de2->rec_len = rec_len;

    return de2;
}

/**
 * Creates directory inode in directory blocks of dir_inode
**/
struct ext2_dir_entry_2 * create_new_dir_entry(struct ext2_inode *dir_inode, int inode_num, char *name, int file_type) {
    int size;
    unsigned int block_num;

    printf("heythere\n");
    // go through the i_block array to find the allocated spaces
    for (int i = 0; i < 12; i++) {
        if (dir_inode->i_block[i] != 0) {
            block_num = dir_inode->i_block[i];

            struct ext2_dir_entry_2 *de2 = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * block_num);
            size = 0;
            int rec_len = pad_rec_len(name);
            // see if there's any space in each of them
            while(size < dir_inode->i_size) {
                if (de2->inode == 0) {

                    struct ext2_dir_entry_2 *new_de2 = initialize_dir_entry(de2, inode_num, name, file_type, rec_len);

                    dir_inode->i_links_count++;

                    return new_de2;
                } else {
                    // see if we can shorten the rec_len of current dir_entry
                    // and insert it there
                    int real_rec_len = pad_rec_len(de2->name);
                    if (rec_len <= (de2->rec_len - real_rec_len)) {
                        de2->rec_len = real_rec_len;

                        size += de2->rec_len;
                        de2 = (void *) de2 + de2->rec_len;
                        
                        int new_entry_rec_len = EXT2_BLOCK_SIZE - size;
                        struct ext2_dir_entry_2 *new_de2 = initialize_dir_entry(de2, inode_num, name, file_type, new_entry_rec_len);

                        dir_inode->i_links_count++;

                        return new_de2;
                    }
                }
                size += de2->rec_len;
                de2 = (void *) de2 + de2->rec_len;
            }
        }
    }

    // coming soon, go through the indirect blocks

    // if not, allocate space
    block_num = allocate_block() + 1;
    if (block_num > 0) {
        // create the new dir_entry
        for (int i = 0; i < 12; i++) {
            if (dir_inode->i_block[i] == 0) {
                // set the dir_inode's pointer to this block
                dir_inode->i_block[i] = block_num;
                dir_inode->i_size += EXT2_BLOCK_SIZE;
                dir_inode->i_blocks = dir_inode->i_blocks / (2 << sb->s_log_block_size);
                dir_inode->i_links_count++;

                struct ext2_dir_entry_2 *de2 = (struct ext2_dir_entry_2 *) (disk + EXT2_BLOCK_SIZE * block_num);
                struct ext2_dir_entry_2 *new_de2 = initialize_dir_entry(de2, inode_num, name, file_type, EXT2_BLOCK_SIZE);
                de2->inode = inode_num;
                strcpy(de2->name, name);
                de2->name_len = strlen(name);
                de2->file_type = TYPE_DIR;
                de2->rec_len = EXT2_BLOCK_SIZE;

                return new_de2;
            }
        }

        // coming soon, dealing with indirect blocks
    }

    exit(ENOSPC);
}

// int main(int argc, char **argv) {
//     initialize_disk(argv[1]);
//     struct ext2_inode *root = get_inode(EXT2_ROOT_INO, gd);
//     int node_num = insert_inode(TYPE_DIR);
//     create_new_dir_entry(root, node_num, "hi", TYPE_DIR);

//     return 0;
// }

