/**
 * Código base (incompleto) para implementação de relógios vetoriais.
 * Meta: implementar a interação entre três processos ilustrada na figura
 * da URL: 
 * 
 * https://people.cs.rutgers.edu/~pxk/417/notes/images/clocks-vector.png
 * 
 * Compilação: mpicc -o rvet rvet.c
 * Execução:   mpiexec -n 3 ./rvet
 */
 
#include <stdio.h>
#include <string.h>  
#include <mpi.h>     
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#define BufferSize 20 // Size of the buffer

pthread_mutex_t mutexClock;


sem_t empty;
sem_t full;
pthread_mutex_t mutex;

sem_t empty1;
sem_t full1;
pthread_mutex_t mutex1;


int in1 = 0;
int out1 = 0;

int in2 = 0;
int out2 = 0;

int in_snap = 0; //bffr snap
int out_snap = 0; //bffr snap

typedef struct 
{
    int pid;
    int destination;
    int p[3];
} Clock;



int pid;

Clock bufferin[BufferSize];
Clock bufferout[BufferSize];
Clock bufferSnap[BufferSize]; //clock ->bufferSnap

Clock clock1;

Clock mark; //relogio para marcador -1


sem_t snapEmpty;
sem_t snapFull;
pthread_mutex_t mutexSnap;


void producer1(Clock c)
{   
    Clock item1=c;
    sem_wait(&empty);
    pthread_mutex_lock(&mutex);
    bufferin[in1] = item1;
    in1 = (in1+1)%BufferSize;
    pthread_mutex_unlock(&mutex);
    sem_post(&full);

}

Clock consumer1()
{   
    sem_wait(&full);
    pthread_mutex_lock(&mutex);
    Clock item2 = bufferin[out1];
    out1 = (out1+1)%BufferSize;
    pthread_mutex_unlock(&mutex);
    sem_post(&empty);
    return item2;
    
}

void producer2(Clock c)
{   
    Clock item3=c;
    sem_wait(&empty1);
    pthread_mutex_lock(&mutex1);
    bufferout[in2] = item3;
    in2 = (in2+1)%BufferSize;
    pthread_mutex_unlock(&mutex1);
    sem_post(&full1);

}
Clock consumer2()
{   
    sem_wait(&full1);
    pthread_mutex_lock(&mutex1);
    Clock item4 = bufferout[out2];
    out2 = (out2+1)%BufferSize;
    pthread_mutex_unlock(&mutex1);
    sem_post(&empty1);
    return item4;
    
}

void comparaClocks(Clock *clockv,int p0, int p1, int p2 );

void Event(int pid, Clock *clockt){
   clockt->p[pid]++;
   //printf("EVENT: %d, Clock: (%d, %d, %d)\n", pid, clock1.p[0], clock1.p[1], clock1.p[2]);
}

void *Emissor (){
   while(1){
   Clock c = consumer1();
   MPI_Send(c.p,3,MPI_INT,c.destination,0,MPI_COMM_WORLD);
    }
}

