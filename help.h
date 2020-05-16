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
	int coordinate;
	struct plist* plist;
};

struct treeNode{	//MAX = 512 bytes, size?
	int isFile;
	char name[60];
	struct Inode inode;
	struct treeNode* dict[100]; //Linkedlist? //Each treenode pointer is 4 bytes.
};

union block{
	struct int_Node node;
	int plist[128];
	char data[BLOCK_SIZE];
};

union block segment[SEGMENT_SIZE];

static FILE* disk;
static FILE* masterInfo;

int currentBlock;
int cleanerSeg;
int currentSeg;
int numberOfNodes;

struct treeNode* root;	//Always the first block

char** pathSplit(const char* path){

	printf("Entering pathsplit\n");

	if(!strcmp(path, "/")){
	//	char* dum[0];

	printf("Exiting pathsplit: path != "/"\n");
		return NULL;
	}


	const char* pathcopy = path;

	int i = 0;

	for(char c = *pathcopy; c != '\0'; pathcopy ++){

		c = *pathcopy;

		if(c == '/'){

			i++;
		}
	}

	pathcopy = path + 1;

	char* (*split)[i];

	split = malloc(sizeof(split));

	i = 0;

	char current[60];

	memset(current, 0, sizeof(current));

	int j = 0;

	for(char c = *pathcopy; c != '\0'; pathcopy ++){

		c = *pathcopy;

		if(c == '/'){

			(*split)[i] = current;
			memset(current, 0, sizeof(current));
			j = 0;
			i++;
		}else{

			current[j] = c;
			j++;
		}


	}

	printf("Exting pathSpilt\n");

	return *split;
}





//creates a file

int reverseTree(struct treeNode* current){
	printf("Entering reverseTree\n");
	struct  int_Node node;

	for(int i = 0; i < 100; i++){

		if(current->dict[i] == NULL){
			node.dict[i] = -1;

		}else{
			node.dict[i] = reverseTree(current->dict[i]);

		}
	}

	strcpy(node.name, current->name);
	node.isFile = current->isFile;
	node.inode.size = current->inode.size;
	node.inode.st_atim = current->inode.st_atim;
	node.inode.st_mtim = current->inode.st_mtim;
	node.inode.plist= current->inode.coordinate;

	segment[currentBlock].node = node;
	currentBlock++;
	printf("Exiting reverseTree\n");

	return currentSeg * 65536 + currentBlock -1;
}


int segmentCtrl(){
	printf("Entering segmentCTRL\n");
	if((currentBlock + numberOfNodes) >= SEGMENT_SIZE){
		reverseTree(root);
		//write out segment
		fwrite(&segment, BLOCK_SIZE, SEGMENT_SIZE, disk);
		currentSeg++;
		currentBlock = 0;

		int array[] = {currentSeg, cleanerSeg};
		fseek(masterInfo, 0, SEEK_SET);
		fwrite(array, sizeof(int), 2 , masterInfo);
	}
	printf("Exiting segmentCtrl\n");

	return 0;
}

int createNode(const char* path, int isFile){
	printf("Entering createNode\n");
	printf("%s\n",path);

	const char s[2] = "/";
	char* path_copy = malloc(strlen(path));
	strcpy(path_copy,path);
	char* token = strtok(path_copy, s);
	printf("%s\n", token);
	struct treeNode* current = root;

	int flag = 0;

	while(token != NULL && !flag){


		int i = 0;
		while(i < 100 && current->dict[i] != NULL && strcmp(current->dict[i]->name, token)){
			i++;
		}
		if(i == 100){
			// file found.
		}else if(current->dict[i] == NULL){
			if(isFile){
				struct treeNode* newFile = (struct treeNode*) malloc(sizeof(struct treeNode));
				strcpy(newFile->name, token);
				newFile->isFile = 1;
				newFile->inode.size = 0;
				newFile->inode.st_atim = time(NULL);
				newFile->inode.st_mtim = time(NULL);
				newFile->inode.plist = (struct plist*) calloc(128, sizeof(char*));
				for(i = 0; i<128; i++){
					segment[currentBlock].plist[i] = -1;
				}
				newFile->inode.coordinate = currentSeg * 65536 + currentBlock;
				currentBlock++;

				for(int i = 0; i < 100; i++){
					newFile->dict[i] = NULL;
				}
				current->dict[i] = newFile;
				flag = 1;
			}else{
				struct treeNode* newDir = (struct treeNode*) malloc(sizeof(struct treeNode));
        		        strcpy(newDir->name, token);
        		        newDir->isFile = 0;
                		newDir->inode.size = 0;
               	         	newDir->inode.st_atim = time(NULL);
                        	newDir->inode.st_mtim = time(NULL);
                        	newDir->inode.plist = NULL;

	                        for(int i = 0; i < 100; i++){
        	                        newDir->dict[i] = NULL;
                        	}

                        	current->dict[i] = newDir;
                        	flag = 1;
			}
		}
		current = current->dict[i];
		token = strtok(NULL,s);
		printf("%s\n",token);
	}

	if(flag == 0){
	//error lol, so reletable
	printf("Exiting createNode\n");

		return -1;
	}
	numberOfNodes++;
	printf("Exiting createNode\n");

	return 0;
}

