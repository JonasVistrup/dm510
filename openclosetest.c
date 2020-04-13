#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
int devOpenNoBlock(int index, int read, int write){
	if(index){
		if(read && write){
			return open("/dev/dm510-1", O_RDWR | O_NONBLOCK);
		}else if(read && !write){
			return open("/dev/dm510-1", O_RDONLY | O_NONBLOCK);
		}else if(!read && write){
			return open("/dev/dm510-1", O_WRONLY | O_NONBLOCK);
		}else{
			printf("Error: neither read or write was selected\n");
			return -1;
		}
	}else{
		if(read && write){
			return open("/dev/dm510-0", O_RDWR | O_NONBLOCK);
		}else if(read && !write){
			return open("/dev/dm510-0", O_RDONLY | O_NONBLOCK);
		}else if(!read && write){
			return open("/dev/dm510-0", O_WRONLY | O_NONBLOCK);
		}else{
			printf("Error: neither read or write was selected\n");
			return -1;
		}
	}
}

int devOpen(int index, int read, int write){
	if(index){
		if(read && write){
			return open("/dev/dm510-1", O_RDWR);
		}else if(read && !write){
			return open("/dev/dm510-1", O_RDONLY);
		}else if(!read && write){
			return open("/dev/dm510-1", O_WRONLY);
		}else{
			printf("Error: neither read or write was selected\n");
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
	int retval;
	int id = 0;
	printf("\nTesting sequential open and close:\n-----------------------------------------------\n");
	printf("\nDev0:\n");
	fd = devOpen(0,1,0);
	if(fd<0){
		printf("\tRead-only Failed\n");
	}else{
		printf("\tRead-only Opened\n");
	}
	if(fd>=0){
		retval = close(fd);
		if(retval){
			printf("\tRead-only failure on close\n\n");
		}else{
			printf("\tRead-only Closed\n\n");
		}
	}

	fd = devOpen(0,0,1);
	if(fd<0){
		printf("\tWrite-only Failed\n");
	}else{
		printf("\tWrite-only Opened\n");
	}
	if(fd>=0){
		retval = close(fd);
		if(retval){
			printf("\tWrite-only failure on close\n\n");
		}else{
			printf("\tWrite-only Closed\n\n");
		}
	}


	fd = devOpen(0,1,1);
	if(fd<0){
		printf("\tRead-Write Failed\n");
	}else{
		printf("\tRead-Write Opened\n");
	}
	if(fd>=0){
		retval = close(fd);
		if(retval){
			printf("\tRead-Write failure on close\n\n");
		}else{
			printf("\tRead-Write Closed\n\n");
		}
	}



	printf("\nDev1:\n");
	fd = devOpen(1,1,0);
	if(fd<0){
		printf("\tRead-only Failed\n");
	}else{
		printf("\tRead-only Opened\n");
	}
	if(fd>=0){
		retval = close(fd);
		if(retval){
			printf("\tRead-only failure on close\n\n");
		}else{
			printf("\tRead-only Closed\n\n");
		}
	}

	fd = devOpen(1,0,1);
	if(fd<0){
		printf("\tWrite-only Failed\n");
	}else{
		printf("\tWrite-only Opened\n");
	}
	if(fd>=0){
		retval = close(fd);
		if(retval){
			printf("\tWrite-only failure on close\n\n");
		}else{
			printf("\tWrite-only Closed\n\n");
		}
	}


	fd = devOpen(1,1,1);
	if(fd<0){
		printf("\tRead-Write Failed\n");
	}else{
		printf("\tRead-Write Opened\n");
	}
	if(fd>=0){
		retval = close(fd);
		if(retval){
			printf("\tRead-Write failure on close\n\n");
		}else{
			printf("\tRead-Write Closed\n\n");
		}
	}


	printf("\nTesting concurrent open and close with Read-only:\n-----------------------------------------------\n");
	if (fork() == 0)
        	id += 4;

	if (fork() == 0)
        	id += 2;

	if (fork() == 0)
        	id += 1;

	if(id<4){
		fd = devOpen(1,1,0);
		if(fd>0){
			printf("\tOpen Read from dev 1, id=%d\n",id);
			sleep(1);
			retval = close(fd);
			if(retval){
				printf("\tClose Read from dev 1 FAILED, id=%d\n",id);
			}else{
				printf("\tClose Read from dev 1, id=%d\n",id);
			}
		}else{
			printf("\tOpen Read from dev 1 FAILED, id=%d\n",id);
		}
	}else{
		fd = devOpen(0,1,0);
		if(fd>0){
			printf("\tOpen Read from dev 0, id=%d\n",id);
			sleep(1);
			retval = close(fd);
			if(retval){
				printf("\tClose Read from dev 0 FAILED, id=%d\n",id);
			}else{
				printf("\tClose Read from dev 0, id=%d\n",id);
			}
		}else{
			printf("\tOpen Read from dev 0 FAILED, id=%d\n",id);
		}
	}

	if(id==0){
		sleep(1);
		printf("\nTesting concurrent open and close with Write-only:\n-----------------------------------------------\n");
		sleep(1);
	}else{
		sleep(2);
	}


	if(id<4){
		fd = devOpen(1,0,1);
		if(fd>0){
			printf("\tOpen Write from dev 1, id=%d\n",id);
			sleep(1);
			retval = close(fd);
			if(retval){
				printf("\tClose Write from dev 1 FAILED, id=%d\n",id);
			}else{
				printf("\tClose Write from dev 1, id=%d\n",id);
			}
		}else{
			printf("\tOpen Write from dev 1 FAILED, id=%d\n",id);
		}
	}else{
		fd = devOpen(0,0,1);
		if(fd>0){
			printf("\tOpen Write from dev 0, id=%d\n",id);
			sleep(1);
			retval = close(fd);
			if(retval){
				printf("\tClose Write from dev 0 FAILED, id=%d\n",id);
			}else{
				printf("\tClose Write from dev 0, id=%d\n",id);
			}
		}else{
			printf("\tOpen Write from dev 0 FAILED, id=%d\n",id);
		}
	}

/*
	switch(id){
		case 0: fd = devOpen(1,0,1);
			printf("Open write-only from dev 1, id=%d\n",id);
			sleep(1);
			printf("",id);
			break;

		case 1: printf("Open write-only from dev 0, id=%d\n",id);
			break;

		case 2: printf("Opening Writer from dev 1, id=%d\n",id);
			break;

		case 3: sleep(1);
			break;

		case 4: printf("Opening Writer from dev 1, id=%d\n",id);
			break;

		case 5: 
			break;

		case 6: 
			break;

		case 7: 
			break;

	}
*/	sleep(3);
	wait(NULL);
	return 0;
}
