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

#define MILLION 2000000

char * public_fifo;
//priv_fifos array will be dynamically allocated based on expected number of threads
char ** priv_fifos;

time_t time_end;
int nsecs;

int task_count = 0;

sem_t sem, sem_time;

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
    //sem_wait(&sem_req);

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
    //sem_post(&sem_req);

}


int get_response(int i){

    //sem_wait(&sem_req);

    //waits for fifo to be opened on the other end
    int fd2;

    while ((fd2 = open(priv_fifos[i],O_RDONLY))< 0);

    //message struct is created and filled with the information received
    struct message msg;
    int r = read(fd2, &msg, sizeof(msg));
    if(r == 0)
    {
        fprintf(stderr, "FIFO empty!\n");
        close(fd2);
        return 1;
    }
    //this end of the fifo is closed
    close(fd2);

    register_op(i,msg.tskload,msg.tskres,GOTRS);
    //sem_post(&sem_req);
    return 0;
}


void *task_request(void *a) {
    sem_wait(&sem);
    int id = task_count;
    task_count++;

    int r = rand()%9 + 1;

    setup_priv_fifo(id);

	send_request(id,r);

    while(get_response(id)){
        usleep(50);
    };

    delete_priv_fifo(id);
    
    sem_post(&sem);
	pthread_exit(a);
}

int time_is_up(){
    sem_wait(&sem);
    time_t curr_time = time(NULL);
    //printf("curr_time: %ld\ttime_end: %ld\n", curr_time, time_end);
    sem_post(&sem);
    return (curr_time >= time_end);
}

int main(int argc, char**argv){

    sem_init(&sem,0,1);
    sem_init(&sem_time,0,1);

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
            int pt = pthread_create(&ids[i], NULL, task_request, NULL);
            i++;
            usleep(rand()%50+50);
        }
        else{
            register_op(0,0,-1,CLOSD);
            sleep(1);
        }
	}

	// wait for finishing of created threads
    void *__thread_return;
	for(int j=0; j < i ; j++) {
		pthread_join(ids[j], &__thread_return);	// Note: threads give no termination code
		//printf("\nTermination of thread %d: %lu.\nTermination value: %d", i, (unsigned long)ids[i], *retVal);
	}
    
	pthread_exit(NULL);	// here, not really necessary...
    return 0;

}