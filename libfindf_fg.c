/*
 *
 *
 * Libfindf.so  -  Public finer-grained search routine. 
 *
 *
 */

#include <errno.h>

#include "libfindf_private.h"
#include <findf.h>


int findf_fg(findf_param_f *search_param)
{
  /*  findf_results_f *results = NULL;*/
  
  if (search_param == NULL){
    errno = EINVAL;
    return ERROR;
  }
  /* 
   * Verify that the search type set by findf_init_param() is
   * not CUSTOM, if it is, default to BFS.
   */
  if (search_param->search_type == BFS
      || search_param->search_type == DFS
      || search_param->search_type == IDDFS
      || search_param->search_type == IDBFS) {
    ; /* Valid */
  }
  else {
    search_param->search_type = BFS;
    search_param->algorithm = intern__findf__BF_search;
  }

  
  /* Execute the search. */
  if (intern__findf__internal(search_param) != RF_OPSUCC){
    findf_perror("Failed to execute search.");
    return ERROR;
  }
  
  return RF_OPSUCC;
}
