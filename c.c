#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int nsecs;
char* fifoname;


int load_args(int argc, char** argv){
    if(argc != 3){
        fprintf(stderr,"Wrong number of arguments!");
        return 1;
    }
    nsecs = atoi(argv[1]);
    fifoname = malloc(sizeof(argv[2]));
    fifoname = argv[2];
    return 0;
}

void free_vars(){
    //free(fifoname);
}

int main(int argc, char**argv)
{
    if(load_args(argc,argv))
        return 1;
    return 0;

}