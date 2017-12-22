#define _SVID_SOURCE
#define _BSD_SOURCE
#define _XOPEN_SOURCE 500
#define _XOPEN_SORUCE 600
#define _XOPEN_SORUCE 600

#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define BUFF_SIZE 20
#define BUFF_SHM "/OS_BUFF"
#define BUFF_MUTEX_A "/OS_MUTEX_A"
#define BUFF_MUTEX_B "/OS_MUTEX_B"

//declaring semaphores names for local usage
sem_t *mutexA;
sem_t *mutexB;

//declaring the shared memory and base address
int shm_fd;
void *base;

//structure for indivdual table
struct table
{
    int num;
    char name[10];
};

void initTables(struct table *base)
{
    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);
    //initialise the tables with table numbers
    struct table *ptr = base;
    for(int i=0;i<10;i++){
        ptr[i].num = 100+i;
        ptr[i].name[0] = '\0';
    }
    for(int i=0;i<10;i++){
        ptr[i+10].num = 200+i;
        ptr[i+10].name[0] = '\0';
    }
    //perform a random sleep  
    sleep(rand() % 10);

    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);
    return;
}

void printTableInfo(struct table *base)
{
    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);
    //print the tables with table numbers and name
    for(int i=0;i<BUFF_SIZE;i++){
        if (base[i].name[0] == '\0'){
            printf("Table: %i reserved under: \n",base[i].num);
        }
        else{
            printf("Table: %i reserved under: %s\n",base[i].num,base[i].name);
        }
    }
    //perform a random sleep  
    sleep(rand() % 10);
    
    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);
    return; 
}

void reserveSpecificTable(struct table *base, char *nameHld, char *section, int tableNo)
{
    switch (section[0])
    {
    case 'A':
        //capture mutex for section A
        sem_wait(mutexA);
        //check if table number belongs to section specified
        //if not: print Invalid table number 
        if(tableNo<100 || tableNo>109){
            printf("Invalid table number.");
            sem_post(mutexA);
            break;
        }
        //reserve table for the name specified
        //if cant reserve (already reserved by someone) : print "Cannot reserve table"
        if(base[tableNo-100].name[0] == '\0'){
            strcpy(base[tableNo-100].name,nameHld);
        }
        else{
            printf("Cannot reserve table.");
        }
       // release mutex
       sem_post(mutexA);
        break;
    case 'B':
        //capture mutex for section B
        sem_wait(mutexB);
        //check if table number belongs to section specified
        //if not: print Invalid table number 
        if(tableNo<200 || tableNo>210){
            printf("Invalid table number.");
            sem_post(mutexB);
            break;
        }
        //reserve table for the name specified ie copy name to that struct
        //if cant reserve (already reserved by someone) : print "Cannot reserve table"
        if(base[tableNo-200+10].name[0] == '\0'){
            strcpy(base[tableNo-200+10].name,nameHld);
        }
        else{
            printf("Cannot reserve table.");
        }
       // release mutex
       sem_post(mutexB);
       break;
    }
    return;
}

void reserveSomeTable(struct table *base, char *nameHld, char *section)
{
    int idx = -1;
    int i;
    switch (section[0])
    {
    case 'A':
        //capture mutex for section A
        sem_wait(mutexA);
        //look for empty table and reserve it ie copy name to that struct
        int found = 0;
        for(i=0; i<10; i++){
             if(base[i].name[0] == '\0'){
                 strcpy(base[i].name,nameHld);
                 found = 1;
                 break;
             }
        }
        //if no empty table print : Cannot find empty table
        if(!found){
            printf("Cannot find empty table");
        }
        //release mutex for section A
        sem_post(mutexA);
        break;
    case 'B':
        //capture mutex for section A
        sem_wait(mutexB);
        //look for empty table and reserve it ie copy name to that struct
        found = 0;
        for(i=10; i<20; i++){
             if(base[i].name[0] == '\0'){
                 strcpy(base[i].name,nameHld);
                 found = 1;
                 break;
             }
        }
        //if no empty table print : Cannot find empty table
        if(!found){
            printf("Cannot find empty table");
        }
        //release mutex for section A
        sem_post(mutexB);
        break;
    }
}

int processCmd(char *cmd, struct table *base)
{
    char *token;
    char *nameHld;
    char *section;
    char *tableChar;
    int tableNo;
    token = strtok(cmd, " ");
    switch (token[0])
    {
    case 'r':
        nameHld = strtok(NULL, " ");
        section = strtok(NULL, " ");
        tableChar = strtok(NULL, " ");
        if (tableChar != NULL)
        {
            tableNo = atoi(tableChar);
            reserveSpecificTable(base, nameHld, section, tableNo);
        }
        else
        {
            if(section[0]!='A' && section[0]!='B'){
                printf("Invalid command.");
                return 1;
            }
            reserveSomeTable(base, nameHld, section);
        }
        sleep(rand() % 10);
        break;
    case 's':
        printTableInfo(base);
        break;
    case 'i':
        initTables(base);
        break;
    case 'e':
        return 0;
    }
    return 1;
}

int main(int argc, char * argv[])
{
    int fdstdin;
    // file name specifed then rewire fd 0 to file 
    if(argc>1)
    {
        //store actual stdin before rewiring using dup in fdstdin
        fdstdin = dup(0);
        //perform stdin rewiring as done in assign 1
        if(freopen(argv[1],"r",stdin) == NULL){
            printf("Invalid file name.\n");
            dup2(fdstdin,0);
            close(fdstdin);
            return 1;
        }
       
    }
    //open mutex BUFF_MUTEX_A and BUFF_MUTEX_B with inital value 1 using sem_open
    mutexA = sem_open(BUFF_MUTEX_A,O_CREAT|O_RDWR,0777,1);
    mutexB = sem_open(BUFF_MUTEX_B,O_CREAT|O_RDWR,0777,1);

    //opening the shared memory buffer ie BUFF_SHM using shm open
    shm_fd = shm_open(BUFF_SHM,O_CREAT|O_RDWR,0777);
    if (shm_fd == -1)
    {
        printf("prod: Shared memory failed: %s\n", strerror(errno));
        exit(1);
    }

    //configuring the size of the shared memory to sizeof(struct table) * BUFF_SIZE using ftruncate
    ftruncate(shm_fd,sizeof(struct table)* BUFF_SIZE);

    //map this shared memory to kernel space
    base = mmap(NULL,sizeof(struct table)* BUFF_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);

    if (base == MAP_FAILED)
    {
        printf("prod: Map failed: %s\n", strerror(errno));
        // close and shm_unlink?
        shm_unlink(BUFF_SHM);
        exit(1);
    }

    //intialising random number generator
    time_t now;
    srand((unsigned int)(time(&now)));

    //array in which the user command is held
    char cmd[100];
    int cmdType;
    int ret = 1;
    while (ret)
    {
        printf("\n>>");
        fgets(cmd,100,stdin);
        if(argc>1)
        {
            printf("Executing Command : %s\n",cmd);
        }
        ret = processCmd(cmd, base);
    }
    //close the semphores
    sem_close(mutexA);
    sem_close(mutexB);

    //reset the standard input
    if(argc>1)
    {
        dup2(fdstdin,0);
        close(fdstdin);
    }

    //unmap the shared memory
    munmap(base,sizeof(struct table)*BUFF_SIZE);
    close(shm_fd);
    return 0;
}