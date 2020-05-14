#include "help.h"
#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int lfs_getattr( const char *, struct stat * );
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_createfile(const char *, mode_t, dev_t);
int lfs_open( const char *, struct fuse_file_info * );
int lfs_read( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *path, struct fuse_file_info *fi);

static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod		= lfs_createfile,
	.mkdir = NULL,
	.unlink = NULL, //Ignore
	.rmdir = NULL,
	.truncate = NULL,
	.open	= lfs_open,
	.read	= lfs_read,
	.release = lfs_release,
	.write = NULL,
	.rename = NULL, //Ignore
	.utime = NULL
};

int lfs_getattr( const char *path, struct stat *stbuf ) {
	int res = 0;
	printf("getattr: (path=%s)\n", path);

	memset(stbuf, 0, sizeof(struct stat));
	if( strcmp( path, "/" ) == 0 ) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if( strcmp( path, "/hello" ) == 0 ) {
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = 12;
	} else
		res = -ENOENT;

	return res;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	(void) offset;
	(void) fi;
	printf("readdir: (path=%s)\n", path);

	if(strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, "hello", NULL, 0);

	return 0;
}

int lfs_createfile(const char* path, mode_t mode, dev_t dev){
	const char s[2] = "/";
	char* path_copy = malloc(strlen(path));
	strcpy(path_copy,path);
	char* token = strtok(path_copy, s);

	struct treeNode* current = root;

	int flag = 0;

	while(token != NULL && !flag){

		int i = 0;
		while(i < 100 && current->dict[i] != NULL &&strcmp(current->dict[i]->name, token)){
			i++;
		}
		if(i == 100){
			// file found.
		}else if(current->dict[i] == NULL){

			struct treeNode* newFile = malloc(sizeof(struct treeNode));
			strcpy(newFile->name, token);
			newFile->isFile = 1;
			newFile->inode.size = 0;
			newFile->inode.st_atim = time(NULL);
			newFile->inode.st_mtim = time(NULL);
			newFile->inode.plist = (struct plist*) calloc(128, sizeof(char*));

			for(int i = 0; i < 100; i++){
				newFile->dict[i] = NULL;
			}
			current->dict[i] = newFile;
			flag = 1;
		}
		current = current->dict[i];
		token = strtok(NULL,s);
	}

	if(flag == 0){
		//error lol, so reletable
		return -1;
	}
	//Increase number of inodes!!!!!!!!!!!!!!!!!!!!!!!
	return 0;
}

//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {
    printf("open: (path=%s)\n", path);
	return 0;
}

int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    printf("read: (path=%s)\n", path);
	memcpy( buf, "Hello\n", 6 );
	return 6;
}

int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("release: (path=%s)\n", path);
	return 0;
}



int main( int argc, char *argv[] ) {

	init();
	fuse_main( argc, argv, &lfs_oper );

	return 0;
}
