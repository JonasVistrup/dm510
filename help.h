#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 512		//number of bytes in a block
#define SEGMENT_SIZE 2048	//number of blocks in a segment
#define TOTAL_SIZE 1000		//number of segments in storage

struct plist{
	char* p[128];
};

//Arraylist
struct int_Inode{           //size = 48 bytes
         size_t size;
         time_t st_atim;
         time_t st_mtim;
         int plist;
};

struct int_Node{
	int isFile;
        char name[60];
        struct int_Inode inode;
        int dict[100];
};

struct Inode{		//size = 48 bytes
	size_t size;
	time_t st_atim;
	time_t st_mtim;
	struct plist* plist;
};

struct treeNode{	//MAX = 512 bytes, size?
	int isFile;
	char name[60];
	struct Inode inode;
	struct treeNode* dict[100]; //Linkedlist? //Each treenode pointer is 4 bytes.
};

static FILE* disk;
static FILE* masterInfo;

int cleanerSeg;
int currentSeg;
int currentInode;

struct treeNode* root;	//Always the first block



// finds a block, reads it and saves it in local.
void* seekNfind(int segment, int block){

	void* local = malloc(sizeof(struct int_Node));
	int offset = (segment * SEGMENT_SIZE * BLOCK_SIZE) + (block * BLOCK_SIZE);
	fseek(disk, offset, SEEK_SET);
	int written = fread(local, BLOCK_SIZE, 1, disk);//check if written is 512 bytes

	return local;
}

void restoreFile(struct int_Node* node, struct treeNode* tNode){

	//tNode->name = node->name;
        strcpy(tNode->name, node->name);
	tNode->isFile = node->isFile;
        tNode->inode.size = node->inode.size;
        tNode->inode.st_atim = node->inode.st_atim;
        tNode->inode.st_mtim = node->inode.st_mtim;

	int (*ptr)[128]	= ((int(*)[128]) seekNfind(node->inode.plist / 65536, node->inode.plist % 65536));

	struct plist* pl = malloc(sizeof(struct plist));

	for(int i = 0; i < 128; i++){

		if((*ptr)[i] == -1){
			(pl->p)[i] = NULL;

		}else{
			(pl->p)[i] = (char*) seekNfind((*ptr)[i] / 65536, (*ptr)[i] % 65536);
		}

	}

	tNode->inode.plist = pl;
	free(ptr);
}


void restoreDir(struct int_Node* node, struct treeNode* tNode){

        strcpy(tNode->name, node->name);
        tNode->isFile = node->isFile;
        tNode->inode.size = node->inode.size;
        tNode->inode.st_atim = node->inode.st_atim;
        tNode->inode.st_mtim = node->inode.st_mtim;
        tNode->inode.plist = NULL;

        int i = 0;
        while(node->dict[i] == -1){

                struct int_Node* intNode = (struct int_Node*) seekNfind( (node->dict[i] / 65536), (node->dict[i] % 65536));

                struct treeNode* treeNode = (struct treeNode*) malloc(sizeof(struct treeNode));

                //check if file or directory
                if(intNode->isFile){
                        restoreFile(intNode, treeNode);
                }else{
                        restoreDir(intNode, treeNode);
                }
                tNode->dict[i] = treeNode;
                i++;
        }
        free(node);
}

// to be deleted:
int restoreStructure(){
	//root = currentSeg * SEGMENT_SIZE * BLOCK_SIZE;
	struct int_Node* node = (struct int_Node*) seekNfind(currentSeg, 0); //check if written == 512

        strcpy(root->name, node->name);
	root->isFile = node->isFile;
	root->inode.size = node->inode.size;
	root->inode.st_atim = node->inode.st_atim;
	root->inode.st_mtim = node->inode.st_mtim;
	root->inode.plist = NULL;

	int i = 0;
	while(node->dict[i] == -1){

		struct int_Node* intNode = (struct int_Node*) seekNfind( (node->dict[i] / 65536), (node->dict[i] % 65536));

		struct treeNode* treeNode = (struct treeNode*) malloc(sizeof(struct treeNode));

		//check if file or directory
		if(intNode->isFile){
			restoreFile(intNode, treeNode);
		}else{
			restoreDir(intNode, treeNode);
		}
		root->dict[i] = treeNode;
		i++;
	}
	free(node);
	return 0;
}

int init(){


	//Open filesystem file
	if(!(disk = fopen("FileSystemFile","r+"))){
		if(!(disk = fopen("FileSystemFile","w+"))){
			return -1;
		}

		//Create "/" dicrectory
	        strcpy(root->name, "/");
		root->inode.size=0;
		root->inode.st_atim = time(NULL);
		root->inode.st_mtim = time(NULL);
		root->inode.plist = NULL;

		for(int i = 0; i < 100; i++){
			root->dict[i] = NULL;
		}

		//root->dict = NULL; //Linkedlist?
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
		int array[] = {currentSeg, cleanerSeg};
		fwrite(array,sizeof(int),2,masterInfo);
	}else{
		int segInfo[2];
		fread(segInfo, sizeof(int),2,masterInfo);
		currentSeg = segInfo[0];
		cleanerSeg = segInfo[1];
	}
	return 0;
}