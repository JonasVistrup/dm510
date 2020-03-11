#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"
#include <stdlib.h>

int main(int argc, char ** arv){

	printf("Testning GET metode\n");
	char* buffer = malloc(sizeof(char)*32);
	printf("%li\n", syscall(__NR_dm510_msgbox_get, buffer, 32));

	printf("%s\n", buffer);
	free(buffer);
	return 0;
}
