#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <string.h>


char myfifo[20];
char* fifoname;

void setup_public_fifo(){
    sprintf(myfifo,"%s", fifoname);
    mkfifo(myfifo, 0666);
}


int main(int argc, char**argv){

    fifoname = argv[1];
    srand(time(NULL));   // Initialization of random function, should only be called once.

    //create_public_fifo();
    setup_public_fifo();
    int fd = open(fifoname, O_RDONLY);
    char str[256];
    read(fd, str, sizeof(str));
    printf("myfifo: %s", str);
    close(fd);
    return 0;

}