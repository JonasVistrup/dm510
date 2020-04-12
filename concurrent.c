#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
int devOpen(int index, int read, int write){
	if(index){
		if(read && write){
			return open("/dev/dm510-1", O_RDWR);
		}else if(read && !write){
			return open("/dev/dm510-1", O_RDONLY);
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
			return open("/dev/dm510-0", O_RDONLY);
		}else if(!read && write){
			return open("/dev/dm510-0", O_WRONLY);
		}else{
			printf("Error: neither read or write was selected\n");
			return -1;
		}
	}
}



int main(){
	int fd;
	int id = 0;

	if (fork() == 0)
        	id += 4;

	if (fork() == 0)
        	id += 2;

	if (fork() == 0)
        	id += 1;

	switch(id){
		case 0: fd = devOpen(0,1,0);
			printf("dn0, read, processor num: %d \n" ,id);
			break;

		case 1: fd = devOpen(0,0,1);
			printf("dn0, write, processor num: %d \n" ,id);
			break;

		case 2: fd = devOpen(0,1,1);
			printf("dn0, read and write, processor num %d \n" ,id);
			break;

		case 3: fd = devOpen(0,0,0);
			printf("dn0, nothing, processor num %d \n" ,id);
			break;

		case 4: fd = devOpen(1,0,1);
			printf("dn1, write, processor num %d \n" ,id);
			break;

		case 5: fd = devOpen(1,1,0);
			printf("dn1, read, processor num %d \n" ,id);
			break;

		case 6: fd = devOpen(1,1,1);
			printf("dn1, read and wrtie, processor num %d \n" ,id);
			break;

		case 7: fd = devOpen(1,0,0);
			printf("dn1, nothing, processor num %d \n" ,id);
			break;

	}
	return 0;
}
