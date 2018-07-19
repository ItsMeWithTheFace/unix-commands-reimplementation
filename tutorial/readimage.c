#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);
    int i;

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);

    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 2*EXT2_BLOCK_SIZE);
    // Locations of each component
    printf("Block group:\n");
    printf("    block bitmap: %d\n", gd->bg_block_bitmap);
    printf("    inode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("    inode table: %d\n", gd->bg_inode_table);
    printf("    free blocks: %d\n", gd->bg_free_blocks_count);
    printf("    free inodes: %d\n", gd->bg_free_inodes_count);
    printf("    used dirs: %d\n", gd->bg_used_dirs_count);

    unsigned char *bitmap = malloc(EXT2_BLOCK_SIZE);
    lseek(fd, (gd->bg_block_bitmap)*1024, SEEK_SET);
    read(fd, bitmap, EXT2_BLOCK_SIZE);

    // Prints out which blocks are being used
    printf("Block bitmap: ");
    char curr, temp;
    int j;
    for (i = 0; i < (sb->s_blocks_count / 8); i++) {
        curr = bitmap[i];

        for (j = 0; j < 8; j++) {
            temp = curr >> j;

            printf("%d", (temp & 1) > 0);
        }
        printf(" ");
    }
    printf("\n");

    lseek(fd, 1024 + (gd->bg_inode_bitmap - 1)*1024, SEEK_SET);
    read(fd, bitmap, EXT2_BLOCK_SIZE);

    // Prints out which inodes are being used
    printf("Inode bitmap: ");
    for (i = 0; i < (sb->s_inodes_count / 8); i++) {
        curr = bitmap[i];

        for (j = 0; j < 8; j++) {
            temp = curr >> j;

            printf("%d", (temp & 1) > 0);
        }
        printf(" ");
    }
    printf("\n");

    printf("\nInodes: \n");
    lseek(fd, 1024 + (gd->bg_inode_table - 1)*1024 + sizeof(struct ext2_inode), SEEK_SET);
    read(fd, bitmap, sizeof(struct ext2_inode));

    struct ext2_inode *in = (struct ext2_inode *) (bitmap);
    char type = 'u';
    if (S_ISDIR(in->i_mode))
        type = 'd';
    else if (S_ISREG(in->i_mode))
        type = 'f';
    printf("type: %c size: %d links: %d blocks: %d\n", type, in->i_size, in->i_links_count, in->i_blocks);
    
    printf("\nDirectory Blocks:\n");
    lseek(fd, 1024 + (gd->bg_inode_table - 1)*1024 + sizeof(struct ext2_inode)*sb->s_inodes_count, SEEK_SET);
    read(fd, bitmap, sizeof(struct ext2_dir_entry_2));

    struct ext2_dir_entry_2 *de2 = (struct ext2_dir_entry_2 *) (bitmap);
		unsigned int size = 0;

    while((size < in->i_size) && de2->inode) {
			char file_name[EXT2_NAME_LEN];
			memcpy(file_name, de2->name, de2->name_len);
			file_name[de2->name_len] = 0;
			printf("Inode: %d rec_len: %d name_len: %d type= %d name=%s\n", de2->inode, de2->rec_len, de2->name_len, de2->file_type, file_name);
			de2 = (void*) de2 + de2->rec_len;
			size += de2->rec_len;
		}
    
    printf("\n");
    return 0;
}

