/*
 *
 *
 * Libfindf.so  -  Main internal routines and search algorithms.
 * Version:        0.1
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include <findf.h>
#include "libfindf_private.h"

/* To synchronize results from all deployed threads. */
extern findf_list_f *temporary_container;

int intern__findf__internal(findf_param_f *callers_param)
{
  size_t        thread_param_pos = 0;      /* To loop through threads parameters. */
  size_t        search_roots_c = 0;        /* To loop through callers_param->search_roots. */
  size_t        i = 0;                     /* Convinient loop counter. */
  size_t        intheadnode_c = 0;         /* Loop through internal_headnode. */
  unsigned int  level = 0;                 /* Our location in the filesystem. */
  findf_list_f  *internal_headnode = NULL; /* Temp list, when gathering work for threads. */
  findf_list_f  *internal_nextnode = NULL; /* Pointer to internal_headnode->next. */
  findf_tpool_f *thread_pool = NULL;       /* Pool of Pthread thread objects. */
  findf_param_f **threads_params = NULL;   /* Array of parameter objects, 1 per thread(s) */
  void          *thread_retval = NULL;     /* Thread return value. */
  size_t         numof_threads = 0;        /* Maximum allowed number of threads for our process. */
  bool           ERR = false;              /* True when there's an error; */
  /* Goto label advance_to_cleanup;           In case the search terminates earlier. */

#ifdef DEBUG
  size_t  c = 0;
#endif /* DEBUG */


  /* Initialization. */

  if ((internal_headnode = intern__findf__init_node(DEF_LIST_SIZE, level++, false)) == NULL){
    intern_errormesg("Failed to initialize a new node.\n");
    ERR = true;
    goto advance_to_cleanup;
  }
  
  if ((internal_headnode->next = intern__findf__init_node(DEF_LIST_SIZE, level++, false)) == NULL){
    intern_errormesg("Failed to initialise a new node.\n");
    ERR = true;
    goto advance_to_cleanup;
  }
  /* For ease of use. */
  internal_nextnode = internal_headnode->next;

  numof_threads = intern__findf__get_avail_cpus();
  if ((thread_pool = intern__findf__init_tpool(numof_threads)) == NULL) {
    intern_errormesg("Failed to initialize a thread pool.\n");
    ERR = true;
    goto advance_to_cleanup;
  }
  
  /* 
   * Allocate memory to the array of parameters, then let _init_param handle
   * each elements individualy. 
   */
  if ((threads_params = calloc(numof_threads, sizeof(findf_param_f))) == NULL){
    perror("calloc");
    ERR = true;
    goto advance_to_cleanup;
  }
  for (i = 0; i < thread_pool->num_of_threads ; i++){
    if ((threads_params[i] = intern__findf__init_param(callers_param->file2find,
						       NULL, /* Will be distributed later on. */
						       callers_param->sizeof_file2find,
						       0,
						       callers_param->dept,
						       callers_param->search_type,
						       callers_param->sort_type,
						       callers_param->sort_f,
						       callers_param->sarg,
						       callers_param->algorithm,
						       NULL)) == NULL) {
      intern_errormesg("Failed to initialize a threads_param parameter object.\n");
      ERR = true;
      goto advance_to_cleanup;
    }
    /* Add the caller's pattern array to the parameter object. */
    threads_params[i]->sizeof_reg_array = callers_param->sizeof_reg_array;
    threads_params[i]->reg_array = callers_param->reg_array;
    /*
    if ((threads_params[i]->reg_array = intern__findf__copy_regarray(callers_param->reg_array,
								     callers_param->sizeof_reg_array)) == NULL){
      intern_errormesg("Failed to copy reg_array to a thread's search parameter");
      ERR = true;
      goto advance_to_cleanup;
      }*/
      
  }

  /* Execution. */
  
  /* Check if there's enough root directories to deploy threads now. */
  if (callers_param->search_roots->position < thread_pool->num_of_threads) {
    /* Populate the linked-list's headnode. */
    while (search_roots_c < callers_param->search_roots->position)
      if (intern__findf__add_element(callers_param->search_roots->pathlist[search_roots_c++],
				     internal_headnode) != RF_OPSUCC){
	intern_errormesg("Failed to add element to internal node.\n");
	ERR = true;
	goto advance_to_cleanup;
      }
    
    /* Begin processing the headnode. */
    while (1){
    /* 
     * When there is no more directories in internal_headnode,
     * return control to our caller. 
     */
      if (intheadnode_c >= internal_headnode->position)
	goto advance_to_cleanup;
      /* 
       * If dept is set to 0, it means infinity.
       * Make sure we don't dig deeper than what our caller specified. 
       */
      if (callers_param->dept > 0 && level > callers_param->dept)
	goto advance_to_cleanup;
      
      if (intern__findf__opendir(internal_headnode->pathlist[intheadnode_c++],
				 internal_nextnode,
				 callers_param) != RF_OPSUCC){
	intern_errormesg("Failed to open directory from internal node.\n");
	ERR = true;
	goto advance_to_cleanup;
      }
      
      if (intheadnode_c < internal_headnode->position)
	continue;
      /* If we have enough directories to deploy all available threads, do it. */
      if (internal_nextnode->position >= thread_pool->num_of_threads)
	break;
      /* Else shift the headnode and start searching again. */
      if ((internal_headnode = intern__findf__shift_node(internal_headnode)) == NULL)
	goto advance_to_cleanup;
      else{
	internal_nextnode = internal_headnode->next;
	intheadnode_c = 0;
	continue;
      }
    }
    
    /* Assign each directories from internal_nextnode to each thread parameters. */
    for (i = 0; i < internal_nextnode->position; i++){
      if (thread_param_pos == thread_pool->num_of_threads)
	thread_param_pos = 0;
      if (intern__findf__add_element(internal_nextnode->pathlist[i],
				     threads_params[thread_param_pos]->search_roots) != RF_OPSUCC){
	intern_errormesg("Failed to add a new element to a thread parameter.\n");
	ERR = true;
	goto advance_to_cleanup;
      }
      thread_param_pos++;
    }
  }
  /* 
   * Our caller passed us enough search_roots for each available threads
   * to have at least 1 directory to search.
   */
  else {
    for (i = 0; i < callers_param->search_roots->position; i++){
      if (thread_param_pos == thread_pool->num_of_threads)
	thread_param_pos = 0;
      if (intern__findf__add_element(callers_param->search_roots->pathlist[i],
				     threads_params[thread_param_pos]->search_roots) != RF_OPSUCC){
	intern_errormesg("Failed to add a new element to a thread parameter.\n");
	ERR = true;
	goto advance_to_cleanup;
      }
      thread_param_pos++;
    }
  }

#ifdef DEBUG
  pthread_mutex_lock(&stderr_mutex);
  fprintf(stderr,"Deploying threads here !\n\n");
  for(i = 0; i < thread_pool->num_of_threads; i++)
    for(c = 0; c < threads_params[i]->search_roots->position; c++)
      fprintf(stderr,"threads_param[%zu]->search_roots[%zu]: %s\n", 
	      i, c,threads_params[i]->search_roots->pathlist[c]);
  pthread_mutex_unlock(&stderr_mutex);
#endif /* DEBUG */

  /* 
   * Send each of our threads through the caller's algorithm routine,
   * passing each threads a findf_param_f parameter.
   * If the search type is CUSTOM and a non NULL void* ->arg has been fed,
   * use it, else if either conditions is not met, use the library's built-ins.
   */
  for (i = 0; i < numof_threads; i++)
    if (pthread_create(&(thread_pool->threads[i]), NULL,
		       callers_param->algorithm, (callers_param->search_type == CUSTOM 
						  && callers_param->arg != NULL) 
		       ? ((void*)callers_param->arg)
		       : ((void*)threads_params[i])) != 0){
		       
      intern_errormesg("Failed to create a new thread\n");
      ERR = true;
      goto advance_to_cleanup;
    }
  
  for (i = 0; i < numof_threads; i++)
    if ((pthread_join(thread_pool->threads[i], &thread_retval) != 0)
	|| ((unsigned long int)thread_retval != 1)){
      intern_errormesg("Failed to join a thread\n");
      ERR = true;
      goto advance_to_cleanup;
    }
  
 
  /* 
   * Once thread have all joined, lock temporary_container
   * and copy its content into callers_param.
   */
  pthread_mutex_lock(temporary_container->list_lock);
  for (i = 0; i < temporary_container->position; i++)
    if (intern__findf__add_element(temporary_container->pathlist[i],
			     callers_param->search_results) != RF_OPSUCC){
      intern_errormesg("Failed to add a new element to the caller's parameter object.\n");
      pthread_mutex_unlock(temporary_container->list_lock);
      ERR = true;
      goto advance_to_cleanup;
    }
  pthread_mutex_unlock(temporary_container->list_lock);
  /* Don't bother checking the sort type if there's nothing to sort. */
  if (callers_param->search_results->position > 1
      || callers_param->sort_type == C_SORT){
    /* 
     * Switch on the caller's->sort_type, see whether 
     * we need to sort results before returning them or not. 
     */
    switch(callers_param->sort_type){
    case NONE:
      break; /* Return results unsorted. */
      
    case C_SORT:
      /* 
       * We're not checking if a custom sort routine succeeded or not. 
       * findf_adv() already verified that the function pointer
       * and its arguments are non-null.
       */
      if (callers_param->sort_f != NULL){
	if (callers_param->sarg != NULL)
	  callers_param->sort_f(callers_param->sarg);
	/* 
	 * If the ->sort_type is C_SORT, 
	 * the ->sort_f function pointer is non-NULL but
	 * the caller has not provided us with a custom (void*)arg for
	 * the custom sorting routine, try to call the custom sorting routine 
	 * passing it a findf_param_f pointer object casted to void *.
	 * Note that users should not rely on this behaviour since theres no way 
	 * of determining whether the sorting routine has succeed or not.
	 */
	else
	  callers_param->sort_f((void*)callers_param);
	break;
      }
      /* 
       * Else, the flag is up but the caller did not provide us with a non-NULL
       * sort function pointer, use sortp().
       */
    case SORTP: /* intern__findf__sortp() is the default behaviour. */   
    default:
      if (callers_param->sizeof_file2find > 0 /* TEST ONLY */
	  && callers_param->sizeof_reg_array > 0){
	/* REMEMBER TO CHECK THE RETURN VALUE ONCE COMPLETED. */
	callers_param->reg_array = intern__findf__init_fre_keys(callers_param->reg_array,
								callers_param->file2find,
								&(callers_param->sizeof_reg_array),
								callers_param->sizeof_file2find);
	break;
      }
      if (intern__findf__sortp(callers_param->search_results->pathlist,
			       callers_param->file2find,
			       callers_param->search_results->position,
			       callers_param->sizeof_file2find) != RF_OPSUCC){
	intern_errormesg("Failed to sort callers_param's results.");
	ERR = true;
	goto advance_to_cleanup;
      }
      break;
    }
  }
  

  /* Termination. */
 advance_to_cleanup:
  if (internal_headnode != NULL)
    if (intern__findf__destroy_list(internal_headnode) != RF_OPSUCC){
      intern_errormesg("Failed to destroy internal linked-list.\n");
      return ERROR;
    }
  internal_headnode = NULL;
  internal_nextnode = NULL;
  if (threads_params){
    for (i = 0; i < thread_pool->num_of_threads; i++){
      if (threads_params[i]){
	/* NULL out copies of the callers_param->reg_array we've made earlier. */
	threads_params[i]->reg_array = NULL;
	if (intern__findf__free_param(threads_params[i]) != RF_OPSUCC){
	  intern_errormesg("Failed to release an internal parameter.\n");
	  /* Don't return an error, break out the loop and continue freeing. */
	  break;
	}
	threads_params[i] = NULL;
      }
    }
    free(threads_params);
    threads_params = NULL;
  }
  if (thread_pool){
    intern__findf__free_tpool(thread_pool);
    thread_pool = NULL;
  }
  if (ERR == false) return RF_OPSUCC;
  else return ERROR;
}


