#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "help.h"

void lfs_destroy(void*);
int lfs_getattr(const char *, struct stat * );
int lfs_readdir(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_createfile(const char *, mode_t, dev_t);
int lfs_createdir(const char *, mode_t);
int lfs_removefile(const char*);
int lfs_removedir(const char *);
int lfs_truncate(const char *path, off_t offset);
int lfs_open(const char *, struct fuse_file_info * );
int lfs_read(const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *path, struct fuse_file_info *fi);
int lfs_write(const char *path, const char *content, size_t content_length, off_t offset, struct fuse_file_info *fi);

static struct fuse_operations lfs_oper = {
	.destroy	= lfs_destroy,
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod		= lfs_createfile,
	.mkdir 		= lfs_createdir,
	.unlink 	= lfs_removefile,
	.rmdir 		= lfs_removedir,
	.truncate 	= lfs_truncate,
	.open		= lfs_open,
	.read		= lfs_read,
	.release 	= lfs_release,
	.write		= lfs_write,
	.rename = NULL, //Ignore
	.utime = NULL	//Ignore
};

/*  Saves our segments into FilesystemFile and saves the current segment
 *  and the cleaner segment into MasterInfo. Frees memory afterwards:
 */
void lfs_destroy(void* private_data){
	printf("Entering destroy\n");
	//1 gem segment i filesystemfile
	currentBlock = SEGMENT_SIZE-numberOfNodes;
	segmentCtrl();

	//2 free memory
	llClean(head);

	freeTree(root);

	fclose(masterInfo);
	fclose(disk);
	printf("Exiting lfs_destroy\n");
}

// Find the requested node and returns the attributes of the node:
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

// Read the directory by finding the requested node from a given path and then prints out its "childrens" names:
int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	printf("Entering lfs_readdir\n");

	printf("path = %s\n", path);

	struct treeNode* node = findNode(path);
	if(node == NULL){
		printf("Node is NULL\n");
		return -ENOENT;
	}


	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for(int i = 0; i<100 && node->dict[i] != NULL; i++){
		printf("%s->dict[%d] = %s\n", node->name, i, node->dict[i]->name);
		filler(buf, node->dict[i]->name, NULL, 0);

	}
	printf("Exiting lfs_readdir\n");

	return 0;
}

// Creates a file at given path:
int lfs_createfile(const char* path, mode_t mode, dev_t dev){
	printf("Entering lfs_createfile\n");
	int retVal = createNode(path, 1);
	printf("Exiting lfs_createfile\n");
	return retVal;
}

// Creates a directory at a given path:
int lfs_createdir(const char *path, mode_t mode){
	printf("Entering lfs_createdir\n");
	int retVal = createNode(path, 0);
	printf("Exiting lfs_creatdir\n");
	return retVal;
}

// Removes a file at a given path:
int lfs_removefile(const char* path){
	printf("Entering lfs_removefile\n");
	int retVal = removeNode(path, 1);
	printf("Exiting lfs_removefile\n");
	return retVal;
}

// Removes a directory at a given path:
int lfs_removedir(const char *path){
	printf("Entering lfs_removedir\n");
	int retVal = removeNode(path, 0);
	printf("Exiting lfs_removedir\n");

	return retVal;
}

// Resizes a file at a given path:
int lfs_truncate(const char *path, off_t offset) {
	printf("Entering lfs_truncate\n");
	(void)offset;

	struct treeNode* node = findNode(path);
	if(node == NULL){
		return -ENOENT;
	}
	for(int i=0; i<128; i++){
		if(node->inode.plist->p[i] != NULL){
			free(node->inode.plist->p[i]);
			node->inode.plist->p[i] = NULL;
		}
	}
	for(int i=0; i<128; i++){
		(*segment)[currentBlock].plist[i] = -1;
	}
	node->inode.coordinate = currentSeg * 65536 + currentBlock;
	currentBlock++;
	segmentCtrl();
	node->inode.size = 0;
	printf("Exiting lfs_truncate\n");

	return 0;
}

// Opens a file at a given path:
int lfs_open( const char *path, struct fuse_file_info *fi ) {
	printf("Entering lfs_open\n");
	printf("open: (path=%s)\n", path);

	struct treeNode* node = findNode(path);

	fi->fh = (uint64_t) node;
	printf("Exiting lfs_open\n");

	return 0;
}

// Reads a file at a given path:
int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
   	printf("Entering lfs_read\n");

	struct treeNode* node = (struct treeNode*) fi->fh;
	int startpoint = offset / BLOCK_SIZE;
	int remainder = offset % BLOCK_SIZE;
	int amount_read = 0;

	for(int i = startpoint; i<128 && amount_read<size; i++){
		if(node->inode.plist->p[i] == NULL){
			size = size - (BLOCK_SIZE - remainder);
		}else{
			for(int j = remainder; j<BLOCK_SIZE && amount_read<size; j++){
				if(node->inode.plist->p[i][j]!='\0'){
					buf[amount_read] = node->inode.plist->p[i][j];
					printf("char =%c\n",node->inode.plist->p[i][j]);
					amount_read++;
				}
			}
		}
		remainder = 0;
	}
	printf("Exiting lfs_read\n");

	return amount_read;
}

//Does literally nothing!:
int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("Entering lfs_release\n");
	printf("Exiting lfs_release\n");
	return 0;
}

// Writes to a file at a given path:
int lfs_write(const char *path, const char *content, size_t content_length, off_t offset, struct fuse_file_info *fi){
	printf("Entering lfs_write\n");
	struct treeNode* node = (struct treeNode*) fi->fh;
	if(node == NULL){
		printf("Node is Null\n");
	}
	int startingpoint = offset / BLOCK_SIZE;
	int remainder = offset % BLOCK_SIZE;
	int currentc = 0;
	for(int i = startingpoint; i<128 && currentc<content_length; i++){
		if(node->inode.plist->p[i] == NULL){
			node->inode.plist->p[i] = calloc(BLOCK_SIZE, sizeof(char));
		}
		for(int j = remainder; j<BLOCK_SIZE && currentc<content_length; j++){
			node->inode.plist->p[i][j] = content[currentc++];
		}
		remainder = 0;
	}

	//insert blocks into segments
	int plist[128];
	for(int i = 0; i<128; i++){
		if(node->inode.plist->p[i] == NULL){
			plist[i] = -1;
		}else{
			for(int j = 0; j<BLOCK_SIZE; j++){
				(*segment)[currentBlock].data[j] = node->inode.plist->p[i][j];
			}
			plist[i] = currentSeg * 65536 + currentBlock;
			printf("SegInsert data: Seg: %d, Block: %d\n",currentSeg, currentBlock);
			currentBlock++;
			segmentCtrl();
		}
	}

	for(int i=0; i<128; i++){
		(*segment)[currentBlock].plist[i] = plist[i];
	}
	node->inode.coordinate = currentSeg * 65536 + currentBlock;
	int size = 0;
	for(int i=0; i<128; i++){
		if(node->inode.plist->p[i] != NULL){
			for(int j=0; j<BLOCK_SIZE; j++){
				if(node->inode.plist->p[i][j] != '\0'){
					size++;
				}
			}
		}
	}
	node->inode.size = size;
	printf("SegInsert plist: Seg %d, Block %d\n", currentSeg, currentBlock);
	currentBlock++;
	segmentCtrl();
	printf("Exiting lfs_write\n");
	return currentc;
}

// Initalizes the filesystem:
int main( int argc, char *argv[] ) {
	printf("Entering main\n");
	init();
	fuse_main( argc, argv, &lfs_oper );
	printf("Exiting main\n");

	return 0;
}
