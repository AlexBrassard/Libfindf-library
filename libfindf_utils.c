/*
 *
 *
 * Libfindf.so  -  Private Utilities.
 *
 *
 */


#include <stdio.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <string.h> 
#include <sys/types.h> 
#include <dirent.h> 
#include <sys/stat.h> 
#include <errno.h> 
#include <limits.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <stdint.h> 

#include <findf.h>
#include "libfindf_private.h"



/* 
 * Getting an error when defining 'entry' MACRO from anywhere else than 
 * from within this file. 
 */

/* 
 * In the hope of saving a couple calls to malloc(): 
 * (Couple being aprox. 30 millions calls on a fresh Debian install.)
 */
#define entry (intern__findf_entry_location())

/* List operations: */

/* Initialize a single findf_list_f node. */
findf_list_f *intern__findf__init_node(size_t size,
				       size_t list_level_s,
				       bool ISGLOBAL)
{
  findf_list_f *to_init = NULL;
  char *error_mesg = NULL;
  size_t tempsize = (size > 0 && size > DEF_LIST_SIZE) ? size : DEF_LIST_SIZE;
  size_t i = 0;
  /* goto label  init_node_err;        Clean-up on error */
  
  /* Init the findf_list_f object. */
  if ((to_init = malloc(sizeof(findf_list_f))) == NULL){
    error_mesg = "Malloc";
    goto init_node_err;
  }
  
  /* Init the list's array of strings. */
  if ((to_init->pathlist = calloc(tempsize, sizeof(char*))) == NULL){
    error_mesg = "Calloc";
    goto init_node_err;
  }
  
  for (i = 0; i < tempsize; i++)
    if ((to_init->pathlist[i] = calloc(F_MAXPATHLEN, sizeof(char))) == NULL){
      error_mesg = "Calloc";
      goto init_node_err;
    }
  
  /* 
   * Look at the ISGLOBAL flag, if true initialize the list's mutex.
   * Else set it to NULL. 
   */
  if (ISGLOBAL == true){
    if ((to_init->list_lock = malloc(sizeof(pthread_mutex_t))) == NULL){
      error_mesg = "Malloc";
      goto init_node_err;
    }
    if (pthread_mutex_init(to_init->list_lock, NULL) != 0){
      error_mesg = "Pthread_mutex_init";
      goto init_node_err;
    }
  }
  else
    to_init->list_lock = NULL;
  
  /* Initialize remaining fields. */
  to_init->size = tempsize;
  to_init->position = 0;
  to_init->list_level = list_level_s;
  to_init->next = NULL;
 
  return to_init;

 init_node_err:
  if (to_init){
    if (to_init->pathlist){
      for (i = 0; i < tempsize; i++){
	if (to_init->pathlist[i]){
	  free(to_init->pathlist[i]);
	}
      }
      free(to_init->pathlist);
    }
    free(to_init);
  }
  perror(error_mesg);
  return NULL;
} /* intern__findf__init_node() */



int intern__findf__free_node(findf_list_f *to_free)
{
  size_t i = 0;
  if (to_free == NULL){
    errno = EINVAL;
    return ERROR;
  }

  /* Free the pathlist array of strings. */
  for (i = 0; i < to_free->size; i++){
    free(to_free->pathlist[i]);
    to_free->pathlist[i] = NULL;
  }
  free(to_free->pathlist);
  /* Check if we had a Pthread mutex initialized. */
  if (to_free->list_lock != NULL){
    if (pthread_mutex_destroy(to_free->list_lock) != 0){
      perror("pthread_mutex_destroy");
      /* Don't return an error, try and free what's left. */
    }
    free(to_free->list_lock);
    to_free->list_lock = NULL;
  }
  to_free->next = NULL;
  
  /* Release resource of the findf_list_f object itself now. */
  free(to_free);

  return RF_OPSUCC;

} /* intern__findf__free_node() */



int intern__findf__destroy_list(findf_list_f *headnode)
{
  findf_list_f *tmp = NULL;

  if (headnode == NULL){
    errno = EINVAL;
    return ERROR;
  }

  while(headnode != NULL) {
    tmp = headnode;
    headnode = headnode->next;
    intern__findf__free_node(tmp);
  }

  return RF_OPSUCC;

} /* intern__findf__destroy_list() */

/* 
 * Cleanup all fields of 'headnode', swap it with the next node.
 * Reusing the list helps Libfindf save alot of memory allocations.
 */
