#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>

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

#define IOC_B_READ_SIZE         _IOW('k', 0, int)
#define IOC_B_WRITE_SIZE        _IOW('k', 1, int)
#define IOC_NUM_OF_READERS      _IOW('k', 2, int)


int main()
{

    int device_0 = devOpen(0,1,0);
    int device_1 = devOpen(1,0,1);

    if(device_0 < 0){
	printf("Fail Open, Error: %d\n",device_0);
	return 1;
    }else{
    printf("Succesfull open\n");
    }

    if(device_1 < 0){
	printf("Fail Open, Error: %d\n",device_1);
    }else{
    printf("Succesfull open\n");
    }





    //ioctl(device_1,IOC_B_WRITE_SIZE, 3);


    //Trying to write:
    int writenbytes = write(device_1, "Test", strlen("Test"));
    printf("The number of bytes written is: %d\n", writenbytes);
    char* buffer = malloc(sizeof(char)*32);

    ioctl(device_1,IOC_B_WRITE_SIZE, 3);

    //Trying to read:
    int readbytes = read(device_0, buffer, 4);
    printf("The number of bytes read is: %d\n", readbytes);
    //Printing out buffer:
    printf("%s\n", buffer);


    //Trying to close:
    int closeID0 = close(device_0);
    int closeID1 = close(device_1);
    if(closeID0){
        printf("Fail Close, Error: %d\n",closeID0);
    }else{
      printf("Succes Close\n");
    }

    if(closeID1){
        printf("Fail Close, Error: %d\n",closeID1);
    }else{
     printf("Succes Close\n");
    }


    return 0;
}



