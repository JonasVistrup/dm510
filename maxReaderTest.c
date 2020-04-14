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

	printf("\n");

	// Open the writer device
    	int device_1 = open("/dev/dm510-1", O_WRONLY);

    	if(device_1 < 0){
		printf("Fail Open, Error: %d\n",device_1);
    	}else{
    		printf("Succesfull open device_1\n");
    	}

	//Writning to buffer
	int writenbytes = write(device_1, "Hello World", strlen("Hello Woorld"));
    	printf("The number of bytes written by device_1:  %d\n", writenbytes);
	

	// Close the writter 
	int deviceID1 = close(device_1);
	if(deviceID1 < 0){
		printf("Fail close, Error: %d\n", deviceID1);
	}else{
		printf("Succesfull close device_write\n");
	}

	//Open  1 read device - to change Max Readers
        int device_read = open("/dev/dm510-0", O_RDONLY);

        if(device_read < 0){
                printf("Fail Open, Error: %d\n",device_read);
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


	//Close the reader device
	int deviceIDReader = close(device_read);
	if(deviceIDReader < 0){
		printf("Fail close, Error %d\n", deviceIDReader);
	}else{
		printf("Succesfull close device_reader\n");
	}

	printf("\n");

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
                printf("Processor %d 		Succesfull open device_0\n", id);
        }

	char* buffer = malloc(sizeof(char)*3);

	//Trying to read:
    	int readbytes = read(device_0, buffer, 3);
	printf("Processor %d", id);
    	printf("		The number of bytes read to device_0:  %d\n", readbytes);

	free(buffer);

	 // Puts the processor to sleep for 3 seconds
        sleep(2);

    	//Trying to close:
    	int closeID0 = close(device_0);

    	if(closeID0){
        	printf("Fail Close, Error: %d\n",closeID0);
    	}else{
      	printf("Processor %d		Succesfull close device_0\n", id);
    	}

	wait(NULL);

    	printf("%s\n","");

    	return 0;

}