findf_list_f *intern__findf__shift_node(findf_list_f *headnode)
{

  size_t i = 0;
  findf_list_f *saved_old_head = NULL;
  findf_list_f *saved_old_next = NULL;
  
  if ((headnode != NULL)
      && (headnode->next != NULL)) {
    ;
  }
  else{
    errno = EINVAL;
    perror("intern__findf__shift_node");
    return NULL;
  }
  /* Clear the list. */
  headnode->position = 0;
  headnode->list_level = headnode->next->list_level + 1;
  for (i = 0; i < headnode->size; i++)
    memset(headnode->pathlist[i], 0 , F_MAXPATHLEN);
  /* Flip the 'nextnode' with the headnode. */
  saved_old_next = headnode->next;
  saved_old_head = headnode;
  headnode->next = NULL;
  headnode = saved_old_next;
  headnode->next = saved_old_head;
  
  return headnode;
  
} /* intern__findf__shift_node() */


int intern__findf__add_element(char *element,
			       findf_list_f *list)
{
  char **tmp = NULL;   /* In case we need a bigger list. */
  char *error_mesg = NULL;
  size_t tmpsize = 0;  
  size_t c;            /* Convinience. */

  /* 
   * Very quick parameter check.
   * Not verifying the lenght of the element to add,
   * am trusting SU_strcpy to fail if it can't fit the source
   * in destination. 
   */

  if ((element != NULL)
      && (list != NULL)){
    ;
  }  
  else{
    errno = EINVAL;
    perror("intern__findf__add_element");
    return ERROR;
  }
  
  /* 
   * Compare the list's size and position,
   * when they are equal we need more memory. 
   */
  if (list->size == list->position){
    tmpsize = (list->size) * 2;
    if ((tmp = realloc(list->pathlist, tmpsize * sizeof(char*))) == NULL){
      perror("Realloc");
      /*
       * Don't need to goto add_elem_error.
       * list->pathlist still needs to be freed by our caller.
       */
      return ERROR;
    }
    /*    list->pathlist = NULL;*/ /* Not needed. */
    for (c = list->position; c < tmpsize; c++)
      if (( tmp[c] = calloc(F_MAXPATHLEN, sizeof(char))) == NULL){
	error_mesg = "Calloc";
	goto add_elem_error;
      }
  
    list->pathlist = tmp;
    list->size = tmpsize;
  }
    
  
  /* Add the element to the pathlist. */
  if (SU_strcpy(list->pathlist[list->position],
		element,
		F_MAXPATHLEN) == NULL){
    intern_errormesg("Failed call to SU_strcpy\n");
    return ERROR;
  }
  list->position += 1;

  return RF_OPSUCC;

 add_elem_error:
  if (tmp){
    for (c = 0; c < tmpsize; c++){
      if (tmp[c]){
	free(tmp[c]);
	tmp[c] = NULL;
      }
    }
    free(tmp);
  }
  perror(error_mesg);
  return ERROR;
}



/* Parameter operations: */
findf_param_f *intern__findf__init_param(char **_file2find,
					 char **search_roots,
					 size_t numof_file2find,
					 size_t numof_search_roots,
					 unsigned int dept,
					 findf_type_f search_type,
					 findf_sort_type_f sort_type,
					 void *(*sort_f)(void *sarg),
					 void *sarg,
					 void *(*algorithm)(void *arg),
					 void *arg)