void *intern__findf__BF_search(void *param)
{
  size_t       i = 0;                            /* Convinient loop counter. */
  unsigned int level = 0;                        /* Current level of the search. 0 being search_roots. */
  findf_list_f *headnode = NULL;                 /* Headnode of BF_search's linked-list. */
  findf_list_f *nextnode = NULL;                    
  findf_param_f *t_param = NULL;                 /* Search parameter given by our caller. */
  /* goto label   bfs_err_jmp;                   Cleanup on exit. */

  if (param == NULL){
    intern_errormesg("Invalid or empty search parameter.\n");
    goto bfs_err_jmp;
  }
#ifdef DEBUG 
  pthread_mutex_lock(&stderr_mutex);
  fprintf(stderr, "\nBFS Start\n\n");
  pthread_mutex_unlock(&stderr_mutex);
#endif /* DEBUG */

  /* Get back our original parameter. */
  t_param = (findf_param_f *)param;

  /* We need a linked-list. */
  if ((headnode = intern__findf__init_node(DEF_LIST_SIZE,level++, false)) == NULL) {
    intern_errormesg("Failed to initialize BF search's linked-list.\n");
    goto bfs_err_jmp;
  }
  
  if ((headnode->next = intern__findf__init_node(DEF_LIST_SIZE,level++, false)) == NULL){
    intern_errormesg("Failed to initialize BF search's linked-list.\n");
    goto bfs_err_jmp;
  }
  nextnode = headnode->next;

  /* Populate the headnode. */
  for(i = 0; i < t_param->search_roots->position; i++)
    if (intern__findf__opendir(t_param->search_roots->pathlist[i],
			 headnode,
			 t_param) != RF_OPSUCC){
      intern_errormesg("Failed to open a node's directory.\n");
      goto bfs_err_jmp;
    }
  
  /* Let the search begin. */
  while(1){
    if (headnode != NULL && headnode->position > 0){
      i = 0;
      /* Break when we reach the caller's limit dept, if any. */
      if (t_param->dept > 0 && headnode->list_level > t_param->dept){
	/*
	 * IDBFS is just like a proper IDDFS search except that it descend
	 * the filesystem in breah-first mode.
	 * If there are results, return them right away,
	 * else we act like we're begining a new search. 
	 */
	if (t_param->search_type == IDBFS){
	  if (t_param->search_results->position > 0)
	    break; 
	  else{
	    level = 0;
	    headnode->list_level = level++;
	    nextnode->list_level = level++;
	   continue; /* Search the headnode. */
	  }
	}
	/* Else we're in BFS mode but a dept limit was set. */
	break;
      }
      
      while(i < headnode->position)	
	if (intern__findf__opendir(headnode->pathlist[i++], nextnode, t_param) != RF_OPSUCC) {
	  intern_errormesg("Failed to open a node's directory.\n");
	  goto bfs_err_jmp;
	}

      /* headnode is completed, shift it out and create a new nextnode */
      if((headnode = intern__findf__shift_node(headnode)) == NULL ){
	intern_errormesg("Failed to shift a list's headnode.\n");
	goto bfs_err_jmp;
      }
      nextnode = headnode->next;
      continue;
    }
    else /* headnode == NULL */
      break;
  }

  /* 
   * When we finaly break out of the above loop, 
   * check if there are results and copy them to the global
   * list temporary_container. 
   */
  i = 0;
  pthread_mutex_lock(temporary_container->list_lock);
  while (i < t_param->search_results->position)
    if (intern__findf__add_element(t_param->search_results->pathlist[i++],
			    temporary_container) != RF_OPSUCC){
      intern_errormesg("Failed to add a new element to the global temporary list.\n");
      pthread_mutex_unlock(temporary_container->list_lock);
      goto bfs_err_jmp;
    }
  pthread_mutex_unlock(temporary_container->list_lock);

  /* Destroy our linked list. */
  if (intern__findf__destroy_list(headnode) != RF_OPSUCC){
    intern_errormesg("Failed to destroy BF search's linked-list.\n");
    goto bfs_err_jmp;
  }

#ifdef DEBUG
  pthread_mutex_lock(&stderr_mutex);
  fprintf(stderr, "\nThread [%zu]\nWork completed, joining with main Thread.\n\n",
	  pthread_self());
  pthread_mutex_unlock(&stderr_mutex);
#endif /* DEBUG */

  /* Success!  Returning 1 since NULL == ((void *)0); */
  pthread_exit(((void*)1));

  
 bfs_err_jmp:
  if (headnode) {
    if (intern__findf__destroy_list(headnode) != RF_OPSUCC){
      intern_errormesg("Failed to destroy BF search's linked-list.\n");
    }
  }
  pthread_exit(NULL);

} /* intern__findf__BF_search() */



