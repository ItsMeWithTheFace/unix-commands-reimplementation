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

// This struct allows us to keep the name of the inode we're looking at
struct NamedInode {
    int inode_num;
    char *name;
    struct ext2_inode *inode;
};

extern void initialize_disk(const char *image);

extern struct ext2_inode * get_inode(int inode_num, const struct ext2_group_desc *group_desc);
extern struct NamedInode * find_in_dir(struct ext2_inode *dir_inode, char *file_name);
extern struct NamedInode * traverse_path(char *path);
extern int insert_inode(int inode_type);
extern struct ext2_dir_entry_2 * create_new_dir_entry(struct ext2_inode *dir_inode, int inode_num, char *name, int file_type);