void *Receive (){
   while(1){
   
   Clock clock;
   int antigo0 = clock1.p[0];
   int antigo1 = clock1.p[1];
   int antigo2 = clock1.p[2];
   int markCount = clock1.p[0] + clock1.p[1] + clock1.p[2];
   MPI_Recv(clock.p, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
   if(markCount ==-3){
        Clock item=clock1;
        sem_wait(&snapEmpty);
        pthread_mutex_lock(&mutexSnap);
        bufferout[in_snap] = item;
        in_snap = (in_snap+1)%BufferSize;
        pthread_mutex_unlock(&mutexSnap);
        sem_post(&snapFull);
        printf("IN - PROCESSO: %d, (%d, %d, %d)\n", pid, clock1.p[0], clock1.p[1], clock1.p[2]);
   }else{
   comparaClocks(&clock,antigo0,antigo1,antigo2);
   producer2(clock);
   }
   }
}

void Snapshot(){
   mark.destination=0; //testa para p0
   producer1(mark);
  
    
}


void comparaClocks(Clock *clockv,int p0, int p1, int p2 )
{
   if(clockv->p[0]<=p0){
      clockv->p[0]=p0;
   }
   if(clockv->p[1]<=p1){
      clockv->p[1]=p1;
   }
   if(clockv->p[2]<=p2){
      clockv->p[2]=p2;
   }
   
}

// Representa o processo de rank 0
void process0(){
   pid =0;
   clock1.p[0]=0;
   clock1.p[1]=0;
   clock1.p[2]=0;
   
   mark.p[0]=(-1);
   mark.p[1]=(-1);
   mark.p[2]=(-1);
   
    pthread_t pro,con;
    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty,0,BufferSize);
    sem_init(&full,0,0);
    
    pthread_mutex_init(&mutex1, NULL);
    sem_init(&empty1,0,BufferSize);
    sem_init(&full1,0,0);
    
    pthread_mutex_init(&mutexSnap, NULL);
    sem_init(&snapEmpty,0,BufferSize);
    sem_init(&snapFull,0,0);
    
    
    pthread_create(&pro, NULL, (void *)Emissor, NULL);
    
    pthread_create(&con, NULL, (void *)Receive, NULL);

   
   printf("INICIO: %d, Clock: (%d, %d, %d)\n", 0, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   Event(pid, &clock1);
   
   printf("EVENT IN - PROCESSO: %d, (%d, %d, %d)\n", pid, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   printf("SEND : %d -> %d, Clock: (%d, %d, %d)\n", pid,clock1.destination=1, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   Event(pid, &clock1);
   clock1.destination=1;
   producer1(clock1);
   
   printf("EVENT + REC: %d, Clock: (%d, %d, %d)\n", 0, clock1.p[0], clock1.p[1], clock1.p[2]);
   
    
   clock1=consumer2();
   Event(pid, &clock1);
   
   printf("EVENT + SEND : %d -> %d, Clock: (%d, %d, %d)\n", pid,clock1.destination=2, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   Event(pid, &clock1);
   clock1.destination=2;
   producer1(clock1);
   
   printf("REC: %d, Clock: (%d, %d, %d)\n", 0, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   clock1=consumer2();
   Event(pid, &clock1);
   
   printf("EVENT +REC : %d -> %d, Clock: (%d, %d, %d)\n", pid,clock1.destination=1, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   
   Event(pid, &clock1);
   clock1.destination=1;
   producer1(clock1);
   
   printf("EVENT: %d, Clock: (%d, %d, %d)\n", 0, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   Event(pid, &clock1);
   
   printf("EVENT: %d, Clock: (%d, %d, %d)\n", 0, clock1.p[0], clock1.p[1], clock1.p[2]);
   
    
    pthread_join(pro, NULL);
    pthread_join(con, NULL);

    pthread_mutex_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    
    pthread_mutex_destroy(&mutex1);
    sem_destroy(&empty1);
    sem_destroy(&full1); 
    
    pthread_mutex_destroy(&mutexSnap);
    sem_destroy(&snapEmpty);
    sem_destroy(&snapFull); 

}
void process1(){
   pid =1;
   clock1.p[0]=0;
   clock1.p[1]=0;
   clock1.p[2]=0;
   
   mark.p[0]=(-1);
   mark.p[1]=(-1);
   mark.p[2]=(-1);
   
    pthread_t pro,con;
    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty,0,BufferSize);
    sem_init(&full,0,0);
    
    pthread_mutex_init(&mutex1, NULL);
    sem_init(&empty1,0,BufferSize);
    sem_init(&full1,0,0);
    
    pthread_mutex_init(&mutexSnap, NULL);
    sem_init(&snapEmpty,0,BufferSize);
    sem_init(&snapFull,0,0);
        
    
    pthread_create(&pro, NULL, (void *)Emissor, NULL);
    
    pthread_create(&con, NULL, (void *)Receive, NULL);
   
   printf("Process: %d, Clock: (%d, %d, %d)\n", 1, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   Event(pid, &clock1);
   clock1.destination=0;
   producer1(clock1);
   
   printf("Process: %d, Clock: (%d, %d, %d)\n", 1, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   clock1=consumer2();
   Event(pid, &clock1);

   printf("Process: %d, Clock: (%d, %d, %d)\n", 1, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   clock1=consumer2();
   Event(pid, &clock1);

   printf("Process: %d, Clock: (%d, %d, %d)\n", 1, clock1.p[0], clock1.p[1], clock1.p[2]);
   
    pthread_join(pro, NULL);
    pthread_join(con, NULL);

    pthread_mutex_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    
    pthread_mutex_destroy(&mutex1);
    sem_destroy(&empty1);
    sem_destroy(&full1); 
    
    pthread_mutex_destroy(&mutexSnap);
    sem_destroy(&snapEmpty);
    sem_destroy(&snapFull); 
}

// Representa o processo de rank 2
void process2(){
   pid = 2;
   clock1.p[0]=0;
   clock1.p[1]=0;
   clock1.p[2]=0;
   
   mark.p[0]=(-1);
   mark.p[1]=(-1);
   mark.p[2]=(-1);
   
   pthread_t pro,con;
    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty,0,BufferSize);
    sem_init(&full,0,0);
    
    pthread_mutex_init(&mutex1, NULL);
    sem_init(&empty1,0,BufferSize);
    sem_init(&full1,0,0);
    
    pthread_mutex_init(&mutexSnap, NULL);
    sem_init(&snapEmpty,0,BufferSize);
    sem_init(&snapFull,0,0);
        
    
    pthread_create(&pro, NULL, (void *)Emissor, NULL);
    
    pthread_create(&con, NULL, (void *)Receive, NULL);
   
   printf("Process: %d, Clock: (%d, %d, %d)\n", 2, clock1.p[0], clock1.p[1], clock1.p[2]);
   
   Event(pid, &clock1);
   
   
   printf("Process: %d, Clock: (%d, %d, %d)\n", 2, clock1.p[0], clock1.p[1], clock1.p[2]);
   Snapshot();
   Event(pid, &clock1);
   clock1.destination=0;
   producer1(clock1);
   
   printf("Process: %d, Clock: (%d, %d, %d)\n", 2, clock1.p[0], clock1.p[1], clock1.p[2]);

   clock1=consumer2();
   Event(pid, &clock1);

   printf("Process: %d, Clock: (%d, %d, %d)\n", 2, clock1.p[0], clock1.p[1], clock1.p[2]);


    pthread_join(pro, NULL);
    pthread_join(con, NULL);

    pthread_mutex_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full); 
    
    pthread_mutex_destroy(&mutex1);
    sem_destroy(&empty1);
    sem_destroy(&full1); 
    
    pthread_mutex_destroy(&mutexSnap);
    sem_destroy(&snapEmpty);
    sem_destroy(&snapFull); 
}



int main(void) {
   int my_rank;    
   

   MPI_Init(NULL, NULL); 
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 

   if (my_rank == 0) { 
      process0();
   } else if (my_rank == 1) {  
      process1();
   } else if (my_rank == 2) {  
      process2();
   }

   /* Finaliza MPI */
   MPI_Finalize(); 
   
   return 0;
}  /* main */