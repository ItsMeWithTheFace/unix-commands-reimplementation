all: ext2_ls

ext2_ls: ext2_ls.c
	gcc -Wall -o ext2_ls ext2_ls.c ext2_util.c

clean:
	rm ext2_ls
