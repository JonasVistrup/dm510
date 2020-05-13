#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
struct plist{
	char* p[64]
}

//Arraylist

struct Inode{
	size_t size;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct plist* plist;
}


struct treeNode{
	char* name;
	struct Inode* inode;
	struct treeNode** dict //Linkedlist?
}


FILE* fd;
int systemSize = 1000*(1024*1024);

struct treeNode* top;

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

int lfs_createfile(const char * path, mode_t mode, dev_t dev){
	struct treeNode current = top;
	char currentName[50];
	int i=0;
	for(char c = *path; c!=NULL; c++){
		if(c == '/'){
			//Find treeNode in current.dict where treeNode.name == currentName
			currentName = char[50];
		}else{
			currentName[i] = c;
			i++;
		}
	}

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
int init(){
	//Open filesystem file
	if(!(fd = fopen("FileSystemFile","r+"))){
		if(!(fd = fopen("FileSystemFile","w+"))){
			return -1;
		}
		//Create "/" dicrectory
		top->name = "/";
		top->inode->size=0;
		top->inode->st_atim = timespec_get();
		top->inode->st_mtim = timespec_get();
		top->inode->plist = NULL;
		top->dict = NULL; //Linkedlist?
	}
}


int main( int argc, char *argv[] ) {
	fuse_main( argc, argv, &lfs_oper );

	return 0;
}
