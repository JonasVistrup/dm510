#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 512		//number of bytes in a block
#define SEGMENT_SIZE 2048	//number of blocks in a segment
#define TOTAL_SIZE 1000		//number of segments in storage


struct plist{
	char* p[64];
};

//Arraylist

struct Inode{		//size = 48 bytes
	size_t size;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct plist* plist;
};

struct treeNode{	//MAX = 512 bytes, size?
	char name[];
	struct Inode inode;
	struct treeNode** dict //Linkedlist? //Each treenode pointer is 4 bytes.
}

FILE* disk;
FILE* masterInfo;

int cleanerSeg;
int currentSeg;


struct treeNode* root;	//Always the first block

int restoreStructure(){
	root = currentSeg * SEGMENT_SIZE * BLOCK_SIZE;

}

int init(){
	//Open filesystem file
	if(!(disk = fopen("FileSystemFile","r+"))){
		if(!(disk = fopen("FileSystemFile","w+"))){
			return -1;
		}
		//Create "/" dicrectory
		root->name = "/";
		root->inode->size=0;
		root->inode->st_atim = timespec_get();
		root->inode->st_mtim = timespec_get();
		root->inode->plist = NULL;
		root->dict = NULL; //Linkedlist?
	}else{
		//Read the file and setup system
		restoreStructure();
	}

	//Open MasterInfo file
	if(!(masterInfo = fopen("MasterInfo","r+"))){
		if(!(masterInfo = fopen("MasterInfo","w+"))){
			return -1;
		}
		currentSeg = -1;
		cleanerSeg = 0;

		fwrite({currentSeg, cleanerSeg},sizeof(int),2,masterInfo);
	}else{
		int segInfo[2];
		fread(segInfo, sizeof(int),2,masterInfo);
		currentSeg = segInfo[0];
		cleanerSeg = segInfo[1];
	}
	return 0;
}


int main( int argc, char *argv[] ) {
	fuse_main( argc, argv, &lfs_oper );

	return 0;
}
