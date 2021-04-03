#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#define NTHREADS 1

int nsecs;
char* fifoname;
char myfifo[20];


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
    sprintf(myfifo,"/tmp/%s", fifoname);
    mkfifo(myfifo, 0666);
}
void send_request(int i, int t){
    int fd = open(myfifo, O_WRONLY);
    //i t pid tid res
    char str[100]; sprintf(str, "%d %d %d %ld %d\n", i, t, getpid(), pthread_self(), -1);
    printf("%s", str);
    //write(fd, "Hello", sizeof("Hello"));
    close(fd);
}


void *task_request(void *a) {
    int* i = malloc(sizeof(int));
    *i = *(int*)a;
    int* r = malloc(sizeof(int));
    *r = rand()%9 + 1;
	printf("In thread PID: %d ; TID: %lu. ; Request: %d\n", getpid(), (unsigned long) pthread_self(), *i);
    //send_request(*i,*r);
	pthread_exit(a);	// no termination code
}

int main(int argc, char**argv){

    if(load_args(argc,argv))
        return 1;

    srand(time(NULL));   // Initialization of random function, should only be called once.
    
    create_public_fifo();
    send_request(0, 1);

    /*
    int i;	// thread counter
	pthread_t ids[NTHREADS];	// storage of (system) Thread Identifiers

	printf("\nMain thread PID: %d ; TID: %lu.\n", getpid(), (unsigned long) pthread_self());

	// new threads creation
	for(i=0; i<NTHREADS; i++) {
		if (pthread_create(&ids[i], NULL, task_request, &i) != 0)
			exit(-1);	// here, we decided to end process
	}
	// wait for finishing of created threads
    void *__thread_return; int *retVal;
	for(i=0; i<NTHREADS; i++) {
		pthread_join(ids[i], &__thread_return);	// Note: threads give no termination code
		retVal =  __thread_return;
		printf("\nTermination of thread %d: %lu.\nTermination value: %d", i, (unsigned long)ids[i], *retVal);
	}

	pthread_exit(NULL);	// here, not really necessary...
    */
    return 0;

}