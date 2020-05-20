#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BLOCK_SIZE 512		//number of bytes in a block
#define SEGMENT_SIZE 2048	//number of blocks in a segment
#define TOTAL_SIZE 1000		//number of segments in storage


//list of pointers to data
struct plist{
	char* p[128];
};


//Filesystem version of a file's or directory's Inode
struct int_Inode{
        size_t size;
        time_t st_atim;
        time_t st_mtim;
        int plist;
};

//Filesystem version of a file or directory
struct int_Node{	//size = 512 bytes
	int isFile;
        char name[76];
        struct int_Inode inode;
        int dict[100];
};

//Main memory version of a file's or directory's Inode
struct Inode{
	size_t size;
	time_t st_atim;
	time_t st_mtim;
	int coordinate;
	struct plist* plist;
};

//Main memory version of a file or directory, directories takes a tree structure
struct treeNode{
	int isFile;
	char name[76];
	struct Inode inode;
	struct treeNode* dict[100];
};

//Union of the type of data stored in a segments
union block{
	struct int_Node node;	//Node
	int plist[128];		//Indirect Pointers
	char data[BLOCK_SIZE];	//Pure data, in the forms of char
};


union block (*segment)[SEGMENT_SIZE];	//Current segment

static FILE* disk;			//File for storing the log based filesystem
static FILE* masterInfo;		//File for storing the info needed to maintain a log based filesystem

int currentBlock;	//Next block to be written to in segment
int cleanerSeg;		//First uncleaned segment
int currentSeg;		//Next segment to written to disk
int numberOfNodes;	//Number of files and directories

struct treeNode* root;	//Root directory



struct string{		//String struct
	char* s;
	int length;
};
typedef struct string string;

struct stringArray{	//String array struct
	string* ss;
	int length;
};
typedef struct stringArray stringArray;

typedef struct ll ll;	//Linkedlist containing allocated pointers
struct ll{
	ll* next;	//Next linkedlist
	void* d;	//allocated pointer
};

ll* head;		//Head of linkedlist


int segmentCtrl();	//Only function need to be declared, all other follows a hierarchi


//Recursivly frees the nodes in the treestructure
void freeTree(struct treeNode* current){
	if(current != NULL){
		if(current->isFile){
			for(int i=0; i<128; i++){
				if(current->inode.plist->p[i] !=NULL){
					free(current->inode.plist->p[i]);
				}
			}
			free(current->inode.plist);
			free(current);
		}else{
			for(int i=0; i<100; i++){
				freeTree(current->dict[i]);
			}
			free(current);
		}
	}
}


//Recursivly frees the allocated pointers in the LinkedList, and frees the LinkedList
void llClean(ll* current){
	if(current != NULL){
		llClean(current->next);
		free(current->d);
		free(current);
	}
}

//Inserts a pointer into the linkedlist
void llInsert(void* data){
	ll* current = head;
	if(current == NULL){
		head = malloc(sizeof(ll));
		head->next = NULL;
		head->d = data;
		return;
	}
	while(current->next != NULL){
		current = current->next;
	}
	current->next = malloc(sizeof(ll));
	current->next->next = NULL;
	current->next->d = data;
	return;
}

//Splits a path into tokens using the delimter '/' and the string struct
stringArray pathSplit(string path){
	printf("In pathsplit\n");
	printf("String path = %s\nString length = %d\n",path.s,path.length);

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
	llInsert(retVal.ss);
	retVal.length = counter;

	string* next = malloc(sizeof(string));
	llInsert(next);
	next->s = calloc(76, sizeof(char));
	llInsert(next->s);
	int nextPos = 0;
	int ncounter = 0;

	for(int i = 1; i<path.length; i++){
		if(path.s[i] == '/'){
			next->length = nextPos;
			retVal.ss[ncounter] = *next;
			ncounter++;
			nextPos=0;
			next = malloc(sizeof(string));
			llInsert(next);
			next->s = calloc(76, sizeof(char));
			llInsert(next->s);
		}else{
			next->s[nextPos] = path.s[i];
			nextPos++;
		}
	}
	next->length = nextPos;
	retVal.ss[ncounter] = *next;
	printf("Exiting pathSplit\n");
	return retVal;

}

//Cleaner function, cleans all segments by finding all active info in the root tree and rewriting it to disk
void cleaner(struct treeNode* tNode){
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


//Recursivly inserts the nodes into the current segment, in such a way that the root is the last block inserted
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
	(*segment)[currentBlock].node = *node;
	free(node);
	currentBlock++;
	segmentCtrl();
	printf("Exiting reverseTree\n");

	return currentSeg * 65536 + currentBlock -1;
}


