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
	pid_t pid = getpid() % 100000;
	char str[5];
	sprintf(str, "%d", pid);
	printf("%li\n", syscall(__NR_dm510_msgbox_put, str, strlen(str)));
	wait(NULL);
	char* buffer = malloc(sizeof(char)*5);
	syscall(__NR_dm510_msgbox_get, buffer, 5);
	printf("%s\n",buffer);
	free(buffer);
	return 0;
}
