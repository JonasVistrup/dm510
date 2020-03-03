#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"
#include<sys/wait.h>

int main(){
	for(int i=0; i<4; i++){
		fork();
	}
	int i = rand() % 100;
	char str[3];
	sprintf(str, "%d", i);
	printf("%li\n", syscall(__NR_dm510_msgbox_put, str, strlen(str)));

	char* buffer = malloc(sizeof(char)*4);
	syscall(__NR_dm510_msgbox_get, buffer, 3);
	printf("%s\n",buffer);
	wait(NULL);
	return 0;
}