void *intern__findf__DF_search(void *param)
{
  size_t             i = 0;                                /* Convinient loop counter. */
  unsigned int       level = 0;                            /* Our position in the filetree. */
  size_t             search_roots_c = 0;                   /* Count of current dir in search_roots. */
  size_t             head_root_c = 0;                      /* Count of current dir in headnode. */
  findf_list_f       *headnode = NULL;                     /* Linked-list's headnode. */
  findf_list_f       *nextnode = NULL;                     /* Linked-list's next node. */
  findf_param_f      *t_param = NULL;                      /* Caller's parameter object. */
  /* goto label      dfs_err_jmp;                             Cleanup on early exit. */

  
  if (param == NULL){
    intern_errormesg("Invalid or empty search parameters\n");
    goto dfs_err_jmp;
  }

  pthread_mutex_lock(&stderr_mutex);
  fprintf(stderr, "\nDFS Start\n\n");
  pthread_mutex_unlock(&stderr_mutex);
  /* Get back our findf parameter. */
  t_param = ((findf_param_f *)param);
  
  /* Initialize DF_search linked-list's head and next nodes. */
  if ((headnode = intern__findf__init_node(DEF_LIST_SIZE,level++, false)) == NULL){
    intern_errormesg("Failed to initialize a findf_list_f node.\n");
    goto dfs_err_jmp;
  }

  if ((headnode->next = intern__findf__init_node(DEF_LIST_SIZE,level++, false)) == NULL){
    intern_errormesg("Failed to initialize a findf_list_f node.\n");
    goto dfs_err_jmp;
  }
  nextnode = headnode->next;

  /* Begin processing the search_roots. */
  while (search_roots_c < t_param->search_roots->position){
    if (intern__findf__opendir(t_param->search_roots->pathlist[search_roots_c++],
			       headnode,
			       t_param) != RF_OPSUCC){
      intern_errormesg("Failed to open a 'root of search' directory.\n");
      goto dfs_err_jmp;
    }
    /* Reset this counter every time we open a new search_roots. */
    head_root_c = 0;

    while(1){
      /* 
       * Make sure we're not past the maximum inclusive dept given
       * by our caller (if any).
       * If we are, break and get a new search_roots.
       */
      if ((t_param->dept > 0)
	  && (headnode->list_level > t_param->dept)){
	if (t_param->search_type == IDDFS){
	  if (t_param->search_results->position > 0)
	    /* We found at least one result, return control to our caller. */
	    break;
	  else{
	    /* Keep searching headnode till we reach t_param->dept again. */
	    level = 0;
	    headnode->list_level = level++;
	    nextnode->list_level = level++;
	    continue;
	  }
	}
	/* 
	 * We're in DFS mode but a dept limit is set and
	 * we just reached it.
	 * Try and get another search_roots. 
	 */
	else
	  break;
      }
      /* Are we finished searching this headnode? */
      if (head_root_c == headnode->position){
	/* Anything in our next node? */
	if (nextnode->position > 0){
	  if ((headnode = intern__findf__shift_node(headnode)) == NULL) {
	    intern_errormesg("Failed to shift DF_search's headnode. \n");
	    goto dfs_err_jmp;
	  }
	  /* Init a new nextnode. */
	  nextnode = headnode->next;
	  /* We're begining a new node. */
	  head_root_c = 0;
	  continue;
	}
	else{
	  /* The headnode is finished but nothing was inside the nextnode. */
	  for (i = 0; i < headnode->size; i++){
	    memset(headnode->pathlist[i], 0, F_MAXPATHLEN);
	    headnode->position = 0;
	    /* Reset level to zero, then adjust the level of the linked-list's nodes. */
	    level = 0;
	    headnode->list_level =level++;
	    nextnode->list_level =level++;
	  }
	  /* Get another search_roots. */
	  break;
	}
      }
      /* There are still some directories to open in the headnode. */
      else {
	if (intern__findf__opendir(headnode->pathlist[head_root_c],
				   nextnode,
				   t_param) != RF_OPSUCC) {
	  intern_errormesg("Failed to open a DF_search node's directory.\n");
	  goto dfs_err_jmp;
	}
	head_root_c++;
	/* Keep searching the node. */
	continue;
      }
    }
  }
  
  /* If we found anything, copy it to the global temporary list. */
  pthread_mutex_lock(temporary_container->list_lock);
  for (i = 0; i < t_param->search_results->position; i++)
    if (intern__findf__add_element(t_param->search_results->pathlist[i],
				   temporary_container) != RF_OPSUCC){
      intern_errormesg("Failed to add element to the global temporary list.\n");
      goto dfs_err_jmp;
    }
  pthread_mutex_unlock(temporary_container->list_lock);

#ifdef DEBUG
  intern_errormesg("Work completed, joining with main thread");
#endif /* DEBUG */
  
  /* Destroy our linked-list. */
  if (intern__findf__destroy_list(headnode) != RF_OPSUCC){
    intern_errormesg("Failed to destroy DFS's linked-list.\n");
    goto dfs_err_jmp;
  }
  
  pthread_exit((void *) 1); /* Success ! */


 dfs_err_jmp:
  if (headnode){
    if (intern__findf__destroy_list(headnode) != RF_OPSUCC){
      intern_errormesg("Failed to destroy DFS's linked-list.");
    }
  }
  pthread_exit(NULL);
}
