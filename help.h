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
	int dummythicc[4];
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


union block (*segment)[SEGMENT_SIZE];

static FILE* disk;
static FILE* masterInfo;

int currentBlock;
int cleanerSeg;
int currentSeg;
int numberOfNodes;

struct treeNode* root;	//Always the first block


struct string{
	char* s;
	int length;
};
typedef struct string string;

struct stringArray{
	string* ss;
	int length;
};
typedef struct stringArray stringArray;

int segmentCtrl();

stringArray pathSplit(string path){
	printf("In pathsplit\n");
	if(!strcmp(path.s, "/")){
		stringArray retVal = {.ss = NULL, .length = 0};
		return retVal;
	}

	int counter = 0;
	for(int i = 0; i < path.length; i++){
		if(path.s[i] == '/'){
			counter++;
		}
	}
	stringArray retVal;
	retVal.ss = malloc(sizeof(string)*counter);
	retVal.length = counter;

	string* next = malloc(sizeof(string));
	next->s = malloc(sizeof(char)*60);
	int nextPos = 0;
	int ncounter = 0;

	for(int i = 1; i<path.length; i++){
		if(path.s[i] == '/'){
			//printf("In if /\n");
			next->length = nextPos;
			retVal.ss[ncounter] = *next;
			ncounter++;
			nextPos=0;
			next = malloc(sizeof(string));
			next->s = malloc(sizeof(char)*60);
			//printf("out of if /\n");
		}else{
			//printf("in else\n");
			next->s[nextPos] = path.s[i];
			nextPos++;
			//printf("out of else\n");
		}
	}
	retVal.ss[ncounter] = *next;
	printf("Exiting pathSplit\n");
	return retVal;

}

int cleaner(struct treeNode* tNode){
	printf("Entering Cleaner for node: %s\n", tNode->name);
	if(tNode->isFile){
		int plist[128];
		for(int i = 0; i < 128; i++){
			if(tNode->inode.plist->p[i] == NULL){
				plist[i] = -1;
			}else{
				for(int j = 0; j<BLOCK_SIZE; j++){
					(*segment)[currentBlock].data[j] = tNode->inode.plist->p[i][j];
				}
				plist[i] = currentSeg * 65536 + currentBlock;
				currentBlock++;
				segmentCtrl();
			}
		}
		for(int i = 0; i<128; i++){
			(*segment)[currentBlock].plist[i] = plist[i];
		}
		tNode->inode.coordinate = currentSeg * 65536 + currentBlock;
		currentBlock++;
		segmentCtrl();
	}else{
		for(int i = 0; i < 100 && tNode->dict[i] != NULL; i++){
			cleaner(tNode->dict[i]);
		}
	}


	printf("Exit Cleaner for node: %s\n", tNode->name);
}


//creates a file

int reverseTree(struct treeNode* current){
	printf("Entering reverseTree\n");
	struct int_Node* node = malloc(sizeof(struct int_Node));

	for(int i = 0; i < 100; i++){

		if(current->dict[i] == NULL){
			node->dict[i] = -1;

		}else{
			node->dict[i] = reverseTree(current->dict[i]);

		}
	}

	strcpy(node->name, current->name);
	node->isFile = current->isFile;
	node->inode.size = current->inode.size;
	node->inode.st_atim = current->inode.st_atim;
	node->inode.st_mtim = current->inode.st_mtim;
	node->inode.plist= current->inode.coordinate;
	printf("For %s: Int node, dict[0] is %d; CurrentSeg = %d; CurrentBlock = %d\n", node->name, node->dict[0], currentSeg, currentBlock);
	(*segment)[currentBlock].node = *node;
	currentBlock++;
	printf("Exiting reverseTree\n");

	return currentSeg * 65536 + currentBlock -1;
}


int segmentCtrl(){
	printf("Entering segmentCTRL\n");
	if((currentBlock + numberOfNodes) == SEGMENT_SIZE){
		reverseTree(root);
		//write out segment
		printf("Before Write\n");
		int written = fwrite(segment, BLOCK_SIZE, SEGMENT_SIZE, disk);
		if(written != SEGMENT_SIZE){
			printf("Write fail, written blocks = %d\n", written);
		}
		printf("After Write\n");
		memset(segment, 0, SEGMENT_SIZE * BLOCK_SIZE);
		currentSeg++;
		if(currentSeg == TOTAL_SIZE){
			fseek(disk, 0, SEEK_SET);
			currentSeg = 0;
		}
		printf("CurrentSeg = %d\n", currentSeg);
		currentBlock = 0;

		int array[] = {currentSeg, cleanerSeg};
		if(fseek(masterInfo, 0, SEEK_SET)){
			printf("Seek for masterInfo failed\n");
		}
		written = fwrite(array, sizeof(int), 2 , masterInfo);
		if(written != 2){
			printf("Write to masterInfo failed, written ints = %d\n", written);
		}
		if(cleanerSeg<currentSeg){
			if(TOTAL_SIZE - currentSeg + cleanerSeg <=50){
				cleanerSeg = currentSeg;
				cleaner(root);
				currentBlock = SEGMENT_SIZE - numberOfNodes;
				segmentCtrl();
			}
		}else{
			if(cleanerSeg - currentSeg <=50){
				cleanerSeg = currentSeg;
				cleaner(root);
				currentBlock = SEGMENT_SIZE - numberOfNodes;
				segmentCtrl();
			}
		}
	}
	printf("Exiting segmentCtrl\n");

	return 0;
}

