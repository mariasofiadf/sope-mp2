#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <string.h>
#include <semaphore.h>
#define NTHREADS 10

int nsecs;
char * public_fifo;
char * priv_fifo;

sem_t sem_req, sem_resp;

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
    public_fifo = malloc(sizeof(argv[3]));
    public_fifo = argv[3];
    return 0;
}

void free_vars(){
    //free(fifoname);
}


void setup_priv_fifo(){
    priv_fifo = malloc(30);
    sprintf(priv_fifo, "/tmp/%d.%ld", getpid(), pthread_self());
    mkfifo(priv_fifo, 0666);
}

void send_request(int i, int t){

    sem_wait(&sem_req);
    int fd;
    int debug = open("debug", O_WRONLY | O_APPEND);
    while((fd = open(public_fifo, O_WRONLY)) < 0);

    //i t pid tid res
    char str[30];
    sprintf(str, "%d %d %d %ld %d\n", i, t, getpid(), pthread_self(), -1);
    printf("Request sent: %s", str);
    write(fd, str, strlen(str));
    close(fd);
    write(debug, str, strlen(str));
    close(debug);

    sem_post(&sem_req);
}


int get_response(){
    sem_wait(&sem_resp);
    int fd2;
    printf("opening\n");
    while ((fd2 = open(priv_fifo,O_RDONLY))< 0);
    //i t pid tid res
    printf("getting response\n");
    char str[100];
    read(fd2, str, sizeof(str));
    printf("Response: %s\n", str);
    close(fd2);
    sem_post(&sem_resp);
    return 1;
}


void *task_request(void *a) {
    int* i = malloc(sizeof(int));
    *i = *(int*)a;
    int* r = malloc(sizeof(int));
    *r = rand()%9 + 1;
    setup_priv_fifo();
	printf("In thread PID: %d ; TID: %lu ; Request: %d\n", getpid(), (unsigned long) pthread_self(), *i);
    send_request(*i,*r);
    get_response();
	pthread_exit(a);	// no termination code
}

int main(int argc, char**argv){

    sem_init(&sem_req,0,1);
    sem_init(&sem_resp,0,1);

    if(load_args(argc,argv))
        return 1;

    srand(time(NULL));   // Initialization of random function, should only be called once.

    setup_priv_fifo();



    int i;	// thread counter
	pthread_t ids[NTHREADS];	// storage of (system) Thread Identifiers

	printf("\nMain thread PID: %d ; TID: %lu.\n", getpid(), (unsigned long) pthread_self());

	// new threads creation
	for(i=0; i<NTHREADS; i++) {
		if (pthread_create(&ids[i], NULL, task_request, &i) != 0)
			exit(-1);	// here, we decided to end process
        usleep(20);
	}
	// wait for finishing of created threads
    void *__thread_return; int *retVal;
	for(i=0; i<NTHREADS; i++) {
		pthread_join(ids[i], &__thread_return);	// Note: threads give no termination code
		retVal =  __thread_return;
		printf("\nTermination of thread %d: %lu.\nTermination value: %d", i, (unsigned long)ids[i], *retVal);
	}

	pthread_exit(NULL);	// here, not really necessary...
    return 0;

}