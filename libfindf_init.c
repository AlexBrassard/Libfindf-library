/*
 *
 * Libfindf.so  -  Initialization, Termination routines.
 * Version:        0.01
 * Version day 0:  06/06/16
 *
 */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>

#include <findf.h>
#include "libfindf_private.h"



/* Library's globals */

findf_list_t *temporary_container;   /* Global list used by all threads as temporary buffer. */

#pragma GCC visibility push(default)
pthread_mutex_t stderr_mutex;        /* Lock to serialize debug messages on stderr stream. */
#pragma GCC visibility pop
/* 
 * intern__findf__opendir() is called on every single entry in the system.
 * In the hope to avoid the overhead of having 1 global 'entry' (readdir_r)
 * parameter and all the synchronization mechanisms involved and 
 * to avoid making billions of calls to malloc/free, we set 'entry' as a thread-safe data type.
 */
pthread_key_t entry_key;             /* Readdir_r 'entry' argument's TSD key. */


/* Initialize the library. */
int __attribute__ ((constructor)) intern__findf__lib_init(void)
{
  
  /* Create the Pthread key used by __findf_entry_location(). */
  if(pthread_key_create(&entry_key, intern__findf__free_pthread_key) != 0){
    perror("pthread_key_create");
    return ERROR;
  }
  /* Initialize the global findf_list_t list. */
  if ((temporary_container = intern__findf__init_node(DEF_LIST_SIZE, 0, true)) != RF_OPSUCC){

    return ERROR;
  }
  /* Initialize the Pthread mutex to serialize stderr output stream in debug messages. */
  if (pthread_mutex_init(&stderr_mutex, NULL) != 0){
    perror("pthread_mutex_init");
    return ERROR;
  }
  return RF_OPSUCC;
}

/* Clean-up behind ourself */
int __attribute__ ((destructor)) intern__findf__lib_finit(void)
{
  struct dirent *temp = NULL;

  /* Release resources of the global linked-list. */
  intern__findf__destroy_list(temporary_container);
  /* Make sure we leave no TSD behind ourselfs. */
  if ((temp = intern__findf_entry_location()) != NULL){
    free(temp);
  }
  /* Release resources of the inited pthread key. */
  if (pthread_key_delete(entry_key) != 0){
    perror("pthread_key_delete");
    return ERROR;
  }
  /* Release resources of the stderr Pthread mutex. */
  if (pthread_mutex_destroy(&stderr_mutex) != 0){
    perror("pthread_mutex_destroy");
    return ERROR;
  }

  return RF_OPSUCC;
}

/* Get the location of per-thread 'entry' parameter of readdir_r() routine. */
struct dirent *intern__findf_entry_location(void)
{
  /* Sets readdir_r's 'entry' argument as thread specific data. */
  struct dirent *entry;

  /* Always try to return an already initialized value first. */
  if ((entry = pthread_getspecific(entry_key)) == NULL){
    if ((entry = malloc(sizeof(struct dirent))) == NULL ){
      perror("malloc");
      return NULL;
    }
    if (pthread_setspecific(entry_key, (void *)entry) != 0){
      perror("pthread_setspecific");
      return NULL;
    }
    /* Call pthread_getspecific again and return the address of our new TSD. */
    if ((entry = pthread_getspecific(entry_key)) == NULL){
      perror("pthread_getspecific");
      return NULL;
    }
    return entry;
  }
  else /* TSD already inited. */
    return entry;
}
    
/* Release a pthread_key_t */
void intern__findf__free_pthread_key(void *key)
{
  free(key);
  key = NULL;
}
