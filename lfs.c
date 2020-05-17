#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "help.h"

void lfs_destroy(void*);
int lfs_getattr( const char *, struct stat * );
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_createfile(const char *, mode_t, dev_t);
int lfs_createdir(const char *, mode_t);
int lfs_removefile(const char*);
int lfs_removedir(const char *);
int lfs_open( const char *, struct fuse_file_info * );
int lfs_read( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *path, struct fuse_file_info *fi);

static struct fuse_operations lfs_oper = {
	.destroy	= lfs_destroy,
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod		= lfs_createfile,
	.mkdir 		= lfs_createdir,
	.unlink 	= lfs_removefile,
	.rmdir 		= lfs_removedir,
	.truncate = NULL,
	.open	= lfs_open,
	.read	= lfs_read,
	.release = lfs_release,
	.write = NULL,
	.rename = NULL, //Ignore
	.utime = NULL
};

void lfs_destroy(void* private_data){
	printf("Entering destroy\n");


	printf("Exiting lfs_destroy\n");
}

int lfs_getattr( const char *path, struct stat *stbuf ) {
	printf("Entering lfs_getattr\n");

	memset(stbuf, 0, sizeof(struct stat));

	struct treeNode* node = findNode(path);

	struct timespec a;
        struct timespec m;

	if(node == NULL){
	printf("Exiting lfs_getattr: node == NULL\n");
		return -ENOENT;
	}

	stbuf->st_size = (off_t) node->inode.size;

	a.tv_sec = node->inode.st_atim;
	a.tv_nsec = 0;

	m.tv_sec = node->inode.st_mtim;
	m.tv_nsec = 0;

	stbuf->st_atim = a;
	stbuf->st_mtim = m;

	if(node->isFile){
		stbuf->st_nlink = 1;
		stbuf->st_mode = S_IFREG | 0777;
	}else{
		int i = 0;

		while(node->dict[i] != NULL){
			i++;
		}
		stbuf->st_nlink = i + 2;
		stbuf->st_mode = S_IFDIR | 0755;
	}
	printf("Exiting lfs_getattr\n");
	return 0;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	printf("Entering lfs_readdir\n");

	printf("readdir: (path=%s)\n", path);

	struct treeNode* node = findNode(path);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for(int i = 0; i<100 && node->dict[i] != NULL; i++){

		filler(buf, node->dict[i]->name, NULL, 0);

	}
	printf("Exiting lfs_readdir\n");

	return 0;
}

int lfs_createfile(const char* path, mode_t mode, dev_t dev){
	printf("Entering lfs_createfile\n");
	createNode(path, 1);
	printf("Exiting lfs_createfile\n");
	return 0;
}

int lfs_createdir(const char *path, mode_t mode){
	printf("Entering lfs_createdir\n");
	createNode(path, 0);
	printf("Exiting lfs_creatdir\n");

	return 0;
}

int lfs_removefile(const char* path){
	printf("Entering lfs_removefile\n");
	removeNode(path, 1);
	printf("Exiting lfs_removefile\n");

	return 0;
}

int lfs_removedir(const char *path){
	printf("Entering lfs_removedir\n");
	removeNode(path, 0);
	printf("Exiting lfs_removedir\n");

	return 0;
}

//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {
	printf("Entering lfs_open\n");
	printf("open: (path=%s)\n", path);

	struct treeNode* node = findNode(path);

	fi->fh = (uint64_t) node;
	printf("Exiting lfs_open\n");

	return 0;
}

int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
   	printf("Entering lfs_read\n");

	printf("read: (path=%s)\n", path);
	memcpy( buf, "Hello\n", 6 );
	printf("Exiting lfs_read\n");

	return 6;
}

//Nothing to doe here
int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("Entering lfs_release\n");
	printf("release: (path=%s)\n", path);
	printf("Exiting lfs_release\n");

	return 0;
}



int main( int argc, char *argv[] ) {
	printf("Entering main\n");
	init();
	fuse_main( argc, argv, &lfs_oper );
	printf("Exiting main\n");

	return 0;
}
