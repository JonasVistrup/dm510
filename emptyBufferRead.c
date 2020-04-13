#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include "dm510_dev.h"
#include<sys/wait.h>

/*
Writes 11 bytes to the buffer, and tells the to read 12.. 
Therefor the processor will wait for more data
*/
int main(){

	// Open the devices
	int rv;
        int maxReaders;

	int device_0 = open("/dev/dm510-0", O_RDONLY);
    	int device_1 = open("/dev/dm510-1", O_WRONLY);

    	if(device_0 < 0){
		printf("Fail Open, Error: %d\n",device_0);
		return 1;
    	}else{
    		printf("Succesfull open device_0\n");
    	}

    	if(device_1 < 0){
		printf("Fail Open, Error: %d\n",device_1);
    	}else{
    		printf("Succesfull open device_1\n");
    	}

    	printf("%s\n","");

	//Writning to buffer
	int writenbytes = write(device_1, "Hello World", strlen("Hello World"));
    	printf("The number of bytes written by device_1:  %d\n", writenbytes);

	//Creating 8 processors

	int readers;
	int id;

	if(fork() == 0){
		id +=4;
	}
	if(fork() == 0){
		id +=2;
	}
	if(fork() == 0){
		id +=1;
	}


	char* buffer = malloc(sizeof(char)*32);

	//Trying to read:
    	int readbytes = read(device_0, buffer, 6);
	printf("Processor id = %d\n", id);
    	printf("The number of bytes read to device_0:  %d\n", readbytes);
    	//Printing out buffer:
    	printf("Content of buffer read from:  %s\n\n", buffer);

	wait(NULL);
	free(buffer);

    	//Trying to close:
    	int closeID0 = close(device_0);
    	int closeID1 = close(device_1);

    	if(closeID0){
        	printf("Fail Close, Error: %d\n",closeID0);
    	}else{
      	printf("Succesfull Close device_0\n");
    	}

    	if(closeID1){
        	printf("Fail Close, Error: %d\n",closeID1);
    	}else{
     	printf("Succesfull Close device_1\n");
    	}

    	printf("%s\n","");

    	return 0;

}