int removeNode(const char* path, int isFile){
	printf("Entering removeNode \n");
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
			printf("Exiting removeNode: i == 100\n");

                        return -1;
                }
                token = strtok(NULL,s);
                if(token == NULL){
                        if(isFile){
                                struct treeNode* node = current->dict[i];
                                for(int j = 0; j < 128 ; j++){
					if(node->inode.plist->p[j]!=NULL){
                                                free(node->inode.plist->p[j]);
					}
                                }
                                free(node->inode.plist);
                                free(current->dict[i]);
                                current->dict[i] = NULL;

                        } else if(current->dict[0] != NULL){
				//files or directories in dir you're trying to remove
				printf("Exiting removeNode: current->dict[0] != NULL\n");

				return -1;

			} else {
				free(current->dict[i]);
				current->dict[i] = NULL;
			}
		}
		current = current->dict[i];
        }
	printf("Exiting removeNode\n");

	return 0;
}

struct treeNode* findNode(const char* path){

	printf("Entering findNode\n");

	printf("path = %s\n", path);
	char** split;

	split = pathSplit(path);

	if(split == NULL){

		printf("Exiting findNode split == NULL\n");

		return root;
	}

	printf("after pathsplit\n");

	struct treeNode* node = root;

	int j;

	for(int i = 0; i < sizeof(split) / sizeof(split[0]); i++){

		for(j = 0; j < 100 && node->dict[j] != NULL && strcmp(node->dict[j]->name, split[i]); j++){}

		if(j == 100 || node->dict[j] == NULL){
			printf("Exiting findNode j== 100 or node->dict[j] == NULL\n");

			return NULL;
		}

		node = node->dict[j];
	}
	printf("Exiting findNode\n");

	return node;
}

// finds a block, reads it and saves it in local.
void* seekNfind(int segment, int block){
	printf("Entering seekNfind\n");

	void* local = malloc(sizeof(struct int_Node));
	int offset = (segment * SEGMENT_SIZE * BLOCK_SIZE) + (block * BLOCK_SIZE);
	fseek(disk, offset, SEEK_SET);
	int written = fread(local, BLOCK_SIZE, 1, disk);//check if written is 512 bytes
	printf("Exiting seekNfind\n");

	return local;
}

void restoreFile(struct int_Node* node, struct treeNode* tNode){
	printf("Entering restoreFile\n");

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
	printf("Exiting restoreFile\n");

}


void restoreDir(struct int_Node* node, struct treeNode* tNode){
	printf("Entering restoreDir\n");

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
	printf("Exiting restoreDir\n");

}

// to be deleted:
int restoreStructure(){
	printf("Entering restoreStructure\n");

	//root = currentSeg * SEGMENT_SIZE * BLOCK_SIZE;
	struct int_Node* node = (struct int_Node*) seekNfind(currentSeg, SEGMENT_SIZE-1); //check if written == 512

	numberOfNodes = 1;

	root = (struct treeNode*) malloc(sizeof(struct treeNode));

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
	printf("Exiting restoreStructure\n");

	return 0;
}

int init(){
	printf("Entering init\n");

	//Open filesystem file
	if(!(disk = fopen("FileSystemFile","r+"))){
		if(!(disk = fopen("FileSystemFile","w+"))){
			printf("Exiting init: disk = fopen returns error\n");

			return -1;
		}

		numberOfNodes = 1;
		root = (struct treeNode*) malloc(sizeof(struct treeNode));

		//Create "/" dicrectory
	        strcpy(root->name, "/");
		root->inode.size=0;
		root->inode.st_atim = time(NULL);
		root->inode.st_mtim = time(NULL);
		root->inode.coordinate = -1;
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
			printf("Exiting init: master = fopen returns error\n");

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
	currentBlock = 0;
	printf("Exiting init\n");

	return 0;
}
