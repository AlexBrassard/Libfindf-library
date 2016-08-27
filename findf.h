/*
 *
 *
 * Libfindf.so  -  Library's public header file.
 *
 *
 */


#ifndef LIBFINDF_PUBLIC_HEADER


# define LIBFINDF_PUBLIC_HEADER 1

# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <stdbool.h>
# include <errno.h>
# include <regex.h>


/* Serialized debug and error message MACRO */
extern pthread_mutex_t stderr_mutex;
# define findf_perror(string) do {				\
    pthread_mutex_lock(&stderr_mutex);				\
    if (errno) {						\
      perror(__func__);						\
    }								\
    fprintf(stderr, "\nThread ID [%zu]\nIn: %s @ L%d: %s\n\n",	\
	    pthread_self(),__func__, __LINE__, string);		\
    pthread_mutex_unlock(&stderr_mutex);			\
  } while (0);

/* 
 * Data-types 
 * Most fields are used internally, removing any might break Libfindf.so .
 * Fields revelant to a user of of the library should search the
 * appropriate man-pages. (findf(3) && "SEE ALSO" section).
 */

/* 
 * Libfindf's lists, careful not to confuse the size, 
 * which is the total number of N elements in the pathlist[N][F_MAXPATHLEN] array,
 * and position, which is the first free B position of the pathlist[B][F_MAXPATHLEN] array.
 */
typedef struct list_f {
  size_t          size;                   /* Size of the outer pathlist array. */
  size_t          position;               /* Position of the first free element of pathlist. */
  size_t          list_level;             /* Filesystem 'level' of the current node's pathlist. */
  pthread_mutex_t *list_lock;             /* Pointer to a Pthread mutex in case of a global list. */
  struct list_f   *next;                  /* Next node in the linked-list. */
  char            **pathlist;             /* Array of strings (absolute pathnames). */

} findf_list_f;

/* Libfindf 'Type of search' flag data type. */
typedef enum srch_type_f {
  CUSTOM = 0,                              /* Use a custom algorithm. */
  DFS,                                     /* Built-in Dept-first(-like) search algorithm. */
  IDDFS,                                   /* Built-in Itterative-deepening dept-first(-like) search. */   
  BFS,                                     /* Built-in Breath-first(-like) search algorithm. */
  IDBFS                                    /* Built-in Itterative-deepening breath-first(-like) search. */
  
} findf_type_f;

/* Libfindf 'type of sorting' flag data type. */
typedef enum sort_type_f {
  NONE = -1,                                /* Results are passed back to the caller unsorted. */
  C_SORT = 0,                               /* Custom sorting algorithm. */
  SORTP                                     /* Libfindf built-in hybrid sorting algorithm. */

} findf_sort_type_f;


/*
 * For match op:         pattern1: Stripped off pattern.
 *                       pattern2: Unused.
 * For substitute op:    pattern1: Stripped off "to replace" pattern.
 *                       pattern2: Stripped off replacement pattern.
 * For transliterate op: pattern1: List of characters to be replaced.
 *                       pattern2: List of replacement characters.
 */
typedef struct fstorage{
  char             *pattern1;
  char             *pattern2;

} findf_opstore_f;

/* Libfindf's regex data structure. */
typedef struct fregex{
  bool             fre_modif_boleol;         /* True when the '\m' modifier is activated. */
  bool             fre_modif_newline;        /* True when the '\s' modifier is activated. */
  bool             fre_modif_icase;          /* True when the '\i' modifier is activated. */
  bool             fre_modif_ext;            /* True when the '\x' modifier is activated. */
  bool             fre_modif_global;         /* True when the '\g' modifier is activated. */
  bool             fre_op_match;             /* True when operation is match. */
  bool             fre_op_substitute;        /* True when operation is substitute. */
  bool             fre_op_transliterate;     /* True when operation is transliterate. */
  bool             fre_p1_compiled;          /* True when ->pattern[0] has been regcompiled. */
  bool             fre_p2_compiled;          /* True when ->pattern[1] has been regcompiled. */
  char             delimiter;                /* The delimiter used by the pattern. */
  int              (*operation)(struct fregex*); /* A pointer to the operation to execute on the regex. */
  int              fre_op_return_val;        /* operation's return value. */
  regex_t          **pattern;                /* A compiled regex pattern via a call to regcomp(). */
  findf_opstore_f  *pat_storage;             /* Type of pattern storage used, depending on the operation. */
  
} findf_regex_f;

