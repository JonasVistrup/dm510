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
			return open("/dev/dm510-1", O_RDONLY);
		}else if(!read && write){
			return open("/dev/dm510-1", O_WRONLY | O_NONBLOCK);
		}else{
			printf("Error: neither read or write was selected");
			return -1;
		}
	}else{
		if(read && write){
			return open("/dev/dm510-0", O_RDWR);
		}else if(read && !write){
			return open("/dev/dm510-0", O_RDONLY);
		}else if(!read && write){
			return open("/dev/dm510-0", O_WRONLY | O_NONBLOCK);
		}else{
			printf("Error: neither read or write was selected\n");
			return -1;
		}
	}
}

int main(int argc, char** argv){
	int retval;
	int fd = devOpen(1,0,1);
	for(int i=1; i<argc; i++){
		retval = write(fd, argv[i], strlen(argv[i]));
		printf("Number of bytes written: %d\n", retval);
	}
	close(fd);
        return 0;
}
