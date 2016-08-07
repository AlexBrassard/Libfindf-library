/*
 *
 *
 * Libfindf.so  -  Library's public header file.
 *
 *
 */

#ifndef LIBFINDF_PUBLIC_HEADER

# pragma GCC visibility push(default)
# define LIBFINDF_PUBLIC_HEADER 1

# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <stdbool.h>
# include <errno.h>


/* Serialized debug and error message MACRO */
extern pthread_mutex_t stderr_mutex;
# define findf_perror(string) do {				\
    pthread_mutex_lock(&stderr_mutex);				\
    if (errno) {						\
      perror(__func__);						\
    }								\
    fprintf(stderr, "\nThread ID [%lu]\nIn: %s @ L%d: %s\n\n",	\
	    pthread_self(),__func__, __LINE__, string);		\
    pthread_mutex_unlock(&stderr_mutex);			\
  } while (0);

/* Data-types */

/* 
 * Homemade lists, careful not to confuse the size, 
 * which is the total number of N elements in the pathlist[N][F_MAXPATHLEN] array,
 * and position, which is the first free B position of the pathlist[B][F_MAXPATHLEN] array.
 */
typedef struct list_t {
  size_t          size;                   /* Size of the outer pathlist array. */
  size_t          position;               /* Position of the first free element of pathlist. */
  size_t          list_level;             /* Filesystem 'level' of the current node's pathlist. */
  pthread_mutex_t *list_lock;             /* Pointer to a Pthread mutex in case of a global list. */
  struct list_t   *next;                  /* Next node in the linked-list. */
  char            **pathlist;             /* Array of strings (absolute pathnames). */

} findf_list_t;

/* Libfindf 'Type of search' flag data type. */
typedef enum srch_type_f {
  CUSTOM = 0,                              /* Use a custom algorithm. */
  DFS,                                     /* Built-in Dept-first(-like) search algorithm. */
  IDDFS,                                   /* Built-in Itterative-deepening dept-first(-like) search. */   
  BFS,                                     /* Built-in Breath-first(-like) search algorithm. */
  IDBFS                                    /* Built-in Itterative-deepening breath-first(-like) search. */
  
} findf_type_f; /* _f == "flag" */

/* Libfindf 'type of sorting' flag data type. */
typedef enum sort_type_f {
  NONE = -1,                                /* Results are passed back to the caller unsorted. */
  C_SORT = 0,                               /* Custom sorting algorithm. */
  SORTP                                     /* Libfindf built-in hybrid sorting algorithm. */

} findf_sort_type_f;

/* Libfindf search parameter data structure. */
typedef struct srch_param_t {
  char              **file2find;            /* Array of strings (Filenames). */
  size_t            sizeof_file2find;       /* Number of elements within file2find. */
  findf_list_t      *search_roots;          /* List pointer, 'search of root' absolute pathnames. */
  size_t            sizeof_search_roots;    /* Number of elements within search_roots list. */
  findf_list_t      *search_results;        /* List pointer, results of the search. */
  unsigned int      dept;                   /* Maximum inclusive dept to search the filesystem. */
  findf_type_f      search_type;            /* CUSTOM, DFS, IDDFS, BFS, IDBFS types of algorithm. */
  findf_sort_type_f sort_type;              /* NONE, CUSTOM, SORTP types of sort. */
  void              *(*sort_f)(void *);     /* Pointer to a custom sort algorithm. */
  void              *sarg;                  /* Argument passed to a custom sort routine. */
  void              *(*algorithm)(void *);  /* Pointer to a custom search algorithm. */
  void              *arg;                   /* Argument passed to a custom search algorithm. */

} findf_param_t;

/* Libfindf thread pool data structure. */
typedef struct tpool_t{
  unsigned long    num_of_threads;          /* Number of threads within the pool. */
  pthread_t        *threads;                /* Array of num_of_threads pthread_t object. */

} findf_tpool_t;

/* Libfindf, findf() specific, results data structure. */
typedef struct reslist{
  size_t           numof_elements;          /* Number of elements within res_buf. */
  /* Lenght of the longest res_buf[string][] before the terminating NULL byte. */
  size_t           max_string_len;          
  char             **res_buf;               /* Buffer of results. */

} findf_results_t;




/* Constants */


/* Maximum allowed lenght of pathnames. (Built-in default = (Pathname = /name/ * 15)). */
#define F_MAXPATHLEN 4096               /* Arbitrary. Admins may change it to fit their systems' needs. */

/* Default findf_list_t initial size. */
#define DEF_LIST_SIZE 512               /* Aribtrary. Admins may change it to fit their systems' needs. */

/* Maximum lenght of names. */ 
#define F_MAXNAMELEN 256                /* Aribtrary. Admins may change it to fit their systems' needs. */

/* Default number of threads, in case sysconf() returned -1. */
#define DEF_THREADS_NUM 2               /* Aribtrary. Admins may change it to fit their systems' needs. */

/* Default maximum number of created threads. */
#define DEF_MAX_THREADS_NUM 32          /* Aribtrary. Admins may change it to fit their systems' needs. */

/* Default standard Unix root. */
#define DEF_UNIX_ROOT "/\0"

/* Default standard Unix root lenght. */
#define DEF_UNIX_ROOT_LEN 2


/* Prototypes */


/* Safely copy a string src into a destination dest previously initialized to n. */
void *SU_strcpy(char *dest, char *src, size_t n);

/* Straight-forward system-wide search. */
findf_results_t* findf(char *file2find,
		       size_t file2find_len,
		       bool IS_BUF);

/* Fine-grained search. */
int findf_fg(findf_param_t *search_param);

#ifdef CW_FINDF_ADVANCED
/* Advanced, fine-grained search. */
int findf_adv(findf_param_t *search_param,
	      void* (*algorithm)(void *),
	      void* (*sort)(void *),
	      void *arg,
	      void *sarg);
#endif /* CW_FINDF_ADVANCED */

/* Initialize a findf_param_t search parameter object. */
findf_param_t* findf_init_param(char **file2find,
				char **search_roots,
				size_t numof_file2find,
				size_t numof_search_roots,
				unsigned int max_inc_dept,
				findf_type_f search_type,
				findf_sort_type_f sort_type);

/* Release resources of a findf_param_t search parameter object. */
int findf_destroy_param(findf_param_t *to_free);

/* Print all results within a results container to stdout stream. */
int findf_read_results(findf_results_t *to_read);

/* Release resources of a findf_results_t result container object. */
int findf_destroy_results(findf_results_t *to_free);


# pragma GCC visibility pop
#endif /* LIBFINDF_PUBLIC_HEADER */
