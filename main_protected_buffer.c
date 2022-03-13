#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "protected_buffer.h"
#include "sem_protected_buffer.h"
#include "utils.h"

protected_buffer_t * protected_buffer;
pthread_t * tasks;

// Main consumer. Get consumer id as argument.
void * main_consumer(void * arg){
  int   i;
  int * id = (int *) arg;
  int * data;

  printf ("start consumer %d\n", *id);

  // Get start time t0, the deadline will be t0 + T 
  struct timespec deadline = get_start_time();

  // Use a private key to store the consumer id. Ignore this.
  pthread_setspecific(task_info_key, arg);
  
  for (i=0; i<(n_values/n_consumers); i++) {
    // Behave as a periodic task. the current deadline corresponds to
    // the previous deadline + one period
    add_millis_to_timespec (&deadline, consumer_period);
    resynchronize();
    switch (sem_consumers) {
    case BLOCKING:
      data = (int *) protected_buffer_get(protected_buffer);
      break;
    case NONBLOCKING:
      data = (int *) protected_buffer_remove(protected_buffer);
      break;
    case TIMEDOUT:
      data = (int *) protected_buffer_poll(protected_buffer, &deadline);
      break;
    default:;
    }
    if (data != NULL) free(data);
    delay_until (&deadline);
  }
  pthread_exit (NULL);
  return NULL;
}

// Main producer. Get producer id as argument.
void * main_producer(void * arg){
  int   i;
  int * id = (int *) arg;
  int * data;
  long  done;

  printf ("start producer %d\n", *id);

  // Get start time t0, the deadline will be t0 + T 
  struct timespec deadline = get_start_time();

  // Use a private key to store the producer id. Ignore this.
  pthread_setspecific(task_info_key, arg);
  
  for (i=0; i<(n_values/n_producers); i++) {

    // Allocate data in order to produce and consume it
    data = (int *)malloc(sizeof(int));

    // Data is split in two parts : first the thread number and the
    // number of data produced.
    *data=*(int *)(arg) * 100 + i;
    
    // Behave as a periodic task. the current deadline corresponds to
    // the previous deadline + one period.
    add_millis_to_timespec (&deadline, producer_period);
    resynchronize();
    
    switch (sem_producers) {
    case BLOCKING:
      protected_buffer_put(protected_buffer, data);
      done=1;
      break;
      
    case NONBLOCKING:
      done=protected_buffer_add(protected_buffer, data);
      break;
      
    case TIMEDOUT:
      done=protected_buffer_offer(protected_buffer, data, &deadline);
      break;
    default:;
    }
    if (!done) data = NULL;
    delay_until (&deadline);
  }
  pthread_exit (NULL);
  return NULL;
}

// Read scenario file
void read_file(char * filename);

int main(int argc, char *argv[]){
  int   i;
  int * data;
  
  if (argc != 2) {
    printf("Usage : %s <scenario file>\n", argv[0]);
    exit(1);
  }

  init_utils();
  read_file(argv[1]);
  
  protected_buffer = protected_buffer_init(sem_impl, buffer_size);


  set_start_time();
  
  // Create consumers and then producers. Pass the *value* of i
  // as parametre of the main procedure (main_consumer or main_producer).
    //Thread for producers:
  //pthread_t producers[n_producers];
  //int *arg_prod;

  //int *arg_consum;
  int *arg;
  //pthread_t consumers[n_consumers];
tasks= malloc(sizeof(n_producers+n_consumers));//We have to give it a SIZE
//We first create consumers
  for (i=0; i<n_consumers; i++) {
    //arg_consum=malloc(sizeof(int));
    //*arg_consum=i;

    arg=malloc(sizeof(int));
    *arg=i;
    pthread_create(&tasks[i], NULL, main_consumer, arg);

    //tasks[i]= &consumers[i]; //save the threads of consumers in tasks
  }

  //and then.. we create producers
  for (i=n_consumers; i<n_producers+n_consumers; i++) {

    //arg_prod= malloc(sizeof(int));
    //*arg_prod= i;

    arg= malloc(sizeof(int));
    *arg=i;
    pthread_create(&tasks[i], NULL, main_producer, arg);
    //tasks[i]= &producers[i]; //save the threads of producers in tasks
  }



  // Wait for producers and consumers termination
  for (i=0; i<n_consumers+n_producers; i++) {
    pthread_join(tasks[i], NULL); //we have to wait until null to know it's terminated
    // because in producers and consumers, for each thread they return NULL
  
  }
}

void read_file(char * filename){
  FILE * file;

  file = fopen (filename, "r");
  
  get_string (file, "#sem_impl", __FILE__, __LINE__);
  get_long   (file, (long *) &sem_impl, __FILE__, __LINE__);
  printf ("sem_impl = %ld\n", sem_impl);
  
  get_string (file, "#sem_consumers", __FILE__, __LINE__);
  get_long   (file, (long *) &sem_consumers, __FILE__, __LINE__);
  printf ("sem_consumers = %ld\n", sem_consumers);
  
  get_string (file, "#sem_producers", __FILE__, __LINE__);
  get_long   (file, (long *) &sem_producers, __FILE__, __LINE__);
  printf ("sem_producers = %ld\n", sem_producers);
  
  get_string (file, "#buffer_size", __FILE__, __LINE__);
  get_long   (file, (long *) &buffer_size, __FILE__, __LINE__);
  printf ("buffer_size = %ld\n", buffer_size);
  
  get_string (file, "#n_values", __FILE__, __LINE__);
  get_long   (file, (long *) &n_values, __FILE__, __LINE__);
  printf ("n_values = %ld\n", n_values);
  
  get_string (file, "#n_consumers", __FILE__, __LINE__);
  get_long   (file, (long *) &n_consumers, __FILE__, __LINE__);
  printf ("n_consumers = %ld\n", n_consumers);
  
  get_string (file, "#n_producers", __FILE__, __LINE__);
  get_long   (file, (long *) &n_producers, __FILE__, __LINE__);
  printf ("n_producers = %ld\n", n_producers);
  
  get_string (file, "#consumer_period", __FILE__, __LINE__);
  get_long   (file, (long *) &consumer_period, __FILE__, __LINE__);
  printf ("consumer_period = %ld\n", consumer_period);
  
  get_string (file, "#producer_period", __FILE__, __LINE__);
  get_long   (file, (long *) &producer_period, __FILE__, __LINE__);
  printf ("producer_period = %ld\n", producer_period);
}


/*

/// PROD AND CONSUM

int main(void){
  //Thread for producers:
  pthread_t producers[n_producers];
  int id_prod;
  int *arg_prod;

  int id_consum;
  int *arg_consum;

  for (id_prod= 0; id_prod< n_producers; id_prod++){
    arg_prod= malloc(sizeof(int));
    *arg_prod= id_prod;
    pthread_create(&producers[id_prod], NULL, main_producer, arg_prod);
  }

  pthread_t consumers[n_consumers];
  for (id_consum=0; id_consum<n_consumers; id_consum++){
    arg_consum=malloc(sizeof(int));
    *arg_consum=id_consum;
    pthread_create(&consumers[id_consum], NULL, main_consumer, arg_consum);
  }
}
*/