{
  findf_param_f *to_init = NULL;
  size_t i;
  char *error_mesg = NULL;
  /* goto label init_param_err;          Clean up on error. */

  /* 
   * We assume all parameters are valid.
   * In theory, a user should never see nor use this routine,
   * but use only the public-facing findf_init_param() routine which does
   * verify all of its parameters.
   * That's a long way of saying be careful when using this function..!
   */
  
  /* Allocate memory to the parameter object itself. */
  if ((to_init = malloc(sizeof(findf_param_f))) == NULL){
    error_mesg = "Malloc";
    goto init_param_err;
  }

  /* Allocate memory to the file to find's array. */
  if ((to_init->file2find = calloc(numof_file2find, sizeof(char *))) == NULL){
    error_mesg = "Malloc";
    goto init_param_err;
  }
  for (i = 0; i < numof_file2find; i++){
    if ((to_init->file2find[i] = calloc(F_MAXNAMELEN, sizeof(char))) == NULL){
      error_mesg = "Malloc";
      goto init_param_err;
    }
    if (SU_strcpy(to_init->file2find[i], _file2find[i], F_MAXNAMELEN) == NULL) {
      intern_errormesg("Failed call to SU_strcpy\n");
      error_mesg = "SU_strcpy";
      goto init_param_err;
    }
  }

  /* Allocate memory to the search_roots list. */
  if ((to_init->search_roots = intern__findf__init_node(numof_search_roots >= DEF_LIST_SIZE 
							? numof_search_roots 
							: DEF_LIST_SIZE
							, 0, false)) == NULL) {
    intern_errormesg("Failed to initialize a new node.\n");
    error_mesg = "Intern__findf__init_node";
    goto init_param_err;
  }
  for (i = 0; i < numof_search_roots; i++){
    if ((intern__findf__add_element(search_roots[i], to_init->search_roots)) != RF_OPSUCC){
      intern_errormesg("Failed to add a new element to an existing node.\n");
      error_mesg = "Intern__findf__add_element";
      goto init_param_err;
    }
  }
  
  
  /* Allocate memory to the search_results list. */
  if ((to_init->search_results = intern__findf__init_node(DEF_LIST_SIZE, 0, false)) == NULL) {
    intern_errormesg("Failed to initialize a new node. \n");
    error_mesg = "Intern__findf__init_node";
    goto init_param_err;
  }

  
  to_init->sizeof_file2find = numof_file2find;
  to_init->sizeof_search_roots = numof_search_roots == 0 ? 1 : numof_search_roots;
  to_init->dept = dept;
  to_init->search_type = search_type;
  to_init->sort_type = sort_type;
  to_init->sort_f = sort_f;
  to_init->sarg = sarg;
  to_init->algorithm = algorithm;
  to_init->arg = arg;

  return to_init;

 init_param_err:
  /* Print why we got here in case any of the _destroy_list() calls fails. */
  if (error_mesg) 
    perror(error_mesg);
  if (to_init){
    /* Make sure all pointers points to NULL. */
    to_init->arg = NULL;
    to_init->algorithm = NULL;
    to_init->sarg = NULL;
    to_init->sort_f = NULL;
    
    if (to_init->search_results){
      if (intern__findf__destroy_list(to_init->search_results) != RF_OPSUCC){
	/* Print why _destroy_list has failed and continue. */
	perror("Intern__findf__destroy_list");
      }
    }
    if (to_init->search_roots) {
      if (intern__findf__destroy_list(to_init->search_roots) != RF_OPSUCC){
	/* Print why _destroy_list has failed and continue. */
	perror("Intern__findf__destroy_list");
      }
    }
    if (to_init->file2find) {
      for (i = 0; i < numof_file2find; i++) {
	if (to_init->file2find[i]){
	  free(to_init->file2find[i]);
	  to_init->file2find[i] = NULL;
	}
      }
      free(to_init->file2find);
      to_init->file2find = NULL;
    }
    free(to_init);
  }
  return NULL;
}


int intern__findf__free_param(findf_param_f *to_free)
{
  size_t i;
   
  for (i = 0; i < to_free->sizeof_file2find; i++){
    free(to_free->file2find[i]);
    to_free->file2find[i] = NULL;
  }
  free(to_free->file2find);
  
  if ((intern__findf__destroy_list(to_free->search_roots) != RF_OPSUCC)
      || (intern__findf__destroy_list(to_free->search_results) != RF_OPSUCC)){
    /* Print what just happened and continue. */
    intern_errormesg("Failed to release resources of a parameter's list.\n");
  }
  

  to_free->search_roots = NULL;
  to_free->search_results = NULL;
  to_free->file2find = NULL;
  to_free->sort_f = NULL;
  to_free->sarg = NULL;
  to_free->algorithm = NULL;
  to_free->arg = NULL;

  free(to_free);
  /*  to_free = NULL;*/


  return RF_OPSUCC;
} /* intern__findf__free_param() */


