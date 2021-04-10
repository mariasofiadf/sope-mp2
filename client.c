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

#define MILLION 1000000

/**
 * @brief dynamically allocated
 * 
 */
char * public_fifo;


/**
 * @brief dynamically allocated based on expected number of threads
 * 
 */
char ** priv_fifos;

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

/**
 * @brief Structure to be using to communicate via FIFOs
 * 
 */
struct message {
    /** @brief request id */
	int rid;	    
    /** @brief process id */
	pid_t pid;      
    /** @brief thread id */
	pthread_t tid;	
    /** @brief task load */
	int tskload;	
    /** @brief task result */
	int tskres;	    
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
 * @see priv_fifos
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
}

/**
 * @brief Get the response from the private fifo at index i of priv_fifos
 * 
 * @param i 
 * @see priv_fifos
 * @return int Returns 0 if successful, 1 othewise
 */
int get_response(int i){

    //waits for fifo to be opened on the other end
    int fd2;
    while ((fd2 = open(priv_fifos[i],O_RDONLY))< 0);

    //message struct is created and filled with the information received
    struct message msg;
    int r = read(fd2, &msg, sizeof(msg));
    if(r == 0){
        fprintf(stderr, "Error: EOF\n");
        close(fd2);
        return 1;
    }

    //this end of the fifo is closed
    close(fd2);

    register_op(i,msg.tskload,msg.tskres,GOTRS);

    return 0;
}

/**
 * @brief Thread function
 * 
 * @param a 
 */
void *producer_thread(void *a) {
    //waits to enter
    sem_wait(&sem);
    
    //gets unniversal request number
    int id = task_count++;

    //generates random task weight
    int r = rand()%9 + 1;


    setup_priv_fifo(id);

	send_request(id,r);

    while(get_response(id)){
        usleep(50);
    };

    delete_priv_fifo(id);
    
    //wakes up next thread
    sem_post(&sem);

	pthread_exit(a);
}

/**
 * @brief Checks if time is up
 * 
 * @return int Returns 1 if time is up, 0 otherwise
 */
int time_is_up(){
    sem_wait(&sem);
    time_t curr_time = time(NULL);
    sem_post(&sem);
    return (curr_time >= time_end);
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

    int i = 0;	// thread counter
	pthread_t * ids;	// storage of (system) Thread Identifiers
    ids = (pthread_t*)malloc(nsecs*MILLION*sizeof(pthread_t));
    priv_fifos = (char**)malloc(nsecs*MILLION*sizeof(char[30]));


	// new threads creation
    while(!time_is_up()){
        if(server_is_open()){
            //ignoring possible errors in thread creation
            pthread_create(&ids[i], NULL, producer_thread, NULL);
            i++;
            usleep(rand()%50+50);
        }
        else{
            register_op(0,0,-1,CLOSD);
            usleep(MILLION/2);
        }
	}
    
	// wait for finishing of created threads
    void *__thread_return;
	for(int j=0; j < i ; j++) {
		pthread_join(ids[j], &__thread_return);	// Note: threads give no termination code
		//printf("\nTermination of thread %d: %lu.\nTermination value: %d", i, (unsigned long)ids[i], *retVal);
	}
    
    register_op(0,0,-1, GAVUP);
	pthread_exit(NULL);	// here, not really necessary...
    return 0;

}