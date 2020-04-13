#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "dm510_dev.h"
#include <linux/string.h>

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
	int i;
/*
=====================
Firs concurrent test:
=====================
*/
        fd = devOpen(0,0,1);
        printf("\n%s\n" ,"Filling device 1 input-buffer with 42 bytes of char\nDEVICE 1 INPUT-BUFFER IS NOW FULL!\n ");
        //Trying to write 42 charachters to fill up buffer on device 1:
	write(fd, "012345678901234567890123456789012345678901", strlen("012345678901234567890123456789012345678901"));
        close(fd);

	if (fork() == 0)
        	id += 4;
	if (fork() == 0)
        	id += 2;
	if (fork() == 0)
        	id += 1;
	/*
	x "device 1 input-buffer" gets filled with 42 bytes charachters.
	x One process tries to write 10 bytes from "device 0" to "device 1", otherwise wait.
	x One process tries to write 10 bytes from "device 0" to "device 1", otherwise wait.
	x One process waits 1 seconds before reading 6 bytes from "device 1".
	x One process waits 1 second before reading 8 bytes from "device 1".
	x One process waits 1 second before reading 6 bytes from "device 1".
	*/
	char* readBuffer = malloc(sizeof(char)*42);

	switch(id){
		case 0: fd = devOpen(0,0,1);
			//Trying to write:
			printf("device 0\t write\t processor num: %d\t Attempting to write 10 bytes\n" ,id);
    			write(fd, "0123456789", strlen("0123456789"));
			close(fd);
			printf("device 0\t\t processor num: %d\t SUCCESFUL WRITE! \n" ,id);
			break;

		case 1: fd = devOpen(0,0,1);
			//Trying to write:
			printf("device 0\t write\t processor num: %d\t Attempting to write 10 bytes\n" ,id);
                        write(fd, "9876543210", strlen("9876543210"));
			close(fd);
			printf("device 0\t\t processor num: %d\t SUCCESFUL WRITE! \n" ,id);
			break;

		case 2: fd = devOpen(1,1,0);
			sleep(1);
			//Trying to read after 1 second:
   	         	printf("device 1\t read\t processor num: %d\t Attempting to read 6 bytes \n" ,id);
			read(fd, readBuffer, 6);
			close(fd);
			printf("device 1\t\t processor num: %d\t SUCCESFUL READ! \n" ,id);
			break;

		case 3: fd = devOpen(1,1,0);
                        sleep(1);
			//Trying to read after 1 second:
   	         	printf("device 1\t read\t processor num: %d\t Attempting to read 6 bytes \n" ,id);
                        read(fd, readBuffer, 6);
			close(fd);
			printf("device 1\t\t processor num: %d\t SUCCESFUL READ! \n" ,id);
			break;

		case 4: fd = devOpen(1,1,0);
			sleep(1);
                        //Trying to read after 1 second:
   	         	printf("device 1\t read\t processor num: %d\t Attempting to read 8 bytes \n" ,id);
                        read(fd, readBuffer, 8);
			close(fd);
			printf("device 1\t\t processor num: %d\t SUCCESFUL READ! \n" ,id);
			break;
	}
	//All processor with id != 0 is terminated, so that processor 0 is the only processor left:
	if(!id==0){
	   free(readBuffer);
	   return 0;
	}
	//Waiting just to make sure all process has terminated before flushing device input-buffer and freeing readBuffer.
	sleep(2);
        fd = devOpen(1,1,0);
        //Trying to read 42 bytes to flush out the buffer:
	printf("\n%s\n\n","Emptying device 1 input-buffer\nDEVICE 1 INPUT-BUFFER IS NOW EMPTY!");
        read(fd, readBuffer, 42);
	free(readBuffer);
	close(fd);

/*
=======================
Second concurrent test:
=======================
*/
	if (fork() == 0)
        	id += 4;
	if (fork() == 0)
        	id += 2;
	if (fork() == 0)
        	id += 1;
	/*
	x "device 1 input-buffer" is now empty.
	x One process tries to read 10 bytes from "device 1", if empty, wait.
	x One process tries to read 10 bytes from "device 1", if empty, wait.
	x One process waits 1 second before writing 6 bytes to "device 1 input-buffer".
	x One process waits 1 second before writing 8 bytes to "device 1 input buffer".
	x One process waits 1 second before writing 6 bytes to "device 1 input buffer".
	*/
	readBuffer = malloc(sizeof(char)*42);

	switch(id){
		case 0: fd = devOpen(1,1,0);
                        //Trying to read:
			printf("device 1\t read\t processor num: %d\t Attempting to read 10 bytes \n" ,id);
                        read(fd, readBuffer, 10);
                        close(fd);
			printf("device 1\t\t processor num: %d\t SUCCESFUL READ! \n" ,id);
			break;

		case 1: fd = devOpen(1,1,0);
			//Trying to read:
                        printf("device 1\t read\t processor num: %d\t Attempting to read 10 bytes \n" ,id);
                        read(fd, readBuffer, 10);
                        close(fd);
                        printf("device 1\t\t processor num: %d\t SUCCESFUL READ! \n" ,id);
			break;

		case 2: fd = devOpen(0,0,1);
			sleep(1);
			//Trying to write after 1 second:
                        printf("device 0\t write\t processor num: %d\t Attempting to write 6 bytes\n" ,id);
                        write(fd, "123456", strlen("123456"));
                        close(fd);
                        printf("device 0\t\t processor num: %d\t SUCCESFUL WRITE! \n" ,id);
                        break;

		case 3: fd = devOpen(0,0,1);
                        sleep(1);
			//Trying to write after 1 second:
                        printf("device 0\t write\t processor num: %d\t Attempting to write 6 bytes\n" ,id);
                        write(fd, "123456", strlen("123456"));
                        close(fd);
                        printf("device 0\t\t processor num: %d\t SUCCESFUL WRITE! \n" ,id);
                        break;

		case 4: fd = devOpen(0,0,1);
			sleep(1);
			//Trying to write after 2 second:
                        printf("device 0\t write\t processor num: %d\t Attempting to write 8 bytes\n" ,id);
                        write(fd, "12345678", strlen("12345678"));
                        close(fd);
                        printf("device 0\t\t processor num: %d\t SUCCESFUL WRITE! \n" ,id);
                        break;
	}
	//All processor with id != 0 is terminated, so that processor 0 is the only processor left:
	if(!id==0){
	   free(readBuffer);
	   return 0;
	}
	sleep(2);
	free(readBuffer);
	printf("%s\n","");
	return 0;
}
