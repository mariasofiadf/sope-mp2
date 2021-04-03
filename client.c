#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <string.h>
#define NTHREADS 10

int nsecs;
char* fifoname;
char myfifo[20];
char priv_fifo[100];


int load_args(int argc, char** argv){
    if(argc != 4){
        fprintf(stderr,"Wrong number of arguments!");
        return 1;
    }
    if(strcmp(argv[1], "-t")){
        fprintf(stderr, "Wrong argument");
        return 1;
    }
    nsecs = atoi(argv[1]);
    fifoname = malloc(sizeof(argv[3]));
    fifoname = argv[3];
    return 0;
}

void free_vars(){
    //free(fifoname);
}

void setup_public_fifo(){
    sprintf(myfifo,"%s", fifoname);
    //mkfifo(myfifo, 0666);
}

void setup_priv_fifo(){
    sprintf(priv_fifo, "/tmp/%d.%ld", getpid(), pthread_self());
    mkfifo(priv_fifo, 0666);
}

void send_request(int i, int t){
    int fd = open(fifoname, O_WRONLY);

    int debug = open("debug", O_WRONLY);
    if(fd == -1)
    {
        fprintf(stderr, "Error opening public fifo!");
        return;
    }
    //i t pid tid res
    char str[30];
    sprintf(str, "%d %d %d %ld %d\n", i, t, getpid(), pthread_self(), -1);
    printf("Request sent: %s\n", str);
    write(fd, str, strlen(str));
    close(fd);
    write(debug, str, strlen(str));
    close(debug);
}


int get_response(){
    int fd2 = open(priv_fifo, O_RDONLY);
    if(fd2 == -1)
    {
        fprintf(stderr, "Error opening private fifo!");
        return 1;
    }
    //i t pid tid res
    printf("getting response\n");
    char str[100];
    read(fd2, str, sizeof(str));
    printf("Response: %s\n", str);
    close(fd2);

    printf("got response\n");
    return 1;
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

    //create_public_fifo();
    setup_public_fifo();
    setup_priv_fifo();
    send_request(0, 1);
    //usleep(100);
    int response = get_response();

    /*
    int i;	// thread counter
	pthread_t ids[NTHREADS];	// storage of (system) Thread Identifiers

	printf("\nMain thread PID: %d ; TID: %lu.\n", getpid(), (unsigned long) pthread_self());

	// new threads creation
	for(i=0; i<NTHREADS; i++) {
		if (pthread_create(&ids[i], NULL, task_request, &i) != 0)
			exit(-1);	// here, we decided to end process
        usleep(10);
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