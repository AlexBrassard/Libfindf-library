
/*

  Libfindf.so  -  Main private header file.
  Version:        0.01
  Version day 0:  06/06/16

*/
#ifndef FINDF_PRIVATE_HEADER
#define FINDF_PRIVATE_HEADER 1

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pthread.h>
#include <dirent.h>

#include <findf.h>

/* Private constants */

/* Represents a succesful operation. */
#define RF_OPSUCC 0
/* Represents an error. */
#define ERROR -1
/* Custom error message. */

#define intern_errormesg(string) do {					\
    pthread_mutex_lock(&stderr_mutex);					\
    if (errno)								\
      perror(__func__);							\
    fprintf(stderr, "\nThread [%lu]\nIn %s @ L%d:\n%s\n\n",		\
	    pthread_self(), __func__, __LINE__, string);		\
    pthread_mutex_unlock(&stderr_mutex);				\
  } while (0) ;


/* 
 * Represents the dot '.' and dotdot '..' directories
 * and their respective lenghts. 
 */
static const char DOT[] = ".\0";
static const char DOTDOT[] = "..\0";
static const char DOTLEN = 2;
static const char DOTDOTLEN = 3;


extern findf_list_t *temporary_container;   /* Global list used by all threads as temporary buffer. */
extern pthread_mutex_t stderr_mutex;        /* Lock to serialize debug messages on stderr stream. */ 


/* Prototypes */


/* Initialize the library. */
int intern__findf__lib_init(void);

/* Terminate the library. */
int intern__findf__lib_finit(void);

/* Readdir_r 'entry' argument's thread specific data location. */
struct dirent *intern__findf_entry_location(void);

/* Used by the Pthread library to release resources of a pthread_key_t object. */
void intern__findf__free_pthread_key(void *key);

/* Main internal routine. */
int intern__findf__internal(findf_param_t *callers_param);


/* Breath-First-like search algorithm. */
void *intern__findf__BF_search(void *param);

/* Dept-First-like search algorithm */
void *intern__findf__DF_search(void *param);

/* Initialize a findf_list_t node. */
findf_list_t *intern__findf__init_node(size_t size,
				       size_t list_level_s,
				       bool ISGLOBAL);

/* Release a findf_list_t node. */
int intern__findf__free_node(findf_list_t *to_free);

/*  Swap the headnode with its ->next field. */
findf_list_t *intern__findf__shift_node(findf_list_t *headnode);

/* Add a pathname to an existing pathlist, extend it's size if needed. */
int intern__findf__add_element(char *element,
			       findf_list_t *list);

/* Destroy an entire findf_list_t linked-list. */
int intern__findf__destroy_list(findf_list_t *headnode);

/* Initialize a parameter object. */
findf_param_t *intern__findf__init_param(char **file2find,
					 char **search_roots,
					 size_t numof_file2find,
					 size_t numof_search_roots,
					 unsigned int dept,
					 findf_type_f search_type,
					 findf_sort_type_f sort_type,
					 void *(*sort_f)(void *sarg),
					 void *sarg,
					 void *(*algorithm)(void *arg),
					 void *arg);

/* Release resources of a parameter object. */
int intern__findf__free_param(findf_param_t *to_free);

/* Open a directory, search it and store results in the given list. */
int intern__findf__opendir(char *pathname,
			   findf_list_t *nextnode,
			   findf_param_t *t_param);

/* Append element to dest of size destsize in the form of a pathname. */
int intern__findf__path_forward(char *dest,
				char *element,
				size_t destsize);

/*
 * Compare the current entry with file(s) to find and 
 * add a successful match to the given parameter object's appropriate list. 
 */
void intern__findf__cmp_file2find(findf_param_t *t_param,
				  char *entry_to_cmp,
				  char *entry_full_path);

/* Initialize a thread pool. */
findf_tpool_t *intern__findf__init_tpool(unsigned long numof_threads);

/* Destroy a thread_pool. */
void intern__findf__free_tpool(findf_tpool_t *to_free);

/* Get the maximum allowed number of cpus/cores for our process. */
size_t intern__findf__get_avail_cpus(void);

/* Initialize a findf_results_t object. */
findf_results_t *intern__findf__init_res(size_t bufsize,
					 char **buffer);

/* Destroy a findf_results_t object. */
void intern__findf__free_res(findf_results_t *to_free);

/* Sort an array of pathnames. */
int intern__findf__sortp(char **sort_buf,
			 char **file2find,
			 size_t sizeof_sort_buf,
			 size_t sizeof_file2find);

/* Rotate a buffer of type size_t * */
int intern__findf__rotate_buffer(size_t *sorted_array,
				 size_t ind_to_move,
				 size_t *last_pos_ind);
#endif /* FINDF_PRIVATE_HEADER */