int intern__findf__path_forward(char *dest,
				char *element,
				size_t destsize)
{
  size_t  _elem_len = 0,dest_len = 0;
  size_t  i = 0, c = 0;
  
  if ((dest != NULL)
      && (element != NULL)
      && (destsize > 0)
      && (destsize < SIZE_MAX - 1)){
    ;
  }
  else{
    errno = EINVAL;
    perror("intern__findf__path_forward");
    return ERROR;
  } 
  _elem_len = strnlen(element, F_MAXNAMELEN - 1);
  dest_len = strnlen(dest, F_MAXPATHLEN - 1);
  
  /* 
   * +2: 1 In case we need to append a trailing slash 
   *     to dest before adding element to it.
   *	 1 For the trailing '\0' that we force at the end of the new pathname. 
   */
  if (((dest_len + _elem_len + 2) > (destsize - 1))
      || ((dest_len + _elem_len + 2) >= F_MAXPATHLEN)){
    errno = EOVERFLOW;
    intern_errormesg("Pathname overflow.");
    return ERROR;
  }

  /* 
   * Check if dest ends  with a slash '/' (That is before the terminating NULL byte).
   * If not, add one and increment dest_len to match the new lenght.
   * dest[dest_len] is the NUL at the end of string.
   */
  if (dest[dest_len - 1] != '/'){
    dest[dest_len] = '/';
    dest_len++;
  }
  
  /* Append element to dest. */
  for (i = dest_len, c = 0; 
       element[c] != '\0' && (i < F_MAXPATHLEN - 1);
       i++, c++)
    dest[i] = element[c];
  
  /* Make sure no slash slipped in at the end of the new pathname. */
  if (dest[i - 1] == '/')
    dest[i - 1] = '\0';
  dest[i] = '\0';
  
  return RF_OPSUCC;
  
} /* intern__findf__path_forward() */


void intern__findf__cmp_file2find(findf_param_f *t_param,
				  char *entry_to_cmp,
				  char *entry_full_path)
{
  size_t i = 0;
  size_t _file2find_len = 0;

  for (i = 0; i < t_param->sizeof_file2find ; i++){
    _file2find_len = strnlen(t_param->file2find[i], F_MAXNAMELEN - 1);
    if (memcmp(t_param->file2find[i], entry_to_cmp, _file2find_len) == 0){
      if (intern__findf__add_element(entry_full_path, t_param->search_results) != RF_OPSUCC){
	intern_errormesg("Failed to add a new element to a parameter's search_results list.\n");
	abort(); /* I think I'm being a little rude here.. */
      }
    }
  }
} /* intern__findf__cmp_file2find() */


findf_tpool_f *intern__findf__init_tpool(unsigned long numof_threads)
{
  findf_tpool_f *to_init = NULL;
  char *error_mesg = NULL;
  /* goto label init_pool_err;         In case we must evacuate. */
  
  if ((to_init = malloc(sizeof(findf_tpool_f))) == NULL){
    error_mesg = "Malloc";
    goto init_pool_err;
  }
  memset(to_init, 0, sizeof(findf_tpool_f));
  to_init->num_of_threads = numof_threads;

  if ((to_init->threads = calloc(to_init->num_of_threads , sizeof(pthread_t))) == NULL){
    error_mesg = "Calloc";
    goto init_pool_err;
  }

  return to_init;

 init_pool_err:
  if (to_init){
    if (to_init->threads){
      free(to_init->threads);
      to_init->threads = NULL;
    }
    free(to_init);
  }
  perror(error_mesg);
  
  return NULL;

} /* intern__findf__tpool_f() */


void intern__findf__free_tpool(findf_tpool_f *to_free)
{
  free(to_free->threads);
  to_free->threads = NULL;
  to_free->num_of_threads = '\0';
  free(to_free);
} /* intern__findf__free_tpool() */


findf_results_f *intern__findf__init_res(size_t bufsize,
					 char **buffer)
{
  unsigned int i = 0;
  findf_results_f *to_init = NULL;
  char *error_mesg = NULL;
  /* goto label  init_res_err;          Early cleanup. */
  
  if ((to_init = malloc(sizeof(findf_results_f))) == NULL){
    error_mesg = "Malloc";
    goto init_res_err;
  }
  if ((to_init->res_buf = calloc(bufsize, sizeof(char*))) == NULL){
    error_mesg = "Calloc";
    goto init_res_err;
  }
  for (i = 0; i < bufsize; i++) {
    if ((to_init->res_buf[i] = calloc(F_MAXPATHLEN, sizeof(char))) == NULL){
      error_mesg = "Calloc";
      goto init_res_err;
    }
    if (SU_strcpy(to_init->res_buf[i], buffer[i], F_MAXPATHLEN) == NULL){
      intern_errormesg("SU_strcpy failure");
      error_mesg = "SU_strcpy";
      goto init_res_err;
    }
  }

  to_init->numof_elements = bufsize;
  to_init->max_string_len = F_MAXPATHLEN;

  return to_init;

 init_res_err:
  if (to_init){
    if (to_init->res_buf){
      for (i = 0; i < bufsize; i++){
	
	if (to_init->res_buf[i]){
	  free(to_init->res_buf[i]);
	  to_init->res_buf[i] = NULL;
	}
      }
      free(to_init->res_buf);
      to_init->res_buf = NULL;
    }
    free(to_init);
  }
  perror(error_mesg);
  return NULL;

} /* intern__findf__init_res() */


