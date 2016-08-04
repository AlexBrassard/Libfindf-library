/*
 *
 *
 * Libfindf.so  -  Advanced search function.
 *                 Use with caution.
 *                 Libfindf does NOT verify whether both function pointers
 *                 call an actualy valid routine, this is up to the user of findf_adv().
 *                 
 *                 By default this routine is not compiled at library's installation
 *                 but a system administrator may set the -DCW_FINDF_ADVANCED GCC's 
 *                 command line argument to get access to this routine and use
 *                 custom search/sort algorithms while keeping the inner workings of Libfindf.
 *
 */
#include <errno.h>

#include "libfindf_private.h"
#include <findf.h>


#ifdef CW_FINDF_ADVANCED /* "compile with findf_adv()" */

int findf_adv(findf_param_t *sparam,
	      void *(*algorithm)(void *),
	      void *(*sort_f)(void *),
	      void *algorithm_arg,
	      void *sort_f_arg)
{
  /* Very quick parameter check, more checks next. */

  if (sparam != NULL) {
    ;
  }
  else {
    errno = EINVAL;
    return ERROR;
  }
  
  /* 
   * Make sure the findf_param_t parameter was built using
   * the right flags. 
   * Calling findf_adv() with a search_type other than CUSTOM
   * is equivalent to a (valid) call to findf_fg().
   * A ->sort_type field that is other than C_SORT (custom sort) or
   * NONE (no sort) is equivalent to setting the field to SORTP.
   * If any of the argument parameters (arg, sarg) are NULL,
   * Libfindf will plug in the caller's search parameter object.
   */
  
  /* Search algorithm. */
  if (sparam->search_type == CUSTOM){
    if (algorithm != NULL
	&& *algorithm != 0) sparam->algorithm = algorithm;
    else {
      sparam->algorithm = intern__findf__BF_search; /* Default to BFS type of search. */
      sparam->search_type = BFS; /* Adjust the flag to keep the parameter's info straight. */
    }

    if (algorithm_arg != NULL) sparam->arg = algorithm_arg;
    else sparam->arg = NULL;
  }
  /* 
   * Else, the parameter was created with a search type of BFS, DFS, IDDFS, IDBFS
   * and the appropriate pointer has been stored in the parameter already.
   *
   * OK now remember that users are supposed to init ther parameters with the library's function.
   * DON'T DO DOUBLE CHECKS HERE ..!!
   */

  /* Sort algorithm */
  if (sparam->sort_type == C_SORT){
    if (sort_f != NULL) sparam->sort_f = sort_f;
    else sparam->sort_f = NULL;

    if (sort_f_arg != NULL) sparam->sarg = sort_f_arg;
    /* 
     * _internal() will pass the caller's search parameter object
     * (findf_param_t *) casted to (void *) to the callers custom
     * sort routine, if the custom sort argument is set to NULL.
     */
    else sparam->sarg = NULL;
  }
  /* 
   * Else, the parameter was created with a sort type of NONE or SORTP
   * and intern__findf__internal() will know what to do.
   */

  
  /* Execute the search. */
  if (intern__findf__internal(sparam) != 0){
    findf_perror("Failed to execute the search.");
    return -1;
  }

  return 0;  
  
} /* findf_adv() */

#endif /* CW_FINDF_ADVANCED */
