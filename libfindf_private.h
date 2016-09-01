
/*
 *
 * Libfindf.so  -  Main private header file.
 * Version:        0.01
 * Version day 0:  06/06/16
 *
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

/* Libfindf-regex contanst. */
#define FRE_MATCH_EXPECT_DELIMITER 2  /* Number of expected delimiters in a match pattern. */
#define FRE_SUBST_EXPECT_DELIMITER 3  /* Number of expected delimiters in a substitution pattern. */
#define FRE_MAX_SUB_MATCHES 9         /* Maximum number of sub-matches captures by libfindf. */
#define FRE_MATCH_UNSUCCESSFUL 2      /* Indicate an unsuccessful match, 0 == success (using RF_OPSUCC) . */

/* The next 2 constants are arrays since we copy them one char at a time. */
static const char FRE_DIGIT_RANGE[] = "[0-9]";
static const char FRE_NOT_DIGIT_RANGE[] = "[^0-9]";

						  
/* 
 * Represents the dot '.' and dotdot '..' directories
 * and their respective lenghts.  We compare each entries of each 
 * directories against DOT && DOTDOT
 */
static const char DOT[] = ".";
static const char DOTDOT[] = "..";
static const size_t DOTLEN = 2;
static const size_t DOTDOTLEN = 3;



/* Custom error message. */
#define intern_errormesg(string) do {					\
    pthread_mutex_lock(&stderr_mutex);					\
    if (errno)								\
      perror(__func__);							\
    fprintf(stderr, "\nThread [%zu]\nIn %s @ L%d:\n%s\n\n",		\
	    pthread_self(), __func__, __LINE__, string);		\
    pthread_mutex_unlock(&stderr_mutex);				\
  } while (0) ;


extern findf_list_f *temporary_container;   /* Global list used by all threads as temporary buffer. */
extern pthread_mutex_t stderr_mutex;        /* Lock to serialize debug messages on stderr stream. */ 


/* --------------Prototypes------------- */

/* Library's initialization and termination. */

/* Initialize the library. */
int intern__findf__lib_init(void);
/* Terminate the library. */
int intern__findf__lib_finit(void);
/* Readdir_r 'entry' argument's thread specific data location. */
struct dirent *intern__findf_entry_location(void);
/* Used by the Pthread library to release resources of a pthread_key_t object. */
void intern__findf__free_pthread_key(void *key);


/* Principal search related routines. */

/* Main internal routine. */
int intern__findf__internal(findf_param_f *callers_param);
/* Breath-First-like search algorithm. */
void *intern__findf__BF_search(void *param);
/* Dept-First-like search algorithm */
void *intern__findf__DF_search(void *param);


/* Internal search related utilities. */

/* Initialize a findf_list_f node. */
findf_list_f *intern__findf__init_node(size_t size,
				       size_t list_level_s,
				       bool ISGLOBAL);
/* Release a findf_list_f node. */
int intern__findf__free_node(findf_list_f *to_free);
/*  Swap the headnode with its ->next field. */
findf_list_f *intern__findf__shift_node(findf_list_f *headnode);
/* Add a pathname to an existing pathlist, extend it's size if needed. */
int intern__findf__add_element(char *element,
			       findf_list_f *list);
/* Destroy an entire findf_list_f linked-list. */
int intern__findf__destroy_list(findf_list_f *headnode);
/* Initialize a parameter object. */
findf_param_f *intern__findf__init_param(char **file2find,
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
int intern__findf__free_param(findf_param_f *to_free);
/* Open a directory, search it and store results in the given list. */
int intern__findf__opendir(char *pathname,
			   findf_list_f *nextnode,
			   findf_param_f *t_param);
/* Append element to dest of size destsize in the form of a pathname. */
int intern__findf__path_forward(char *dest,
				char *element,
				size_t destsize);
/*
 * Compare the current entry with file(s) to find and 
 * add a successful match to the given parameter object's appropriate list. 
 */
void intern__findf__cmp_file2find(findf_param_f *t_param,
				  char *entry_to_cmp,
				  char *entry_full_path);
/* Initialize a thread pool. */
findf_tpool_f *intern__findf__init_tpool(unsigned long numof_threads);
/* Destroy a thread_pool. */
void intern__findf__free_tpool(findf_tpool_f *to_free);
/* Get the maximum allowed number of cpus/cores for our process. */
size_t intern__findf__get_avail_cpus(void);
/* Verify if the search_type is valid, else set a valid one. */
void intern__findf__verify_search_type(findf_param_f *search_param);
/* Initialize a findf_results_f object. */
findf_results_f *intern__findf__init_res(size_t bufsize,
					 char **buffer);
/* Destroy a findf_results_f object. */
void intern__findf__free_res(findf_results_f *to_free);


/* Internal pathname sorting related routines. */

/* Sort an array of pathnames. */
int intern__findf__sortp(char **sort_buf,
			 char **file2find,
			 size_t sizeof_sort_buf,
			 size_t sizeof_file2find);
/* Rotate a buffer of type size_t* */
int intern__findf__rotate_buffer(size_t *sorted_array,
				 size_t ind_to_move,
				 size_t *last_pos_ind);


/* Fre (findf-regex) pattern parsing related routines. */

/* Allocate memory for a single findf_regex_f object. */
findf_regex_f* intern__findf__init_regex(void);
/* Release resources of a findf_regex_f object. */
int intern__findf__free_regex(findf_regex_f *to_free);
/* Release resources of an array of findf_regex_f object. */
int intern__findf__free_regarray(findf_regex_f **reg_array,
				 size_t numof_patterns);
/* Initialize the regex parser. */
findf_regex_f** intern__findf__init_parser(char **patterns,
					   size_t numof_patterns);
/* Parse a matching pattern. */
int intern__findf__strip_match(char *pattern,
			       size_t token_ind,
			       findf_regex_f *freg_object);
/* Parse a substitution or transliteration pattern. */
int intern__findf__strip_substitute(char *pattern,
				    size_t token_ind,
				    findf_regex_f *freg_object);
/* Check for unsupported escape sequences. */
int intern__findf__validate_esc_seq(char token,
				    char *buffer,
				    size_t *buf_ind,
				    size_t *buf_len);
/* Validate pattern's modifier(s). */
int intern__findf__validate_modif(char *modifiers,
				  findf_regex_f *freg_object);
/* Skip extended pattern's comments. */
void intern__findf__skip_comments(char *pattern,
				  size_t *cur_ind,
				  size_t *pattern_len);
/* Convert Perl-like syntax into POSIX ere syntax. */
int intern__findf__perl_to_posix(char *pattern,
				 findf_regex_f *freg_object);
/* Compile a POSIX regex pattern. */
int intern__findf__compile_pattern(findf_regex_f *freg_object);


/* Fre pattern operations related routines. */

/* Execute a pattern match operation. */
int intern__findf__match_op(struct fregex *freg_object,
			    char *filename);
/* Execute a substitution operation. */

#endif /* FINDF_PRIVATE_HEADER */
