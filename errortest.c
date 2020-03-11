#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"
#include <stdlib.h>

int main(int argc, char ** arv){
	printf("Negative length: %li\n",4294967295-syscall(__NR_dm510_msgbox_put, "Hello world", -3)+1);
	printf("Null pointer: %li\n",4294967295-syscall(__NR_dm510_msgbox_put, NULL, 3)+1);
	char* buffer = malloc(sizeof(char)*10);
	printf("Getting from an empty stack: %li\n",4294967295-syscall(__NR_dm510_msgbox_get, buffer, 10)+1);
	syscall(__NR_dm510_msgbox_put, "Hello World!", strlen("Hello World!"));
	printf("Giving a too small buffer: %li\n", 4294967295-syscall(__NR_dm510_msgbox_get, buffer, 10)+1);
	printf("Giving a too small buffer, but lying about its size: %li\n", 4294967295-syscall(__NR_dm510_msgbox_get, buffer, 32)+1);
}

