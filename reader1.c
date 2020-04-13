#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

//Index 0|1, for dm510_0|dm510_1. read & write are booleans. write+read>=1. Returns filedescripter.
int devOpen(int index, int read, int write){
	if(index){
		if(read && write){
			return open("/dev/dm510-1", O_RDWR);
		}else if(read && !write){
			return open("/dev/dm510-1", O_RDONLY | O_NONBLOCK);
		}else if(!read && write){
			return open("/dev/dm510-1", O_WRONLY);
		}else{
			printf("Error: neither read or write was selected");
			return -1;
		}
	}else{
		if(read && write){
			return open("/dev/dm510-0", O_RDWR);
		}else if(read && !write){
			return open("/dev/dm510-0", O_RDONLY | O_NONBLOCK);
		}else if(!read && write){
			return open("/dev/dm510-0", O_WRONLY);
		}else{
			printf("Error: neither read or write was selected\n");
			return -1;
		}
	}
}

int main(void){
	char* minibuf = malloc(sizeof(char));
	int fd = devOpen(1,1,0);
	//printf("open done");
	fflush(stdin);
	int retval = read(fd,minibuf,1);
	while(retval==1){
		printf("%s",minibuf);
		fflush(stdin);
		retval = read(fd,minibuf,1);
	}
	printf("\n");
	close(fd);
	free(minibuf);
        return 0;
}
