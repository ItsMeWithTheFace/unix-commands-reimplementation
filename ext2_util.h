#include "ext2.h"

#define TYPE_FILE   1
#define TYPE_LINK   7
#define TYPE_DIR    2

#define EXT2_BLOCK_OFFSET(block) (EXT2_BLOCK_SIZE + (block - 1)*EXT2_BLOCK_SIZE)

// This struct allows us to keep the name of the inode we're looking at
struct NamedInode {
    char *name;
    struct ext2_inode *inode;
};

extern int get_file_descriptor(const char *image);

extern struct ext2_inode * get_inode(int fd, int inode_num, const struct ext2_group_desc *group_desc);
extern struct ext2_inode * find_in_dir(int fd, struct ext2_inode *dir_inode, char *file_name);
extern struct NamedInode * traverse_path(int fd, char *path);
