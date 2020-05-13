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
         struct timespec st_atim;
         struct timespec st_mtim;
         int plist;
};

struct int_Node{
//	int isFile;
        char name[60];
        struct int_Inode inode;
        int dict[100];
};

struct Inode{		//size = 48 bytes
	size_t size;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct plist* plist;
};

struct treeNode{	//MAX = 512 bytes, size?
	int isFile;
	char name[60];
	struct Inode inode;
	struct TreeNode* dict[100]; //Linkedlist? //Each treenode pointer is 4 bytes.
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

	tNode->name = node->name;
        tNode->isFile = node->isFile;
        tNode->inode.size = node->inode.size;
        tNode->inode.st_atim = node->inode.st_atim;
        tNode->inode.st_mtim = node->inode.st_mtim;
	int[128] plist = *((int[]*) seekNfind(node->inode.plist / 65536, node->inode.plist % 65536));
	struct plist* pl = malloc(sizeof(struct plist));

	for(int i = 0; i < 128; i++){

		if(plist[i] == -1){
			(*pl)[i] = -1;
		}else{

			(*pl)[i] = (char*) seekNfind(plist[i] / 65536, plist[i] % 65536);
		}

	}
	tNode->Inode->plist = pl;
	free(plist);
}


void restoreDir(struct int_Node* node, struct treeNode* tNode){

	tNode->name = node->name;
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

int restoreStructure(){
	//root = currentSeg * SEGMENT_SIZE * BLOCK_SIZE;
	struct int_Node* node = (struct int_Node*) seekNfind(currentSeg, 0); //check if written == 512

	root->name = node->name;
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

