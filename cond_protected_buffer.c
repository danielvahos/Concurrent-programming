#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "circular_buffer.h"
#include "protected_buffer.h"
#include "utils.h"

// Initialise the protected buffer structure above. 
protected_buffer_t * cond_protected_buffer_init(int length) {
  protected_buffer_t * b;
  b = (protected_buffer_t *)malloc(sizeof(protected_buffer_t));
  b->buffer = circular_buffer_init(length);
  // Initialize the synchronization components
  pthread_mutex_init(&(b->mutex), NULL); //initialization of mutex in prot buffer
  pthread_cond_init(&(b->v_empty),NULL); //initialization of cond in prot buffer
  pthread_cond_init(&(b->v_full),NULL); //initialization of cond in prot buffer
  return b;
}

// Extract an element from buffer. If the attempted operation is
// not possible immedidately, the method call blocks until it is.
void * cond_protected_buffer_get(protected_buffer_t * b){
  void * d;
  
  // Enter mutual exclusion

  pthread_mutex_lock(&b->mutex);
  
  // Wait until there is a full slot to get data from the unprotected
  // circular buffer (circular_buffer_get).

  while ((d = circular_buffer_get(b->buffer)) == NULL){
    pthread_cond_wait(&(b->v_full), &(b->mutex));
  }

  //print_task_activity ("get", d);
  // Signal or broadcast that an empty slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&(b->v_empty));


  //d = circular_buffer_get(b->buffer);
  print_task_activity ("get", d);

  // Leave mutual exclusion
  pthread_mutex_unlock(&(b->mutex));
  return d;
}

  //condfull - condempty for the get
  //condempty - condfull fot the put

// Insert an element into buffer. If the attempted operation is
// not possible immedidately, the method call blocks until it is.
void cond_protected_buffer_put(protected_buffer_t * b, void * d){

  // Enter mutual exclusion
  pthread_mutex_lock(&b->mutex);
  // Wait until there is an empty slot to put data in the unprotected
  // circular buffer (circular_buffer_put).
  while((circular_buffer_put(b->buffer, d))==0){
    pthread_cond_wait(&(b->v_empty),&(b->mutex));
  }

  // Signal or broadcast that a full slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&(b->v_full));

  circular_buffer_put(b->buffer, d);
  print_task_activity ("put", d);

  // Leave mutual exclusion
  pthread_mutex_unlock(&(b->mutex));
}

// Extract an element from buffer. If the attempted operation is not
// possible immedidately, return NULL. Otherwise, return the element.
void * cond_protected_buffer_remove(protected_buffer_t * b){
  void * d;
  

  // Signal or broadcast that an empty slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->v_empty);


  d = circular_buffer_get(b->buffer);
  print_task_activity ("remove", d);
  
  return d;
}

// Insert an element into buffer. If the attempted operation is
// not possible immedidately, return 0. Otherwise, return 1.
int cond_protected_buffer_add(protected_buffer_t * b, void * d){
  int done;
  
  // Enter mutual exclusion
  pthread_mutex_lock(&b->mutex);
  
  // Signal or broadcast that a full slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->v_full);

  done = circular_buffer_put(b->buffer, d);
  if (!done) d = NULL;
  print_task_activity ("add", d);

  // Leave mutual exclusion
  pthread_mutex_unlock(&(b->mutex));

  return done;
}

// Extract an element from buffer. If the attempted operation is not
// possible immedidately, the method call blocks until it is, but
// waits no longer than the given timeout. Return the element if
// successful. Otherwise, return NULL.
void * cond_protected_buffer_poll(protected_buffer_t * b, struct timespec *abstime){
  void * d = NULL;
  int    rc = 0;
  
  // Enter mutual exclusion
  pthread_mutex_lock(&b->mutex);
  
  //Wait until there is a full slot to get data from the unprotected
  //(ERROR) Wait until there is an empty slot to put data in the unprotected
  // circular buffer (circular_buffer_put) but waits no longer than
  // the given timeout.

  while (b->buffer->size==0 && rc != ETIMEDOUT){
    rc = pthread_cond_timedwait(&(b->v_full), &(b->mutex),abstime);
  }
/*

  while ((d=circular_buffer_get(b->buffer)) == NULL && rc != ETIMEDOUT){
    rc = pthread_cond_timedwait(&(b->v_empty), &(b->mutex),abstime);
  }
  */
/*
  while((circular_buffer_put(b->buffer, d))==0){
  pthread_cond_timedwait(&(b->v_empty),&(b->mutex),&*abstime);
  }
  */
  
  // Signal or broadcast that a full slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->v_full);
  
  d = circular_buffer_get(b->buffer);
  print_task_activity ("poll", d);

  // Leave mutual exclusion
  pthread_mutex_unlock(&(b->mutex));

  return d;
}

// Insert an element into buffer. If the attempted operation is not
// possible immedidately, the method call blocks until it is, but
// waits no longer than the given timeout. Return 0 if not
// successful. Otherwise, return 1.
int cond_protected_buffer_offer(protected_buffer_t * b, void * d, struct timespec * abstime){
  int rc = 0;
  int done = 0;
  
  // Enter mutual exclusion
  pthread_mutex_lock(&b->mutex);
  

  // Wait until there is an empty slot to put data in the unprotected
  // circular buffer (circular_buffer_put) but waits no longer than
  // the given timeout.

/*
  while((circular_buffer_put(b->buffer, d))==0){
  pthread_cond_timedwait(&(b->v_full),&(b->mutex),&*abstime);
  }
  */

  while(b->buffer->size==b->buffer->max_size && rc != ETIMEDOUT){
    rc= pthread_cond_timedwait(&(b->v_empty),&(b->mutex), abstime);
  }
  
  // Signal or broadcast that a full slot is available in the
  // unprotected circular buffer (if needed)
  pthread_cond_broadcast(&b->v_full);

  done = circular_buffer_put(b->buffer, d);
  if (!done) d = NULL;
  print_task_activity ("offer", d);
    
  // Leave mutual exclusion
  pthread_mutex_unlock(&(b->mutex));
  return done;
}
