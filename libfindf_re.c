/*
 *
 *
 *  Libfindf.so  -  Finer-grained search using regex pattern(s).
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#include <findf.h>
#include "libfindf_private.h"

int findf_re(findf_param_f *sparam,
	     char **patterns,
	     size_t numof_patterns)
{
  size_t i = 0;
  findf_regex_f **reg_array = NULL;


  if (sparam == NULL){
    errno = EINVAL;
    return ERROR;
  }

  intern__findf__verify_search_type(sparam);

  if (patterns == NULL){
    /* We're just a findf_fg() clone then. */
    if (sparam->file2find == NULL){
      errno = EINVAL;
      return ERROR;
    }
    if (intern__findf__internal(sparam) != RF_OPSUCC){
      findf_perror("Failed to execute the search");
      return ERROR;
    }
    return RF_OPSUCC;
  }
   /* 
    * Ignore any patterns if numof_pattern is 0. 
    * Execute the search findf_fg style. 
    */
  if (numof_patterns == 0){
    if (sparam->file2find == NULL){
      errno = EINVAL;
      return ERROR;
    }
    else{
      if (intern__findf__internal(sparam) != RF_OPSUCC){
	findf_perror("Failed to execute the search");
	return ERROR;
      }
      return RF_OPSUCC;
    }
  }
  else if (numof_patterns > FINDF_MAX_PATTERNS){
    /* 
     * Trim down the ambitions of our caller. 
     * Be nice and explain why.. 
     */
    numof_patterns = FINDF_MAX_PATTERNS;
    pthread_mutex_lock(&stderr_mutex);
    fprintf(stderr, "%s - Too many patterns.\nUsing only the first %zu\n\n", __func__, numof_patterns);
    pthread_mutex_unlock(&stderr_mutex);
  }

  if ((sparam->reg_array = intern__findf__init_parser(patterns, numof_patterns)) == NULL){
    intern_errormesg("Intern__findf__init_parser failure");
    return ERROR;
  }
  sparam->sizeof_reg_array = numof_patterns;
  if (intern__findf__internal(sparam) != RF_OPSUCC){
    intern_errormesg("Failed to execute the search");
    return ERROR;
  }
  return RF_OPSUCC;
}