int createNode(const char* path, int isFile){
	printf("Entering createNode\n");
	printf("%s\n",path);

	string s =  {.s = path, .length = strlen(path)};
	stringArray split = pathSplit(s);

	if(split.ss == NULL){
		printf("Exiting createNode split == NULL\n");

		return -1;
	}

	struct treeNode* node = root;
	int j;

	int k;

	for(k = 0; k < split.length-1; k++){

		for(j = 0; j < 100 && node->dict[j] != NULL && strcmp(node->dict[j]->name, split.ss[k].s); j++){}

		if(j == 100 || node->dict[j] == NULL){
			printf("Exiting createNode j== 100 or node->dict[j] == NULL\n");

			return -ENOENT;
		}

		node = node->dict[j];
	}

	for(j = 0; j < 100 && node->dict[j] !=NULL; j++){}

	if(j == 100){
		printf("No space in dict\n");
		return -ENOMEM;
	}

	if(isFile){
		struct treeNode* newFile = (struct treeNode*) malloc(sizeof(struct treeNode));
		printf("inside isFile\n");
                strcpy(newFile->name, split.ss[k].s); //split[k]
		//strcpy(newFile->name, split[sizeof(split) / sizeof(split[0])-1]);
		newFile->isFile = 1;
		newFile->inode.size = 0;
		newFile->inode.st_atim = time(NULL);
		newFile->inode.st_mtim = time(NULL);
		printf("before calloc\n");
		newFile->inode.plist = (struct plist*) calloc(128, sizeof(char*));
		printf("after calloc\n");
		for(int i = 0; i<128; i++){
			(*segment)[currentBlock].plist[i] = -1;
		}
		printf("after loop\n");
		newFile->inode.coordinate = currentSeg * 65536 + currentBlock;
		currentBlock++;
		printf("after coordinate\n");

		for(int i = 0; i < 100; i++){
			newFile->dict[i] = NULL;
		}
		node->dict[j] = newFile;
	}else{
		printf("is dir - createNode\n");

		struct treeNode* newDir = (struct treeNode*) malloc(sizeof(struct treeNode));
		//the splitted path disappears when we malloc memory for a treeNode

		printf("Before string cp\n");
                strcpy(newDir->name, split.ss[k].s); //split[k]
		printf("After string cp\n");
                newDir->isFile = 0;
               	newDir->inode.size = 0;
           	newDir->inode.st_atim = time(NULL);
                newDir->inode.st_mtim = time(NULL);
		newDir->inode.coordinate = -1;
                newDir->inode.plist = NULL;

		printf("Before for - createNode\n");

	        for(int i = 0; i < 100; i++){
	 		newDir->dict[i] = NULL;
		}
		node->dict[j] = newDir;

		printf("Exiting is dir - createNode\n");
	}
	numberOfNodes++;
	printf("Exiting createNode\n");

	return 0;
}

int removeNode(const char* path, int isFile){
	printf("Entering removeNode \n");
	printf("%s\n",path);

	struct treeNode* current = root;

	string s = {.s = path, .length = strlen(path)};
	stringArray split = pathSplit(s);

	if(split.ss == NULL){
		printf("Exiting removeNode split == NULL\n");
		return -1;
	}

	int k;
	int j = 0;

	for(k = 0; k < split.length-1; k++){
		//printf("printing split[0] in for-loop:  %s\n", split[k]);

		for(j = 0; j < 100 && current->dict[j] != NULL && strcmp(current->dict[j]->name, split.ss[k].s); j++){}

		if(j == 100 || current->dict[j] == NULL){
			printf("Exiting removeNode j== 100 or node->dict[j] == NULL\n");
			return -ENOENT;
		}
		printf("HEEEELLLOOOOOOO\n");
		current = current->dict[j];
	}

	int i = 0;
	while(i < 100 && current->dict[i] != NULL && strcmp(current->dict[i]->name, split.ss[k].s)){
		i++;
	}
	if(i == 100){
		printf("Exiting removeNode: i == 100\n");
		return -1;
	}

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

        }else if(current->dict[0]->dict[0] != NULL){
		printf("printing current->name:   %s\n", current->name);
		printf("printing node->name:  %s\n", current->dict[0]->name);
		//files or directories in dir you're trying to remove
		printf("Exiting removeNode: current->dict[0] != NULL\n");

		return -1;

	} else {
		free(current->dict[0]); //changed i to 0
		current->dict[i] = NULL;
	}
	numberOfNodes--;

	if(i<127 && current->dict[i+1] != NULL){
		int l;
		for(l = i+1; l<128 && current->dict[l] !=NULL; l++){}
		current->dict[i] = current->dict[l-1];
		current->dict[l-1] = NULL;
	}
	printf("Exiting removeNode\n");
	//Remeber to free memory for createnode, findnode and this
	return 0;
}