void intern__findf__free_res(findf_results_f *to_free)
{
  unsigned int i = 0;

  for (i = 0; i < to_free->numof_elements; i++){
    free(to_free->res_buf[i]);
    to_free->res_buf[i] = NULL;
  }
  free(to_free->res_buf);
  to_free->res_buf = NULL;

  to_free->numof_elements = '\0';
  to_free->max_string_len = '\0';

  free(to_free);
  to_free = NULL;

} /* intern__findf__free_res() */


/* 
 * For most system calls within intern__findf__opendir(),
 * we ignore, sometimes silently sometimes not, those 
 * directories for which we get an EACCES, ENOENT or ENOTDIR 
 * error code. It may happen that a directory gets deleted
 * or a process terminated in between the time when their pathnames
 * were recorded into the linked-list, and the time we try to open,
 * read and stat them. 
 *
 * I recently been aware that readdir_r() will be (is) deprecated in 
 * favor of readdir(). Even tho readdir() is supposed to become thread-safe,
 * I wrote Libfindf on a Debian 8.5 system with Glibc 2.19 in which readdir() 
 * is not yet thread-safe. The cost in execution time to serialize, even just
 * the intern__findf__opendir() call is enormous, intern__findf__opendir() being 
 * on of the routine that's the most called throughout the library.
 * This explains why I choose to stick with the thread-safe readdir_r() routine
 * at least for the time being.
 * 
 */

int intern__findf__opendir(char *pathname,
			   findf_list_f *nextnode,
			   findf_param_f *t_param)
{
  char temppathname[F_MAXPATHLEN] = "";       /* Directory we're working on. */
  DIR *dirstream = NULL;                      /* Stream returned by opendir(). */
  struct dirent *res = NULL;                  /* To feed readdir_r(). */
  struct stat stat_buf;                       /* Buffer used by lstat(). */
  
  /* Verify parameters. */
  if ((pathname != NULL && pathname[0] != '\0')
      && (nextnode != NULL)
      && (t_param != NULL))
    ;
  else{
    errno = EINVAL;
    perror("intern__findf__opendir");
    return ERROR;
  }
  if ((dirstream = opendir(pathname)) == NULL){
    /* 
     * Don't return an error when hitting a forbidden directory.
     * Print a message ONLY if permission is denied or 
     * the error was not EACCES, ENOTDIR, ENOENT.
     * QUIET_OPENDIR must be undefined for the error message to show up.
     * (Defined by default, happens too often for a regular user).
     * Then, get another directory.
     */
    if (errno == EACCES){
#ifndef QUIET_OPENDIR
      pthread_mutex_lock(&stderr_mutex);
      fprintf(stderr, "Thread [%zu]\n%s: %s: Permission denied.\n",
	      pthread_self(), __func__, pathname);
      pthread_mutex_unlock(&stderr_mutex);
#endif
      if (dirstream)
	closedir(dirstream);
      return RF_OPSUCC;
    }
    else if (errno == ENOTDIR
	     || errno == ENOENT){
      if (dirstream)
	closedir(dirstream);
      /* Silently discard any invalid or unexistant directories. */
      return RF_OPSUCC;
    }
    /* We cannot allow any other errors throught. */
#ifndef QUIET_OPENDIR
    pthread_mutex_lock(&stderr_mutex);
    fprintf(stderr, "Thread [%zu]\n", pthread_self());
    perror("opendir");
    fprintf(stderr, "\nFaulty: %s\n", pathname);
    pthread_mutex_unlock(&stderr_mutex);
#endif /* QUIET_OPENDIR */

    if (dirstream)
      closedir(dirstream);
    return ERROR;
  }
  
  /* Read the directory's content. */
  while (1){
    if ((readdir_r(dirstream, entry, &res)) != 0){
#ifndef QUIET_OPENDIR
	pthread_mutex_lock(&stderr_mutex);
	fprintf(stderr, "Thread [%zu]\n%s: On entry: %s: Operation not permitted.\n",
		pthread_self(), __func__, entry->d_name);
	pthread_mutex_unlock(&stderr_mutex);
#endif /* QUIET_OPENDIR */

	/* 
	 * Force termination of intern__findf__opendir() on error 
	 * from readdir_r() by simply jumping out of this loop. 
	 */
	break; 
    }

    if (res == NULL){ /* The directory is completely read. */
      if (dirstream)
	closedir(dirstream);
      return RF_OPSUCC;
    }
    
    /* Skip '.' and '..' directories. */
    if((memcmp(entry->d_name, DOT, DOTLEN) == 0)
       || (memcmp(entry->d_name, DOTDOT, DOTDOTLEN) == 0)){
      continue;
    }
    
    /* Copy pathname into temppathname. */
    memset(temppathname, 0, F_MAXPATHLEN); 
    if (SU_strcpy(temppathname, pathname, F_MAXPATHLEN) == NULL){
      intern_errormesg("Failed call to SU_strcpy\n");
      return ERROR;
    }
    if (intern__findf__path_forward(temppathname, entry->d_name, F_MAXPATHLEN) != RF_OPSUCC){
      intern_errormesg("Failed call to intern__findf__path_forward.\n");
      return ERROR;
    }
    /* 
     * Debugged quite a few bugs.
     * But does produce tons of output..
     */
#ifndef QUIET_OPENDIR
# ifdef DEBUG
    pthread_mutex_lock(&stderr_mutex);
    fprintf(stderr,"Thread [%zu]\ntemppathname is: %s\n",
	    pthread_self(), temppathname);
    pthread_mutex_unlock(&stderr_mutex);
# endif /* DEBUG */
#endif
    
    /* Check if the current entry is a directory. */
    /* May be a security risk: */
    if (lstat(temppathname, &stat_buf) == -1){
      if((errno == EACCES)
	 || (errno == ENOENT)
	 || (errno == ENOTDIR)){
	/* Silently skip those kind of issues. */
	continue;
      }
      perror("lstat");
      return ERROR;
    }
    

    /* Compare the entry with the file(s) to find. */
    intern__findf__cmp_file2find(t_param, entry->d_name, temppathname);

    /* If the entry is a directory, we need it! */
    if (S_ISDIR(stat_buf.st_mode))
      if ((intern__findf__add_element(temppathname, nextnode)) != RF_OPSUCC){
	intern_errormesg("Failed to add element to list\n");
	return ERROR;
      }
    continue;
  }
  /* In case we broke up of while early. */
  if (dirstream)
    closedir(dirstream);
  return RF_OPSUCC;

}  /* intern__findf__opendir() */