int segmentCtrl(){
	printf("Entering segmentCTRL\n");
	if((currentBlock + numberOfNodes) == SEGMENT_SIZE){
		reverseTree(root);
		int written = fwrite(segment, BLOCK_SIZE, SEGMENT_SIZE, disk);
		if(written != SEGMENT_SIZE){
			printf("Write fail, written blocks = %d\n", written);
		}
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

//Creates a node, either a file or a directory
int createNode(const char* path, int isFile){
	printf("Entering createNode\n");
	printf("path = %s\n",path);

	string s  ={.s = (char*) path, .length = strlen(path)};
	stringArray split = pathSplit(s);

	if(split.ss == NULL){
		printf("Exiting createNode split == NULL\n");

		return -EPERM;
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
                strcpy(newFile->name, split.ss[k].s);
		newFile->isFile = 1;
		newFile->inode.size = 0;
		newFile->inode.st_atim = time(NULL);
		newFile->inode.st_mtim = time(NULL);
		newFile->inode.plist = (struct plist*) calloc(128, sizeof(char*));
		for(int i = 0; i<128; i++){
			(*segment)[currentBlock].plist[i] = -1;
		}
		newFile->inode.coordinate = currentSeg * 65536 + currentBlock;
		printf("SegInsert plist: Seg: %d, Block: %d\n",currentSeg, currentBlock);
		currentBlock++;
		segmentCtrl();

		for(int i = 0; i < 100; i++){
			newFile->dict[i] = NULL;
		}
		node->dict[j] = newFile;
	}else{

		struct treeNode* newDir = (struct treeNode*) malloc(sizeof(struct treeNode));

                strcpy(newDir->name, split.ss[k].s);
                newDir->isFile = 0;
               	newDir->inode.size = 0;
           	newDir->inode.st_atim = time(NULL);
                newDir->inode.st_mtim = time(NULL);
		newDir->inode.coordinate = -1;
                newDir->inode.plist = NULL;

	        for(int i = 0; i < 100; i++){
	 		newDir->dict[i] = NULL;
		}
		node->dict[j] = newDir;
	}
	numberOfNodes++;

	//free(split.ss);
	//freeStringArray(split);
	printf("Exiting createNode\n");

	return 0;
}

//Removes a node, either a file or directory
int removeNode(const char* path, int isFile){
	printf("Entering removeNode \n");
	printf("%s\n",path);

	struct treeNode* current = root;

	string s = {.s = (char*)path, .length = strlen(path)};
	stringArray split = pathSplit(s);

	if(split.ss == NULL){
		printf("Exiting removeNode split == NULL\n");
		return -EPERM;
	}

	int k;
	int j = 0;

	for(k = 0; k < split.length-1; k++){
		for(j = 0; j < 100 && current->dict[j] != NULL && strcmp(current->dict[j]->name, split.ss[k].s); j++){}

		if(j == 100 || current->dict[j] == NULL){
			printf("Exiting removeNode j== 100 or node->dict[j] == NULL\n");
			return -ENOENT;
		}
		current = current->dict[j];
	}

	int i = 0;
	while(i < 100 && current->dict[i] != NULL && strcmp(current->dict[i]->name, split.ss[k].s)){
		i++;
	}
	if(i == 100){
		printf("Exiting removeNode: i == 100\n");
		return -ENOENT;
	}
	if(current->dict[i] == NULL){
		printf("Exiting removeNode: dict[i] == NULL");
		return -ENOENT;
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

        }else if(current->dict[i]->dict[0] != NULL){
		printf("Error: directory is not empty\n");
		printf("printing current->name:   %s\n", current->name);
		printf("printing node->name:  %s\n", current->dict[0]->name);
		printf("Exiting removeNode: current->dict[0] != NULL\n");

		return -ENOTEMPTY;

	} else {
		free(current->dict[i]);
		current->dict[i] = NULL;
	}
	numberOfNodes--;

	if(i<99 && current->dict[i+1] != NULL){
		int l;
		for(l = i+1; l<100 && current->dict[l] !=NULL; l++){}
		current->dict[i] = current->dict[l-1];
		current->dict[l-1] = NULL;
	}

	//free(split.ss);
	printf("Exiting removeNode\n");
	return 0;
}

struct treeNode* findNode(const char* path){

	printf("Entering findNode\n");

	printf("path = %s\n", path);
	string s = {.s = (char*)path, .length = strlen(path)};

	stringArray split = pathSplit(s);

	if(split.ss == NULL){

		printf("Exiting findNode split == NULL\n");

		return root;
	}
	struct treeNode* node = root;

	int j;

	for(int i = 0; i < split.length; i++){
		printf("Split[%d] = %s\n", i, split.ss[i].s);

		for(j = 0; j < 100 && node->dict[j] != NULL && strcmp(node->dict[j]->name, split.ss[i].s); j++){
			printf("dict[%d] = %s\n", j, node->dict[j]->name);
		}

		if(j == 100){
			printf("Exiting findNode j=100\n");
			return NULL;
		}else if(node->dict[j] == NULL){
			printf("Exiting findNode for i=%d dict[%d] = NULL\n",i, j);
			return NULL;
		}

		node = node->dict[j];
	}

	//free(split.ss);
	//freeStringArray(split); //HAVENT TESTED THIS ONE, BUT ASUMES IT KILLS YOUR DOG OR SOMETHING

	printf("Exiting findNode\n");
	return node;
}

//finds a block, reads it and returns a pointer to it.
void* seekNfind(int segment, int block){
	printf("Entering seekNfind\n");
	printf("seekSegment = %d, seekBlock = %d\n", segment, block);
	void* local = malloc(BLOCK_SIZE);
	int offset = (segment * SEGMENT_SIZE * BLOCK_SIZE) + (block * BLOCK_SIZE);
	if(fseek(disk, offset, SEEK_SET)){
		printf("fseek failed, offset = %d\n",offset);
	}
	int written = fread(local, BLOCK_SIZE, 1, disk);//check if written is 512 bytes
	if(written != 1){
		printf("Write fail, written=%d\n",written);
	}
	printf("Exiting seekNfind\n");

	return local;
}


//Imports a file to main memory from the disk
void restoreFile(struct int_Node* node, struct treeNode* tNode){
	printf("Entering restoreFile\n");
        strcpy(tNode->name, node->name);
	tNode->isFile = node->isFile;
        tNode->inode.size = node->inode.size;
        tNode->inode.st_atim = node->inode.st_atim;
        tNode->inode.st_mtim = node->inode.st_mtim;
	tNode->inode.coordinate = node->inode.plist;
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

//Imports a directory to main memory from a disk
void restoreDir(struct int_Node* node, struct treeNode* tNode){
	printf("Entering restoreDir\n");
        strcpy(tNode->name, node->name);
        tNode->isFile = node->isFile;
        tNode->inode.size = node->inode.size;
        tNode->inode.st_atim = node->inode.st_atim;
        tNode->inode.st_mtim = node->inode.st_mtim;
        tNode->inode.plist = NULL;

	printf("Dir name = %s\n", tNode->name);

	for(int i = 0; i<100 && node->dict[i] != -1; i++){
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
        }
        free(node);
	printf("Exiting restoreDir\n");

}

//Imports root to main memory from disk
int restoreStructure(){
	printf("Entering restoreStructure\n");

	struct int_Node* node;
	if(currentSeg==0){
		node = (struct int_Node*) seekNfind(TOTAL_SIZE-1, SEGMENT_SIZE-1);
	}else{
		node = (struct int_Node*) seekNfind(currentSeg-1, SEGMENT_SIZE-1);
	}
	numberOfNodes = 1;

	root = (struct treeNode*) malloc(sizeof(struct treeNode));
	printf("Roots name = %s\n",node->name);
        strcpy(root->name, node->name);
	root->isFile = node->isFile;
	root->inode.size = node->inode.size;
	root->inode.st_atim = node->inode.st_atim;
	root->inode.st_mtim = node->inode.st_mtim;
	root->inode.plist = NULL;

	for(int i=0; i<100 && node->dict[i] != -1; i++){
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
	}
	free(node);
	printf("Exiting restoreStructure\n");

	return 0;
}

//Initiate filesystem
int init(){
	printf("Entering init\n");

	//Open MasterInfo file
	if(!(masterInfo = fopen("MasterInfo","r+b"))){
		if(!(masterInfo = fopen("MasterInfo","w+b"))){
			printf("Exiting init: master = fopen returns error\n");

			return -EIO;
		}
		currentSeg = 0;
		cleanerSeg = 0;
		int array[] = {currentSeg, cleanerSeg};
		fwrite(array,sizeof(int),2,masterInfo);
	}else{
		int segInfo[2];
		int read = fread(segInfo, sizeof(int),2,masterInfo);
		if(read != 2){
			printf("MasterInfo read failed, read=%d\n", read);
		}
		currentSeg = segInfo[0];
		cleanerSeg = segInfo[1];
		printf("MasterInfo: currentSeg = %d, cleanerSeg = %d\n",segInfo[0],segInfo[1]);
	}

	segment = malloc(sizeof(union block)*SEGMENT_SIZE);
	llInsert(segment);

	//Open filesystem file
	if(!(disk = fopen("FileSystemFile","r+b"))){
		if(!(disk = fopen("FileSystemFile","w+b"))){
			printf("Exiting init: disk = fopen returns error\n");

			return -EIO;
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

	}else{
		fseek(disk, 0, SEEK_SET);
		restoreStructure();
	}
	fseek(disk, currentSeg*SEGMENT_SIZE*BLOCK_SIZE, SEEK_SET);
	printf("Number of Nodes: %d\n", numberOfNodes);
	printf("Exiting init\n");

	return 0;
}
