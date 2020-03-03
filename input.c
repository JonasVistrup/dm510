#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"

int main(int argc, char ** argv){

        int i;
        for(i=1; i<argc; i++){
                //printf("%s", argv[i]);
                syscall(__NR_dm510_msgbox_put, argv[i], strlen(argv[i]));
        }
        return 0;
}