size_t intern__findf__get_avail_cpus(void)
{
  /* Always return at least DEF_THREADS_NUM threads. */
  long sys_ret = 0;
  size_t ret = 0;
  sys_ret = sysconf(_SC_NPROCESSORS_ONLN);
  /* Make sure the value returned by sysconf is within reasonable limits. */
  if (sys_ret > DEF_THREADS_NUM && sys_ret < DEF_MAX_THREADS_NUM)
    return (ret = sys_ret);
  else
    return (ret = DEF_THREADS_NUM);
}

/* 
 * Copy source into destination of size n (including the trailing NUL byte).
 * Note, SU_strcpy() always clears the buffer before
 * copying results into it. 
 */
void *SU_strcpy(char *dest, char *src, size_t n)
{
  size_t src_s = 0;

  
  if (dest != NULL                /* We need an already initialized buffer. */
      && src != NULL              /* We need a valid, non-empty source. */
      && src[0] != '\0'
      && n > 0                    /* Destination buffer's size must be bigger than 0, */
      && n < (SIZE_MAX - 1))      /* and smaller than it's type size - 1. */
    ; /* Valid input. */
  else {
    errno = EINVAL;
    return NULL;
  }
  
  /* Look out for a NULL byte, never past SIZE_MAX - 1. */
  src_s = strnlen(src, SIZE_MAX - 1);
  if (src_s > n - 1){ /* -1, we always add a NULL byte at the end of string. */
    errno = EOVERFLOW;
    fprintf(stderr,"%s: Source must be at most destination[n - 1]\n\n", __func__);
    dest = NULL;
    return dest;
  }
  
  memset(dest, 0, n);
  memcpy(dest, src, src_s);
  
  return dest;
}

void intern__findf__verify_search_type(findf_param_f *search_param)
{
  if (search_param->search_type == BFS
      || search_param->search_type == DFS
      || search_param->search_type == IDDFS
      || search_param->search_type == IDBFS
      ){
    ; /* Valid */
  }
  else {
    search_param->search_type = BFS;
    search_param->algorithm = intern__findf__BF_search;
  }
}
