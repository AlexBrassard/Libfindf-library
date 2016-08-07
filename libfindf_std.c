/*
 *
 *
 * Libfindf.so  -  Public standard findf search routine.
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>

#include "libfindf_private.h"
#include <findf.h>



/* System wide quick search. */
findf_results_f *findf(char *file2find,
		       size_t file2find_len,
		       bool IS_BUF)
{
  unsigned int i = 0;
  char **temp = NULL;
  char **rtemp = NULL;
  findf_param_f *findf_param = NULL;
  findf_results_f *callers_results = NULL;

  /* The only way to verify findf() has failed is to check errno. */
  errno = 0;

  if (file2find == NULL || file2find[0] == '\0'){
    errno = EINVAL;
    return NULL;
  }
  if (file2find_len > 0 && file2find_len < F_MAXNAMELEN -1){
    ;
  }
  else{
    errno = ENAMETOOLONG;
    return NULL;
  }
  /* 
   * Create a temporary array of string, of filenames to search.
   * Users of findf() are allowed to input only 1 filename. 
   */
  if ((temp = calloc(1, sizeof(char *))) == NULL){
    findf_perror("Calloc failure.");
    return NULL;
  }
  if ((temp[0] = calloc(F_MAXNAMELEN, sizeof(char))) == NULL){
    findf_perror("Calloc failure.");
    return NULL;
  }
  if (SU_strcpy(temp[0], file2find, F_MAXNAMELEN) == NULL){
    findf_perror("SU_strcpy failure.");
    return NULL;
  }

  /* 
   * Create a temporary array of string, search roots pathnames. 
   * Users of findf() are allowed system-wide searches only. 
   */
  if ((rtemp = calloc(1, sizeof(char *))) == NULL){
    findf_perror("Calloc failure.");
    return NULL;
  }
  if ((rtemp[0] = calloc(F_MAXNAMELEN, sizeof(char))) == NULL){
    findf_perror("Calloc failure.");
    return NULL;
  }
  if (SU_strcpy(rtemp[0], DEF_UNIX_ROOT, F_MAXNAMELEN) == NULL){
    findf_perror("SU_strcpy failure.");
    return NULL;
  }

  /* intern__findf__internal() needs a parameter object. */
  if ((findf_param = intern__findf__init_param(temp,
					       rtemp,
					       1,    /* Number of file to find. */
					       1,    /* Number of search root.  */
					       0,    /* Till the end of time.   */
					       BFS,  /* BFS-like search algorithm (built-in). */
					       SORTP,/* Built-in sorting algorithm. */
					       NULL, /* Using _sortp(). */
					       NULL, /* Using findf_param_f object's results list. */
					       intern__findf__BF_search, 
					       NULL)) == NULL) { /* Using a findf_param_f object. */
    findf_perror("Failed to initialize a search parameter object.");
    return NULL;
  }
  /* We don't need these anymore. */
  free(temp[0]);
  temp[0] = NULL;
  free(temp);
  temp = NULL;
  free(rtemp[0]);
  rtemp[0] = NULL;
  free(rtemp);
  rtemp = NULL;

  /* Execute the search. */
  if (intern__findf__internal(findf_param) != RF_OPSUCC){
    findf_perror("Failed to execute search.");
    if (intern__findf__free_param(findf_param) != RF_OPSUCC){
      findf_perror("Failed to release a parameter object.");
      return NULL;
    }
    return NULL;
  }


  /* Caller wants results packed into a buffer. */
  if (IS_BUF == true){
    if ((callers_results = intern__findf__init_res(findf_param->search_results->position,
						   findf_param->search_results->pathlist)) == NULL){
      findf_perror("Failed to initialize a results buffer.");
      return NULL;
    }
    if (intern__findf__free_param(findf_param) != RF_OPSUCC) {
      findf_perror("Failed to release a parameter object.");
      return NULL;
    }
    return callers_results;
  }
  /* Caller wants results printed to stdout. */
  else {
    for (i = 0; i < findf_param->search_results->position; i++)
      printf("Result[%d] is at [%s]\n", i, findf_param->search_results->pathlist[i]);
    if (intern__findf__free_param(findf_param) != RF_OPSUCC)
      findf_perror("Failed to release a parameter object.");
    return NULL;
  }
}
