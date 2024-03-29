#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include "ext2.h"


unsigned char *disk;

static unsigned int block_size;

int get_file_descriptor(const char *image) {
    int fd = open(image, O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    return fd;
}

struct ext2_inode * read_inode(fd, inode_num, group_desc)
    int fd;
    int inode_num;
    const struct ext2_group_desc *group_desc; 
{
    struct ext2_inode *in = malloc(sizeof(struct ext2_inode));
    unsigned char *buffer = malloc(EXT2_BLOCK_SIZE);

    lseek(fd, 1024 + (group_desc->bg_inode_table - 1)*1024 + inode_num*sizeof(struct ext2_inode), SEEK_SET);
    read(fd, buffer, sizeof(struct ext2_inode));

    in = (struct ext2_inode *) buffer;

    return in;
}

int num_blocks(inode)
    const struct ext2_inode *inode;
{
    int num = 0;
    for (int i = 0; i < 15; i++)
        num += inode->i_block[i];

    return num;
}

void print_dir_contents(fd, in, i) 
    int fd;
    const struct ext2_inode * in;
    int i;
{
    void *block;
    if (S_ISDIR(in->i_mode) && in->i_size > 0 && (i == 1 || i > 10)) {
      if ((block = malloc(block_size)) == NULL) { /* allocate memory for the data block */
        fprintf(stderr, "Memory error\n");
        close(fd);
        exit(1);
      }

      lseek(fd, 1024 + (in->i_block[0] - 1)*block_size, SEEK_SET);
      read(fd, block, block_size);

      struct ext2_dir_entry_2 *de2 = (struct ext2_dir_entry_2 *) (block);
      int size = 0;
      while(de2->inode) {
        if (size == 0) {
          printf("    DIR BLOCK NUM: %d (for inode %d)\n", num_blocks(in), i+1);
        }
        char file_name[EXT2_NAME_LEN];
        memcpy(file_name, de2->name, de2->name_len);
        file_name[de2->name_len] = 0;
        printf("Inode: %d rec_len: %d name_len: %d type= %d name=%s\n", de2->inode, de2->rec_len, de2->name_len, de2->file_type, file_name);
        de2 = (void*) de2 + de2->rec_len;
        size += 1;
      }
      free(block);
    }
}


int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }

    int fd = get_file_descriptor(argv[1]);
    int i;

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

    printf("Inodes: \n");

    struct ext2_inode *in;
    for (int i = 1; i < sb->s_inodes_count; i++) {
        in = read_inode(fd, i, gd);
        char type = 'u';
        if ((S_ISDIR(in->i_mode) || S_ISREG(in->i_mode) || S_ISLNK(in->i_mode)) && in->i_size > 0 && (i == 1 || i > 10)) {
            if (S_ISDIR(in->i_mode))
                type = 'd';
            else if (S_ISREG(in->i_mode))
                type = 'f';
            else if (S_ISLNK(in->i_mode))
                type = 'l';
            
            printf("[%i] type: %c size: %d links: %d blocks: %d\n", i+1, type, in->i_size, in->i_links_count, in->i_blocks);
            printf("Blocks: %d\n", num_blocks(in));
        }
    }
    
    printf("\nDirectory Blocks:\n");

    block_size = 1024 << sb->s_log_block_size;
    for (int i = 1; i < sb->s_inodes_count; i++) {
      in = read_inode(fd, i, gd);
      print_dir_contents(fd, in, i);
    }
    
    printf("\n");
    return 0;
}

