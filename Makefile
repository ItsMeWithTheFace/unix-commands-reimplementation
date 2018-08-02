all: ext2_ls ext2_mkdir

ext2_ls: ext2_ls.c
	gcc -Wall -o ext2_ls ext2_ls.c ext2_util.c

ext2_mkdir: ext2_mkdir.c
	gcc -Wall -o ext2_mkdir ext2_mkdir.c ext2_util.c

clean:
	rm ext2_ls ext2_mkdir
