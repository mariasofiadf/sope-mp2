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
#define NTHREADS 1

int nsecs;
char * public_fifo;
char * priv_fifos[NTHREADS];

sem_t sem_req, sem_resp;

struct message {
	int rid;	// request id
	pid_t pid;	// process id
	pthread_t tid;	// thread id
	int tskload;	// task load
	int tskres;	// task result
};

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

enum oper{
    IWANT,
    GOTRS,
    CLOSD,
    GAVUP
};

void register_op(int i, int t, enum oper oper){
    printf("%ld ; %d ; %d ; %d ; %ld ; %d", time(NULL), i, t, getpid(), pthread_self(), -1);
    switch (oper)
    {
    case IWANT:
        printf(" ; IWANT \n");
        break;
    case GOTRS:
        printf(" ; GOTRS \n");
        break;
    case CLOSD:
        printf(" ; CLOSD \n");
        break;
    case GAVUP:
        printf(" ; GAVUP \n");
        break;
    default:
        break;
    }
}


void setup_priv_fifo(int i){
    priv_fifos[i] = malloc(30);
    sprintf(priv_fifos[i], "/tmp/%d.%ld", getpid(), pthread_self());
    mkfifo(priv_fifos[i], 0666);
}


void delete_priv_fifo(int i){
    remove(priv_fifos[i]);
}

void send_request(int i, int t){
    register_op(i, t, IWANT);
    sem_wait(&sem_req);
    int fd;
    int debug = open("debug", O_WRONLY | O_APPEND);
    while((fd = open(public_fifo, O_WRONLY)) < 0);

    struct message msg;
    msg.rid = i;
    msg.tskload = t;
    msg.pid = getpid();
    msg.tid = pthread_self();
    msg.tskres = -1;
    write(fd, &msg, sizeof(msg));
    close(fd);
    write(debug, &msg, sizeof(msg));
    close(debug);

    sem_post(&sem_req);
}


int get_response(int i){
    //sem_wait(&sem_resp);
    int fd2;
    while ((fd2 = open(priv_fifos[i],O_RDONLY))< 0);
    //i t pid tid res
    return 0;
    struct message msg;
    read(fd2, &msg, sizeof(msg));
    close(fd2);
    //sem_post(&sem_resp);
    return 1;
}


void *task_request(void *a) {
    int* i = malloc(sizeof(int));
    *i = *(int*)a;
    int* r = malloc(sizeof(int));
    *r = rand()%9 + 1;
    setup_priv_fifo(*i);
	send_request(*i,*r);
    get_response(*i);
    delete_priv_fifo(*i);
    usleep(30);
	pthread_exit(a);
}

int main(int argc, char**argv){

    sem_init(&sem_req,0,1);
    sem_init(&sem_resp,0,1);

    if(load_args(argc,argv))
        return 1;

    srand(time(NULL));   // Initialization of random function, should only be called once.

    int i;	// thread counter
	pthread_t ids[NTHREADS];	// storage of (system) Thread Identifiers

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
		//printf("\nTermination of thread %d: %lu.\nTermination value: %d", i, (unsigned long)ids[i], *retVal);
	}

	pthread_exit(NULL);	// here, not really necessary...
    return 0;

}