#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char** pathSplit(const char* path){
	printf("Entering pathsplit\n");

	if(!strcmp(path, "/")){
	printf("Exiting pathsplit: path != \"/\"\n");
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

	printf("printing i (size of array): %d\n", i);
	pathcopy = path + 1;

	char* (*split)[i];
	split = malloc(sizeof(split));

	printf("printing PATHCOPY: %s\n", pathcopy);
	printf("printing length of split array: %ld\n", sizeof(*split) / sizeof((*split)[0]));

	i = 0;

	char current[60];

	memset(current, 0, sizeof(current));

	int j = 0;


	for(char c = *pathcopy; c != '\0'; pathcopy ++){
		c = *pathcopy;
		if(c == '/'){
			(*split)[i] = current;
			printf("string printed: %s\n", *split[i]);
			memset(current, 0, sizeof(current));
			j = 0;
			i++;
		}else{
			current[j] = c;
			j++;
		}
		printf("%c\n",c);

	}

	printf("printing i (number of entries - 1) :  %d\n", i);
	(*split)[i] = current; //works for now

	char** test = *split;

	printf("printing out *split[0]: %s\n", test[0]);
        printf("printing out *split[1]: %s\n", test[1]);
        printf("printing out *split[2]: %s\n", test[2]);

	printf("Exting pathSpilt\n");

	return *split;
}


int main( int argc, char *argv[] ) {
	printf("Entering main\n");
	//char path[27] = "/mikkel/jonas/thomas/magnus";
	char path[13] = "/mikkel/jonas";

	printf("PATH = %s\n", path);


	char** split = pathSplit(path);

	if(split == NULL){
		printf("Exiting createNode split == NULL\n");
		return -1;
	}

	printf("printing length of split: %ld\n", sizeof(*split) / sizeof((*split)[0]));

	printf("Exiting main\n");
	return 0;
}
