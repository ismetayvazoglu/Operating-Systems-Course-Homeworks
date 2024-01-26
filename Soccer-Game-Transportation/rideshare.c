#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


/*
sem_t s; sem_init(&s, 0, X);

int sem_wait(sem_t *s) {
  decrement the value of semaphore s by one
  wait if value of semaphore s is negative
}

int sem_post(sem_t *s) {
  increment the value of semaphore s by one
  if there are one or more threads waiting, wake one
}
*/


int car_id = -1;
int aFan = 0;
int bFan = 0;
int driver_c = 0;

sem_t mutex;
sem_t a;
sem_t b;

pthread_t driver_id;

void *fan_thread(void *arg) {

    sem_wait(&mutex);
    
    char team = *((char *) arg);
    pthread_t tid = pthread_self();
    printf("Thread ID: %lu, Team: %c, I am looking for a car\n",
        (unsigned long) tid, team);
    
    
    if (team == 'A'){
      aFan++;
      
      if (aFan == 4){
        driver_id = pthread_self();
        aFan -=4;
        car_id++;
        sem_post(&a);
        sem_post(&a);
        sem_post(&a);
        
      }
      else if (aFan == 2 && bFan >= 2){
        driver_id = pthread_self();
        aFan -=2;
        bFan -=2;
        car_id++;
        sem_post(&a);
        sem_post(&b);
        sem_post(&b);
        
      }
      else{
        sem_post(&mutex);
        sem_wait(&a);
      }
    }
    else{
      bFan++;
      
      if (bFan == 4){
        driver_id = pthread_self();
        bFan -=4;
        car_id++;
        sem_post(&b);
        sem_post(&b);
        sem_post(&b);
        
      }
      else if (bFan == 2 && aFan >= 2){
        driver_id = pthread_self();
        bFan -=2;
        aFan -=2;
        car_id++;
        sem_post(&b);
        sem_post(&a);
        sem_post(&a);
        
      }
      else{
        sem_post(&mutex);
        sem_wait(&b);
      }
    } 
    
    printf("Thread ID: %lu, Team: %c, I have found a spot in a car\n",
        (unsigned long) tid, team);
    driver_c++;
        
    if (pthread_self() == driver_id){
      while(driver_c != 4){
        //spin-wait
      }
      printf("Thread ID: %lu, Team: %c, I am the captain and driving the car with ID %d\n",
            (unsigned long) tid, team, car_id);
      driver_c =0;      
      sem_post(&mutex);
    }  
    
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {

    // Check if there are exactly three command-line arguments
    if (argc != 3) {
        printf("Usage: %s <A> <B>\n", argv[0]);
        return 1; // Return an error code
    }
    // Convert command-line arguments to integers
    int A = atoi(argv[1]);
    int B = atoi(argv[2]);

    

    // Check conditions
    if (A % 2 == 0 && B % 2 == 0 && (A+B) % 4 == 0) {
        
        sem_init(&mutex, 0, 1);
        sem_init(&a, 0, 0);
        sem_init(&b, 0, 0);
        
        // Create arrays to store thread IDs
        pthread_t a_threads[A];
        pthread_t b_threads[B];
        
        // Create fan threads for Team A
        for (int i = 0; i < A; i++) {
            pthread_create(&a_threads[i], NULL, fan_thread, "A");
            
        }

        // Create fan threads for Team B
        for (int j = 0; j < B; j++) {
            pthread_create(&b_threads[j], NULL, fan_thread, "B");
        }

        // Wait for all fan threads to finish
        for (int k = 0; k < A; k++) {
            pthread_join(a_threads[k], NULL);
        }
        for (int l = 0; l < B; l++) {
            pthread_join(b_threads[l], NULL);
        }
    }
    
    printf("The main terminates\n");

    return 0;
}
