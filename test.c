#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

stringArray pathSplit(string path){
	printf("In pathsplit\n");
	if(!strcmp(path.s, "/")){
		stringArray retVal = {.ss = NULL, .length = 0};
		return retVal;
	}
	printf("after root check\n");
	int counter = 0;
	for(int i = 0; i < path.length; i++){
		if(path.s[i] == '/'){
			counter++;
		}
	}
	printf("After Counter\n");
	stringArray retVal;
	retVal.ss = malloc(sizeof(string)*counter);
	retVal.length = counter;
	printf("Counter = %d\n", counter);
	string* next = malloc(sizeof(string));
	next->s = malloc(sizeof(char)*60);
	int nextPos = 0;
	int ncounter = 0;
	printf("right before for-loop\n");
	for(int i = 1; i<path.length; i++){
		if(path.s[i] == '/'){
			printf("In if /\n");
			next->length = nextPos;
			retVal.ss[ncounter] = *next;
			ncounter++;
			nextPos=0;
			next = malloc(sizeof(string));
			next->s = malloc(sizeof(char)*60);
			printf("out of if /\n");
		}else{
			printf("in else\n");
			next->s[nextPos] = path.s[i];
			nextPos++;
			printf("out of else\n");
		}
	}
	retVal.ss[ncounter] = *next;
	return retVal;

}



int main( int argc, char *argv[] ) {
	printf("Entering main\n");
	//char path[27] = "/mikkel/jonas/thomas/magnus";
	char* path = "/mikkel/jonas/hello/ho/jo/bo/so";
	string s = { .s = path, .length = strlen(path)};
	printf("What?\n");
	stringArray tokens = pathSplit(s);
	for(int i=0; i<tokens.length; i++){
		printf("Token %d: %s\n",i,tokens.ss[i].s);
	}
	printf("Exiting main\n");
	return 0;
}