struct treeNode* findNode(const char* path){

	printf("Entering findNode\n");

	printf("path = %s\n", path);
	string s = {.s = path, .length = strlen(path)};

	stringArray split = pathSplit(s);

	if(split.ss == NULL){

		printf("Exiting findNode split == NULL\n");

		return root;
	}

	printf("after pathsplit\n");

	struct treeNode* node = root;

	int j;

	for(int i = 0; i < split.length; i++){

		for(j = 0; j < 100 && node->dict[j] != NULL && strcmp(node->dict[j]->name, split.ss[i].s); j++){}

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
	printf("Segment = %d, Block = %d\n", segment, block);
	void* local = malloc(BLOCK_SIZE);
	printf("1\n");
	int offset = (segment * SEGMENT_SIZE * BLOCK_SIZE) + (block * BLOCK_SIZE);
	printf("2\n");
	if(fseek(disk, offset, SEEK_SET)){
		printf("fseek failed, offset = %d\n",offset);
	}
	printf("3\n");
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
        while(node->dict[i] != -1){
		numberOfNodes++;
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
	struct int_Node* node = (struct int_Node*) seekNfind(currentSeg-1, SEGMENT_SIZE-1); //check if written == 512
	numberOfNodes = 1;

	root = (struct treeNode*) malloc(sizeof(struct treeNode));
	printf("Roots name = %s\n",node->name);
        strcpy(root->name, node->name);
	root->isFile = node->isFile;
	root->inode.size = node->inode.size;
	root->inode.st_atim = node->inode.st_atim;
	root->inode.st_mtim = node->inode.st_mtim;
	root->inode.plist = NULL;

	int i = 0;
	printf("OKAY BOIS, dict[0] = %d\n", node->dict[i]);
	while(node->dict[i] != -1){
		numberOfNodes++;
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
	printf("---------------------------\n int_node size = %d\n int_Inode size = %d\n union Block size = %d\n-------------------------\n", sizeof(struct int_Node), sizeof(struct int_Inode), sizeof(union block));
	printf("Entering init\n");

	//Open MasterInfo file
	if(!(masterInfo = fopen("MasterInfo","r+b"))){
		if(!(masterInfo = fopen("MasterInfo","w+b"))){
			printf("Exiting init: master = fopen returns error\n");

			return -1;
		}
		printf("New MasterInfo\n");
		currentSeg = 948;
		cleanerSeg = 0;
		int array[] = {currentSeg, cleanerSeg};
		fwrite(array,sizeof(int),2,masterInfo);
	}else{
		printf("Read MasterInfo\n");
		int segInfo[2];
		fread(segInfo, sizeof(int),2,masterInfo);
		currentSeg = segInfo[0];
		cleanerSeg = segInfo[1];
		printf("MasterInfo: currentSeg = %d, cleanerSeg = %d\n",segInfo[0],segInfo[1]);
	}

	segment = malloc(sizeof(union block)*SEGMENT_SIZE);


	//Open filesystem file
	if(!(disk = fopen("FileSystemFile","r+b"))){
		if(!(disk = fopen("FileSystemFile","w+b"))){
			printf("Exiting init: disk = fopen returns error\n");

			return -1;
		}
		numberOfNodes = 1;
		root = (struct treeNode*) malloc(sizeof(struct treeNode));

		//Create "/" dicrectory
	        strcpy(root->name, "/");
		root->isFile = 0;
		root->inode.size=0;
		root->inode.st_atim = time(NULL);
		root->inode.st_mtim = time(NULL);
		root->inode.coordinate = -1;
		root->inode.plist = NULL;

		for(int i = 0; i < 100; i++){
			root->dict[i] = NULL;
		}
		currentBlock = SEGMENT_SIZE - numberOfNodes;
		segmentCtrl();

		//root->dict = NULL; //Linkedlist?
	}else{
		fseek(disk, 0, SEEK_SET);
		//Read the file and setup system
		restoreStructure();
	}
	fseek(disk, currentSeg*SEGMENT_SIZE*BLOCK_SIZE, SEEK_SET);
	printf("Number of Nodes: %d\n", numberOfNodes);
	printf("Exiting init\n");

	return 0;
}
