#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "dm510_dev.h"
#include <linux/string.h>
#include "dm510_dev.h"
#include <sys/ioctl.h>
#include <linux/ioctl.h>

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
/*
=================
Buffer size test:
=================
*/
 	fd = devOpen(0,0,1);
        printf("\n%s\n" ,"Filling device 1 READ-buffer with 42 bytes of char\nDEVICE 1 READ-BUFFER IS NOW FULL!\n ");
        //Trying to write 42 charachters to fill up READ-buffer on device 1:
	write(fd, "012345678901234567890123456789012345678901", strlen("012345678901234567890123456789012345678901"));
        close(fd);

	if (fork() == 0)
        	id += 4;
	if (fork() == 0)
        	id += 2;
	if (fork() == 0)
        	id += 1;

	char* readBuffer = malloc(sizeof(char)*52);

	switch(id){
		case 0: fd = devOpen(0,0,1);
			//Trying to write:
			printf("device 0\t write\t\t processor num: %d\t Attempting to write 10 bytes to full buffer\n" ,id);
    			write(fd, "0123456789", strlen("0123456789"));
			close(fd);
			printf("device 0\t\t\t processor num: %d\t SUCCESFUL WRITE! \n" ,id);
			break;

		case 1: fd = devOpen(1,1,0);
			//Trying to change device 1 input-buffer after 1 second:
			sleep(1);
			printf("device 1\t ioctl\t\t processor num: %d\t Attempting to change READ-buffersize to 52\n" ,id);
			ioctl(fd,IOC_B_READ_SIZE, 52);
			close(fd);
			printf("device 1\t\t\t processor num: %d\t READ BUFFERSIZE CHANGED to 52! \n" ,id);
			break;

		case 2: fd = devOpen(1,1,1);
			//Trying to read&write after 2 second:
			sleep(2);
			printf("device 1\t read&write\t processor num: %d\t Attempting to read 52 bytes and afterwards write 42 bytes + 10 extra bytes\n" ,id);
			read(fd, readBuffer, 52);
                        printf("device 1\t\t\t processor num: %d\t SUCCESFUL READ! \n" ,id);
			write(fd, "012345678901234567890123456789012345678901", strlen("012345678901234567890123456789012345678901"));
                   	write(fd, "0123456789", strlen("0123456789"));
			close(fd);
                        printf("device 1\t\t\t processor num: %d\t SUCCESFUL WRITE! \n" ,id);
			break;

		case 3: fd = devOpen(1,1,0);
                        sleep(4);
			//Trying to change device 1 output-buffer after 3 seconds:
   	         	printf("device 1\t ioctl\t\t processor num: %d\t Attempting to change WRITE-buffersize to 52\n" ,id);
			ioctl(fd,IOC_B_WRITE_SIZE, 52);
			close(fd);
			printf("device 1\t\t\t processor num: %d\t WRITE BUFFERSIZE CHANGED to 52! \n" ,id);
			break;

	}

	//All processor with id != 0 is terminated, so that processor 0 is the only processor left:
	if(!id==0){
	   free(readBuffer);
	   return 0;
	}

	//Waiting just to make sure all process has terminated before flushing device input-buffer and freeing readBuffer.
	sleep(5);
        fd = devOpen(0,1,0);
        //Trying to read 52 bytes to flush out the buffer:
	printf("\n%s\n\n","Emptying device 0 READ-buffer\nDEVICE 0 READ-BUFFER IS NOW EMPTY!");
        read(fd, readBuffer, 52);
	free(readBuffer);
	close(fd);
	return 0;
}
