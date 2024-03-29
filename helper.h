#include "ext2.h"

#define TYPE_FILE   1
#define TYPE_LINK   7
#define TYPE_DIR    2

#define EXT2_BLOCK_OFFSET(block) (EXT2_BLOCK_SIZE + (block - 1)*EXT2_BLOCK_SIZE)

extern int fd;
extern unsigned char *disk;
extern struct ext2_super_block *sb;
extern struct ext2_group_desc *gd;
extern unsigned int block_size;

// This struct allows us to keep some metadata of the inode we're looking at
struct NamedInode {
    int inode_num;
    char *name;
    struct ext2_inode *inode;
};

// Holds the existing path and name of the new directory to enter
struct PathTuple {
    char *path;
    char *dir_name;
};

extern void initialize_disk(const char *image);

extern struct ext2_inode * get_inode(int inode_num, const struct ext2_group_desc *group_desc);
extern struct NamedInode * find_in_dir(struct ext2_inode *dir_inode, char *file_name);
extern struct NamedInode * traverse_path(char *path);
extern int insert_inode(int inode_type);
extern struct ext2_dir_entry_2 * create_new_dir_entry(struct ext2_inode *dir_inode, int inode_num, char *name, int file_type);
extern int check_exists(struct ext2_inode * dir_inode, char *name);
extern unsigned int get_current_time();
extern int allocate_inode();
extern int allocate_block();
extern void free_inode(int block_num);
extern void free_block(int block_num);
extern void remove_dir_entry(struct ext2_inode *dir_inode, char *name);
extern void add_inode_block(struct ext2_inode *inode, int block_num);
extern struct PathTuple parse_directory_path(char *path);
extern int transfer_contents(char *contents, struct ext2_inode *file_inode);