/* Libfindf search parameter data structure. */
typedef struct srch_param_f {
  char              **file2find;            /* Array of strings (Filenames). */
  size_t            sizeof_file2find;       /* Number of elements within file2find. */
  findf_list_f      *search_roots;          /* List pointer, 'search of root' absolute pathnames. */
  size_t            sizeof_search_roots;    /* Number of elements within search_roots list. */
  findf_list_f      *search_results;        /* List pointer, results of the search. */
  unsigned int      dept;                   /* Maximum inclusive dept to search the filesystem. */
  findf_type_f      search_type;            /* CUSTOM, DFS, IDDFS, BFS, IDBFS types of algorithm. */
  findf_sort_type_f sort_type;              /* NONE, CUSTOM, SORTP types of sort. */
  void              *(*sort_f)(void *);     /* Pointer to a custom sort algorithm. */
  void              *sarg;                  /* Argument passed to a custom sort routine. */
  void              *(*algorithm)(void *);  /* Pointer to a custom search algorithm. */
  void              *arg;                   /* Argument passed to a custom search algorithm. */
  size_t            sizeof_reg_array;       /* Number of element in the reg_array array. */
  findf_regex_f     **reg_array;            /* Used by findf_re to do a regex-based search. */

} findf_param_f;

/* Libfindf thread pool data structure. */
typedef struct tpool_f{
  unsigned long    num_of_threads;          /* Number of threads within the pool. */
  pthread_t        *threads;                /* Array of num_of_threads pthread_t object. */

} findf_tpool_f;

/* Libfindf, findf() specific, results data structure. */
typedef struct reslist{
  size_t           numof_elements;          /* Number of elements within res_buf. */
  /* Lenght of the longest res_buf[string][] before the terminating NULL byte. */
  size_t           max_string_len;          
  char             **res_buf;               /* Buffer of results. */

} findf_results_f;


/* Constants */

/*
 * The following contants are mostly arbitary.
 * Admins may change them to suit their systems' needs.
 */

/* Maximum allowed lenght of pathnames. (Built-in default: (Pathname = /name/ * 15)). */
#define F_MAXPATHLEN 4096               

/* Default findf_list_f initial size. */
#define DEF_LIST_SIZE 512

/* Maximum lenght of names. */ 
#define F_MAXNAMELEN 256

/* Default number of threads, in case sysconf() returned -1. */
#define DEF_THREADS_NUM 2

/* Default maximum number of created threads. */
#define DEF_MAX_THREADS_NUM 32

/* Default maximum number of regex pattern expected. */
#define FINDF_MAX_PATTERNS 4096

/* Default maximum lenght of a single regex pattern. */
#define FINDF_MAX_PATTERN_LEN 256       /* 256 is the limit to remain POSIX conformant. */

/* Default standard Unix root. */
#define DEF_UNIX_ROOT "/\0"

/* Default standard Unix root lenght. */
#define DEF_UNIX_ROOT_LEN 2


/* Prototypes */


/* Safely copy a string src into a destination dest previously initialized to n. */
void *SU_strcpy(char *dest, char *src, size_t n);

/* Straight-forward system-wide search. */
findf_results_f* findf(char *file2find,
		       size_t file2find_len,
		       bool IS_BUF);

/* Fine-grained search. */
int findf_fg(findf_param_f *search_param);

/* Fine-grained, regex search. */
int findf_re(findf_param_f *search_param,
	     char **patterns,
	     size_t numof_patterns);

#ifdef CW_FINDF_ADVANCED
/* Advanced, fine-grained search. */
int findf_adv(findf_param_f *search_param,
	      void* (*algorithm)(void *),
	      void* (*sort)(void *),
	      void *arg,
	      void *sarg);
#endif /* CW_FINDF_ADVANCED */

/* Initialize a findf_param_f search parameter object. */
findf_param_f* findf_init_param(char **file2find,
				char **search_roots,
				size_t numof_file2find,
				size_t numof_search_roots,
				unsigned int max_inc_dept,
				findf_type_f search_type,
				findf_sort_type_f sort_type);

/* Release resources of a findf_param_f search parameter object. */
int findf_destroy_param(findf_param_f *to_free);

/* Print all results within a results container to stdout stream. */
int findf_read_results(findf_results_f *to_read);

/* Release resources of a findf_results_f result container object. */
int findf_destroy_results(findf_results_f *to_free);


#endif /* LIBFINDF_PUBLIC_HEADER */
