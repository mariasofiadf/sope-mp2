#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#define NTHREADS 10

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


void create_public_fifo(){
    char myfifo[64];
    sprintf(myfifo,"/tmp/%s", fifoname);
    mkfifo(myfifo, 0666);
}

void *task_request(void *a) {
    int* i = malloc(sizeof(int));
    i = (int*)a;
	printf("In thread PID: %d ; TID: %lu. ; Request: %d\n", getpid(), (unsigned long) pthread_self(), *i);

	pthread_exit(a);	// no termination code
}

int main(int argc, char**argv){

    if(load_args(argc,argv))
        return 1;

    srand(time(NULL));   // Initialization of random function, should only be called once.
    
    create_public_fifo();


    int i;	// thread counter
	pthread_t ids[NTHREADS];	// storage of (system) Thread Identifiers

	printf("\nMain thread PID: %d ; TID: %lu.\n", getpid(), (unsigned long) pthread_self());

    int* r;
	// new threads creation
	for(i=0; i<NTHREADS; i++) {
        r = malloc(sizeof(int));
        *r = rand()%9 + 1;
		if (pthread_create(&ids[i], NULL, task_request, r) != 0)
			exit(-1);	// here, we decided to end process
	}
	// wait for finishing of created threads

	printf("\n");
	pthread_exit(NULL);	// here, not really necessary...
    return 0;

}