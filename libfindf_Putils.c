/*
 *
 *
 * Libfindf.so  -  Public utilities source file.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <findf.h>              /* Public header file. */
#include "libfindf_private.h"   /* Private header file. */


/* Initialize a findf_param_f object. */
findf_param_f *findf_init_param(char **file2find,
				char **search_roots,
				size_t numof_file2find,
				size_t numof_search_roots,
				unsigned int max_inc_dept,
				findf_type_f search_type,
				findf_sort_type_f sort_type)
{
  bool NO_ROOT = false; /* True: caller did not give at least 1 root directory. */
  char **def_root_array = NULL; /* Array of 1 pathname, DEF_UNIX_ROOT. */
  size_t i = 0;
  size_t f2f_c = 0;
  findf_param_f *to_init = NULL;

  if ((numof_file2find < SIZE_MAX - 1
       && max_inc_dept < UINT_MAX - 1)  /* Can be 0, meaning infinity. */
      && (numof_search_roots < SIZE_MAX - 1)
      && (sort_type == NONE  /* Only verify validity here, _internal() knows how to handle the field. */
	  || sort_type == C_SORT
	  || sort_type == SORTP)){
    ; /* Valid arguments, do nothing. */
  }
  else { /* A parameter check did not succeed. */
    errno = EINVAL;
    return NULL;
  }
  /* If no pathnames are given, the system's root directory is used. */
  if (numof_search_roots == 0
      || search_roots == NULL
      || search_roots[0] == NULL
      || search_roots[0][0] == '\0'){
    numof_search_roots = 1;
    NO_ROOT = true;
    if ((def_root_array = calloc(1, sizeof(char*))) == NULL){
      findf_perror("Calloc failure");
      return NULL;
    }
    if ((def_root_array[0] = calloc(DEF_UNIX_ROOT_LEN, sizeof(char))) == NULL){
      findf_perror("Calloc failure");
      goto cleanup;
    }
    if (SU_strcpy(def_root_array[0], DEF_UNIX_ROOT, DEF_UNIX_ROOT_LEN) == NULL){
      findf_perror("SU_strcpy failure");
      goto cleanup;
    }
      
  }
  /* 
   * Verify that all search_roots, are absolute pathnames.
   * If a pathname does NOT begin with a slash '/', 
   * explain why we're not going further, return an error.
   */
  else {
    for (i = 0; i < numof_search_roots; i++){
      if (search_roots[i] != NULL){
	if (search_roots[i][0] != '/'){
	  errno = ENOENT;
	  pthread_mutex_lock(&stderr_mutex);
	  fprintf(stderr, "The Libfindf library does not support relative pathname(s):\n[%s]\n",
		  search_roots[i]);
	  pthread_mutex_unlock(&stderr_mutex);
	  goto cleanup;
	}
      }
      /* 
       * The caller gave a numof_search_roots that's bigger than the
       * actual amount of pathnames in the array.
       * Adjust the numof_search_roots to the appropriate amount and leave this loop.
       */
      else {
	numof_search_roots = i;
	break;
      }
    }
  }
  /* findf_re requires to let NULL slip in. */
  if (file2find){
    for (i = 0; i < numof_file2find; i++){
      if (file2find[i] == NULL && f2f_c == 0){
	errno = ENODATA;
	pthread_mutex_lock(&stderr_mutex);
	fprintf(stderr, "The Libfindf library needs at least 1 filename to search for.\n\n");
	pthread_mutex_unlock(&stderr_mutex);
	goto cleanup;
      }
      /* 
       * The caller gave a numof_file2find that's bigger that
       * the actual amount of filenames in file2find.
       */
      else if(file2find[i] == NULL && f2f_c > 0) {
	numof_file2find = f2f_c;
	break;
      }
      else /* Valid */ 
	f2f_c++;
    }
  }
  /* Caller wants a BFS/IDBFS type of search. */
  if (search_type == BFS 
      || search_type == IDBFS){
    if ((to_init = intern__findf__init_param(file2find, 
					     NO_ROOT == false ? search_roots : def_root_array,
					     numof_file2find, 
					     numof_search_roots,
					     max_inc_dept, 
					     search_type,
					     sort_type,
					     NULL, /* Using library's _sortp() algorithm. */
					     NULL,
					     intern__findf__BF_search, 
					     NULL)) == NULL)
      goto cleanup;
  }
  /* Caller wants a DFS/IDDFS type of search. */
  else if (search_type == DFS
	   || search_type == IDDFS){
    if ((to_init = intern__findf__init_param(file2find,
					     NO_ROOT == false ? search_roots : def_root_array,
					     numof_file2find, 
					     numof_search_roots,
					     max_inc_dept, 
					     search_type,
					     sort_type,
					     NULL, /* Using library's _sortp() algorithm. */
					     NULL,
					     intern__findf__DF_search, 
					     NULL)) == NULL)
      goto cleanup;
  }
  /* Caller has a custom algorithm. */
  else if (search_type == CUSTOM){
    if ((to_init = intern__findf__init_param(file2find,
					     NO_ROOT == false ? search_roots : def_root_array,
					     numof_file2find, 
					     numof_search_roots,
					     max_inc_dept, 
					     search_type,
					     sort_type,
					     NULL,
					     NULL,  /* Set by findf_adv(), if needed. */
					     NULL, /* Set by findf_adv(), if needed. */
					     NULL)) == NULL)
      goto cleanup;
  }
  else {
    /* Invalid search type. */
    errno = EINVAL;
    goto cleanup;
  }

  if (NO_ROOT == true){
    free(def_root_array[0]);
    def_root_array[0] = NULL;
    free(def_root_array);
    def_root_array = NULL;
  }
  return to_init;

 cleanup:
  if (def_root_array){
    if (def_root_array[0]){
      free(def_root_array[0]);
      def_root_array[0] = NULL;
    }
    free(def_root_array);
    def_root_array = NULL;
  }
  return NULL;
}
/* Destroy a findf_param_f object. */
int findf_destroy_param(findf_param_f *to_free)
{
  if (to_free == NULL){
    /* Warn user by setting errno to EINVAL but return with success. */
    errno = ENODATA;
    return RF_OPSUCC;
    
  }
  else
    if (intern__findf__free_param(to_free) != RF_OPSUCC)
      return ERROR;

  return RF_OPSUCC;
}


/* Destroy a findf_results_f object. */
int findf_destroy_results(findf_results_f *to_free)
{
  if (to_free == NULL){
    /* Warn the user by setting errno but return with success. */
    errno = ENODATA;
    return RF_OPSUCC;
  }
  intern__findf__free_res(to_free);

  return RF_OPSUCC;

}


/* Print the content of a findf_results_f object to stdout. */
int findf_read_results(findf_results_f *to_read)
{
  size_t i = 0;

  if (to_read != NULL){

    for (i = 0; i < to_read->numof_elements
	   && to_read->res_buf[i][0] != '\0'; i++)
      fprintf(stdout, "Result[%zu] is at [%s]\n",
	      i, to_read->res_buf[i]);
    return RF_OPSUCC;
  }
  else {
    errno = EINVAL;
    return ERROR;
  }
}
