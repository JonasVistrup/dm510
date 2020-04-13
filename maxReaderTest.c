#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include "dm510_dev.h"
#include <sys/wait.h>
#include <unistd.h>

int main(){

	// Open the writer device
    	int device_1 = open("/dev/dm510-1", O_WRONLY);

    	if(device_1 < 0){
		printf("Fail Open, Error: %d\n",device_1);
    	}else{
    		printf("Succesfull open device_1\n");
    	}

    	printf("%s\n","");

	//Writning to buffer
	int writenbytes = write(device_1, "Hello Woorld", strlen("Hello Woorld"));
    	printf("The number of bytes written by device_1:  %d\n\n", writenbytes);


	//Open  1 read device - to change Max Readers
        int device_read = open("/dev/dm510-0", O_RDONLY);

        if(device_read < 0){
                printf("Fail Open, Error: %d\n",device_read);
                return 1;
        }else{
                printf("Succesfull open device_read\n");
        }

	//Setting the number of max reads to 2
        int rv;
        int maxReaders;
        printf ("Setting the number of max reader to 2\n");
        maxReaders = 2;
        rv = ioctl(device_read, IOC_NUM_OF_READERS, maxReaders);
        if(rv < 0){
                printf("Ioctl failed");
        }


	//Creating 4 processors

        int readers;
        int id;

        if(fork() == 0){
                id +4;
        }
        if(fork() == 0){
                id +=2;
        }


	//Open read device
	int device_0 = open("/dev/dm510-0", O_RDONLY);
	if(device_0 < 0){
                printf("Fail Open, Error: %d\n",device_0);
                return 1;
        }else{
                printf("Succesfull open device_0\n");
        }

	// Puts the processor to sleep for 2 seconds
	sleep(2);

	char* buffer = malloc(sizeof(char)*32);

	//Trying to read:
    	int readbytes = read(device_0, buffer, 3);
	printf("Processor id = %d\n", id);
    	printf("The number of bytes read to device_0:  %d\n", readbytes);
    	//Printing out buffer:
    	printf("Content of buffer read from:  %s\n\n", buffer);

	free(buffer);

    	//Trying to close:
    	int closeID0 = close(device_0);

    	if(closeID0){
        	printf("Fail Close, Error: %d\n",closeID0);
    	}else{
      	printf("Succesfull Close device_0\n");
    	}

	wait(NULL);

	int closeID1 = close(device_1);


    	if(closeID1){
        	printf("Fail Close, Error: %d\n",closeID1);
    	}else{
     	printf("Succesfull Close device_1\n");
    	}

    	printf("%s\n","");


	int closeIDREAD = close(device_read);

        if(closeIDREAD){
                printf("Fail Close, Error: %d\n",closeID0);
        }else{
        	printf("Succesfull Close device_0\n");
        }



    	return 0;

}

