// Author: Rayanne Souza
// Last modification: 14 Sep 2018
//

#include<pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>

#define SHARED 1
#define SIZE 10

void *Producer(void *);
void *Consumer(void *);


sem_t entry, prod, cons;
int **consumed, *vproduced;
int gfront=0, rear=0, buffer[SIZE], qtr[SIZE], empty=SIZE;
int n_producers, n_consumers;
int dc=0, dp=0,  // numero de consumidores e produtores em espera  
    dcf=0, // variavel booleana para indicar que tem consumidores aguardando elemento para consumir
    dpe=0, // variavel booleana para indicar que um produtor aguarda espaco no buffer para preencher
    np=0, nc=0, 
    items_produced=0; 


int main(int argc, char *argv[]){
	int i;
	pthread_t pid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	sem_init(&entry, SHARED, 1);
	sem_init(&prod, SHARED, 0);
	sem_init(&cons, SHARED, 0);

	for(i = 0; i < SIZE; i++)
		qtr[i] = 0;

	n_producers = atoi(argv[1]);
	n_consumers = atoi(argv[2]);

	consumed = (int**)malloc(n_consumers*sizeof(int*));
	vproduced = (int*)malloc(n_producers*sizeof(int));
	if(consumed == NULL || vproduced == NULL){
		printf("Memory not allocated \n");	
		exit(1);
	}

	for(i = 0; i < n_consumers; i++){
		consumed[i] = (int*)malloc(n_producers*sizeof(int));
		if(consumed [i] == NULL){
			printf("Memory not allocated\n");       
			exit(1);
		}
	}
	

	pthread_create(&pid, &attr, Producer, NULL);

	pthread_t cid[n_consumers];
	for(i = 0; i < n_consumers; i++) 
		pthread_create(&cid[i], &attr, Consumer, (void*) i);
	

	pthread_join(pid, NULL);
		
	for(i = 0; i < n_consumers; i++)
		pthread_join(cid[i], NULL);


	// TEST
	for(i = 0; i < n_consumers; i++){
		for(int j = 0; j < n_producers; j++)
			assert(consumed[i][j] == vproduced[j]);
	}

	free(vproduced);
	for(i = 0; i < n_consumers; i++)
               free(consumed[i]);
	free(consumed);

	return 0;		
}

void *Producer(void *arg){
	int produced;

	for(produced = 1; produced <= n_producers; produced++){
		
		sem_wait(&entry);
		
		if(nc > 0 || empty == 0){
			dp = dp + 1;
			if(empty == 0)
				dpe = 1;

			sem_post(&entry);
			sem_wait(&prod);
		}
	
		np = np + 1;
		sem_post(&entry);
				
		buffer[rear] = produced;
		vproduced[items_produced] = produced;
		

		printf("Producer %d generates %d \n", produced, buffer[rear]);

		sem_wait(&entry);
		rear = (rear + 1)%SIZE;
		np = np - 1;
		empty --;
		items_produced++;	


		if(dc > 0){
			dc = dc - 1;
			if(dcf)
				dcf = 0;
			sem_post(&cons);
		}
		else
			sem_post(&entry);
	
		printf("PRODUCER %d FINISHED\n", produced);	
	}
}

void *Consumer(void *id){

	int numIters=1, total=0, front=0, items_consumed=0;
	int index = (int) id;

	while(numIters <= n_producers){      
		numIters ++;

	 	sem_wait(&entry);
		if(nc > 0 || np > 0 || items_consumed == items_produced){
			dc = dc + 1;
			if(items_consumed == items_produced)
				dcf = 1;

			sem_post(&entry);
wait:			sem_wait(&cons);
		}
		// Se o consumidor liberado estava esperando por um novo item no buffer e esse item ainda
		// nao foi produzido, ele volta a aguardar. 
		if(dcf){
			if(items_consumed == items_produced){
				if(dc > 0)
					sem_post(&cons);
				
				else if(empty > 0 && dpe > 0){
                        		dpe = 0;
                        		dp = dp - 1;	
                        		sem_post(&prod);
                		}
                		else if(dp > 0 && empty > 0){
                        		dp = dp - 1;
                        		sem_post(&prod);
                		}
				else 
					sem_post(&entry);
				goto wait;
			}
		}	

		
		nc = nc + 1;
		sem_post(&entry);	
		
		
		total = total + buffer[front];
		consumed[index][items_consumed] = buffer[front];
	

		printf("Consumer %d gets %d \n", index, buffer[front]);
		
		sem_wait(&entry);	
		nc = nc - 1;
		items_consumed++;
		qtr[front] = qtr[front] + 1;
		front = (front + 1)%SIZE; 

		if(qtr[gfront] == n_consumers){
                        qtr[gfront] = 0;
                        gfront = (gfront + 1)%SIZE;
                      	empty++;
                }
         
		if(empty > 0 && dpe > 0){
			dpe = 0;
			dp = dp - 1;
			sem_post(&prod);
		}
		else if(dp > 0 && empty > 0){
                        dp = dp - 1;
                        sem_post(&prod);
                }
		else if(dc > 0){
			dc = dc - 1;
			sem_post(&cons);
		}
    		else 
                        sem_post(&entry);
		
      	
      	}

	printf("CONSUMER %d ENDED WITH TOTAL %d\n", index, total);
	pthread_exit(NULL); 
}
