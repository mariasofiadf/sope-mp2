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
char * priv_fifos[NTHREADS];

sem_t sem_req, sem_resp;

/**
 * @brief Structure to be using to communicate via FIFOs
 * 
 */
struct message {
	int rid;	// request id
	pid_t pid;	// process id
	pthread_t tid;	// thread id
	int tskload;	// task load
	int tskres;	// task result
};

/**
 * @brief Possible operations done by Client
 * 
 */
enum oper{
    IWANT,
    GOTRS,
    CLOSD,
    GAVUP
};

/**
 * @brief Prints program usage
 * 
 */
void print_usage(){
    fprintf(stderr,"Usage: ./c <-t nsecs> fifoname\n");
}

/**
 * @brief Loads needed arguments such as public_fifo and nsecs
 * 
 * @param argc 
 * @param argv 
 * @return int Returns 0, if successful and 1 otherwise
 */
int load_args(int argc, char** argv){
    if(argc != 4 || strcmp(argv[1], "-t")){
        print_usage();
        return 1;
    }

    nsecs = atoi(argv[1]);

    public_fifo = malloc(sizeof(argv[3]));
    public_fifo = argv[3];

    return 0;
}



/**
 * @brief Checks if server is open
 * 
 * @return int Returns 1 if server is open, 0 otherwise
 */
int server_is_open(){
    return(access(public_fifo, O_RDONLY) == 0);
}

/**
 * @brief Outputs operations in the format "inst ; i ; t ; pid ; tid ; res ; oper" to stdout
 * 
 * @param i universal unique request number
 * @param t task weight
 * @param res result
 * @param oper operation @see enum oper
 */
void register_op(int i, int t, int res, enum oper oper){
    printf("%ld ; %d ; %d ; %d ; %ld ; %d", time(NULL), i, t, getpid(), pthread_self(), res);
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

/**
 * @brief Set up private fifo in "/tmp/pid.tid"
 * 
 * @param i universal unique request number
 * 
 * @note this function allocates memory for every new fifo
 */
void setup_priv_fifo(int i){
    priv_fifos[i] = malloc(30);
    sprintf(priv_fifos[i], "/tmp/%d.%ld", getpid(), pthread_self());
    mkfifo(priv_fifos[i], 0666);
}

/**
 * @brief Deletes a private fifo
 * 
 * @param i universal unique request number
 * 
 */
void delete_priv_fifo(int i){
    remove(priv_fifos[i]);
}

/**
 * @brief Sends request to public fifo (server's fifo)
 * 
 * @param i universal unique request number
 * @param t task weight
 * 
 * @note This is a critical zone. Therefore, a semaphore is used to prevent simultaneous access to the public fifo
 * 
 * @returns Returns 0 uppon success, 1 otherwise
 */
void send_request(int i, int t){

    //wait to enter
    sem_wait(&sem_req);

    //waits for fifo to be opened on the other end
    int fd;
    while((fd = open(public_fifo, O_WRONLY)) < 0);

    //message struct is created and filled with info to be sent
    struct message msg;
    msg.rid = i;
    msg.tskload = t;
    msg.pid = getpid();
    msg.tid = pthread_self();
    msg.tskres = -1;

    //message is sent and this end of the fifo is closed
    write(fd, &msg, sizeof(msg));
    close(fd);

    register_op(i, t, -1, IWANT);

    //wake up next thread
    sem_post(&sem_req);

}


int get_response(int i){
    //sem_wait(&sem_resp);

    //waits for fifo to be opened on the other end
    int fd2;
    while ((fd2 = open(priv_fifos[i],O_RDONLY))< 0);

    //message struct is created and filled with the information received
    struct message msg;
    read(fd2, &msg, sizeof(msg));

    //this end of the fifo is closed
    close(fd2);

    register_op(i,msg.tskload,msg.tskres,GOTRS);
    //sem_post(&sem_resp);
    return 1;
}


void *task_request(void *a) {
    int* i = malloc(sizeof(int));   *i = *(int*)a;
    int* r = malloc(sizeof(int));   *r = rand()%9 + 1;

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
        if(server_is_open()){
            if (pthread_create(&ids[i], NULL, task_request, &i) != 0)
                exit(-1);	// here, we decided to end process
            usleep(20);
        }
        else{
            register_op(0,0,-1,CLOSD);
            sleep(1);
        }
	}
	// wait for finishing of created threads
    /*void *__thread_return;
	for(i=0; i<NTHREADS; i++) {
		pthread_join(ids[i], &__thread_return);	// Note: threads give no termination code
		//printf("\nTermination of thread %d: %lu.\nTermination value: %d", i, (unsigned long)ids[i], *retVal);
	}
    */

	pthread_exit(NULL);	// here, not really necessary...
    return 0;

}