#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include "dm510_dev.h"

//Index 0|1, for dm510_0|dm510_1. read & write are booleans. write+read>=1. Returns filedescripter.
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

//#define IOC_MAGIC 'k'
//#define IOC_B_READ_SIZE _IOW(IOC_MAGIC, 0, int)
//#define IOC_B_WRITE_SIZE _IOW(IOC_MAGIC, 1, int)
//#define IOC_NUM_OF_READERS _IOW(IOC_MAGIC, 2, int)


int main()
{
    printf("%s\n","");

    int size = 5;

    int device_0 = devOpen(0,1,0);
    int device_1 = devOpen(1,0,1);

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

    int rc;

    rc = ioctl(device_0,IOC_B_WRITE_SIZE, size);


    //Trying to write:
    int writenbytes = write(device_1, "Test", strlen("Test"));
    printf("The number of bytes written by device_1:  %d\n\n", writenbytes);
    char* buffer = malloc(sizeof(char)*32);

//    rc = ioctl(device_1,IOC_B_WRITE_SIZE, size);

    printf("Return value from ioctl:  %d\n\n", rc);

    //Trying to read:
    int readbytes = read(device_0, buffer, 4);
    printf("The number of bytes read to device_0:  %d\n", readbytes);
    //Printing out buffer:
    printf("Content of buffer read from:  %s\n\n", buffer);

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



