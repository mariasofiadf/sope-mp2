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
#include "./common.h"

#define MILLION 1000000

/**
 * @brief dynamically allocated
 * 
 */
char * public_fifo;


/**
 * @brief Storage of private fifos' names
 * 
 * @note Dynamically allocated based on expected number of threads
 * 
 */
char ** priv_fifos;

/**
 * @brief Storage of file descriptors for the private fifos. 
 * 
 * @note Dynamically allocated based on expected number of threads
 * 
 */
int * fds;

/**
 * @brief Storage of file descriptor for the public fifo 
 * 
 */
int public_fifo_fd;

/**
 * @brief time at which the main thread will stop creating producing threads
 * 
 * @see time_is_up
 */
time_t time_end;

/**
 * @brief number of seconds that the main thread will create producing threads
 * 
 */
int nsecs;

/**
 * @brief total number of task requests (producing threads)
 * 
 */
int task_count = 0;

/**
 * @brief semaphore used for critacal regions
 * 
 */
sem_t sem;

/*
struct msg { int i; int t; pid_t pid; pthread_t tid; int res; };
*/
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
 * @see nsecs
 * @see time_end
 * @see public_fifo
 * @return int Returns 0, if successful and 1 otherwise
 */
int load_args(int argc, char** argv){
    if(argc != 4 || strcmp(argv[1], "-t")){
        print_usage();
        return 1;
    }

    nsecs = atoi(argv[2]);

    time_end = time(NULL) + (time_t) nsecs;

    public_fifo = malloc(sizeof(argv[3]));
    public_fifo = argv[3];

    return 0;
}



/**
 * @brief Checks if server is open
 * 
 * @return int Returns 1 if server is open, 0 otherwise
 */
int public_fifo_exists(){
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
 * @brief Checks if time is up
 * 
 * @return int Returns 1 if time is up, 0 otherwise
 */
int time_is_up(){
    time_t curr_time = time(NULL);
    return (curr_time >= time_end);
}

/**
 * @brief Set up private fifo in "/tmp/pid.tid"
 * 
 * @param i universal unique request number
 * 
 * @see priv_fifos
 * 
 * @note this function allocates memory for every new fifo
 */
void setup_priv_fifo(int i){
    sem_wait(&sem);
    priv_fifos[i] = malloc(30);
    sprintf(priv_fifos[i], "/tmp/%d.%ld", getpid(), pthread_self());
    mkfifo(priv_fifos[i], 0666);
    sem_post(&sem);
}

/**
 * @brief Deletes a private fifo
 * 
 * @param i universal unique request number
 * 
 */
void delete_priv_fifo(int i){
    sem_wait(&sem);
    remove(priv_fifos[i]);
    sem_post(&sem);
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
    sem_wait(&sem);

    public_fifo_fd = open(public_fifo, O_WRONLY);

    //message struct is created and filled with info to be sent
    Message msg;
    msg.rid = i;
    msg.tskload = t;
    msg.pid = getpid();
    msg.tid = pthread_self();
    msg.tskres = -1;

    //message is sent and this end of the fifo is closed
    write(public_fifo_fd, &msg, sizeof(msg));

    register_op(i, t, -1, IWANT);

    sem_post(&sem);
}

/**
 * @brief Get the response from the private fifo at index i of priv_fifos
 * 
 * @param i 
 * @see priv_fifos
 * @return int Returns 0 if successful, 1 othewise
 */
int get_response(int i, int t){
    sem_wait(&sem);
    
    //waits for fifo to be opened on the other end
    fds[i] = open(priv_fifos[i], O_RDONLY);

    //message struct is created and filled with the information received
    Message msg;
    
    read(fds[i], &msg, sizeof(msg));

    //this end of the fifo is closed
    close(fds[i]);

    if (msg.tskres == -1){
        register_op(msg.rid,msg.tskload,msg.tskres, CLOSD);
    } else {
        register_op(i,msg.tskload,msg.tskres,GOTRS);
    }
    
    sem_post(&sem);

    return 0;
}

/**
 * @brief Thread function
 * 
 * @param a 
 */
void *producer_thread(void *a) {

    //gets unniversal request number
    int i = task_count++;

    //generates random task weight
    int t = rand()%9 + 1;

    setup_priv_fifo(i);

    send_request(i,t);

    if(time_is_up()){
        sem_wait(&sem);
        register_op(i,t,-1, GAVUP);
        sem_post(&sem);
    }else{
        get_response(i, t);
    }   
        
    delete_priv_fifo(i);
    
    //wakes up next thread

	pthread_exit(a);
}


/**
 * @brief Main
 * 
 * @param argc 
 * @param argv 
 * @return int Returns 0 uppon success, non-zero otherwise
 */
int main(int argc, char**argv){

    sem_init(&sem,0,1);

    if(load_args(argc,argv))
        return 1;

    srand(time(NULL));   // Initialization of random function, should only be called once.

	setbuf(stdout,NULL); //For debug purposes

	pthread_t * ids;	// storage of (system) Thread Identifiers

    ids = (pthread_t*)malloc(nsecs*MILLION*sizeof(pthread_t));
    priv_fifos = (char**)malloc(nsecs*MILLION*sizeof(char[30]));
    fds = (int*)malloc(nsecs*MILLION*sizeof(int));

    int over = 0;

	// new threads creation
    while(!over){
        
        //ignoring possible errors in thread creation
        if (public_fifo_exists()){
            pthread_create(&ids[task_count], NULL, producer_thread, NULL);
        }
        usleep(rand()%50+50);
        
        sem_wait(&sem);
        over = time_is_up();
        sem_post(&sem);
    }

    close(public_fifo_fd);

	// wait for finishing of created threads
    void *__thread_return;
	for(int j=0; j < task_count ; j++) {
		pthread_join(ids[j], &__thread_return);	// Note: threads give no termination code
		//printf("\nTermination of thread %d: %lu.\nTermination value: %d", i, (unsigned long)ids[i], *retVal);
	}
    
    return 0;

}