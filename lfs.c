#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "help.h"

int lfs_getattr( const char *, struct stat * );
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_createfile(const char *, mode_t, dev_t);
int lfs_createdir(const char *, mode_t);
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

int lfs_destoy(void* private_data){
	return 0;
}

int lfs_getattr( const char *path, struct stat *stbuf ) {

	struct treeNode* node = getNode(path);

	stbuf->st_size = (off_t) node->inode.size;
	stbuf->st_atim = node->inode.st_atim;
	stbuf->st_mtim = node->inode.st_mtim;

	if(node->isFile){
		stbuf->st_nlink = 1;
		stbuf->st_mode = S_IFREG | 0777;
	}else{
		int i = 0;

		while(note->dict[i] != NULL){
			i++;
		}
		stbuf->st_nlink = i + 2;
		stbuf->st_mode = S_IFDIR | 0755;
	}

	return 0;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {

	printf("readdir: (path=%s)\n", path);

	struct treeNode* node = getNode(path);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for(int i = 0; i<100 && node->dict[i] != NULL; i++){

		filler(buf, node->dict[i]->name, NULL, 0);

	}

	return 0;
}

int lfs_createfile(const char* path, mode_t mode, dev_t dev){
	createNode(path, 1);
	return 0;
}

int lfs_createdir(const char *path, mode_t mode){
	createNode(path, 0);
	return 0;
}

int lfs_removefile(const char* path){
	removeNode(path, 1);
	return 0;
}

int lfs_removedir(const char *path){
	createNode(path, 0);
	return 0;
}

//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {
	printf("open: (path=%s)\n", path);

	struct treeNode* node = getNode(path);

	fi->fh = (uint64_t) node;
	return 0;
}

int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    	printf("read: (path=%s)\n", path);
	memcpy( buf, "Hello\n", 6 );
	return 6;
}

//Nothing to doe here
int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("release: (path=%s)\n", path);
	return 0;
}



int main( int argc, char *argv[] ) {

	init();
	fuse_main( argc, argv, &lfs_oper );

	return 0;
}
