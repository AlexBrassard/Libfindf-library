/*
 *
 *
 * Libfindf.so  -  Sorting related routines source file.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

#include <findf.h>
#include "libfindf_private.h"




/* Rotate a buffer of array's indexes "counter-clockwise" */
int intern__findf__rotate_buffer(size_t *sorted_array,  /* Buffer to rotate. */
				 size_t ind_to_move,    /* The index we're moving. */
				 size_t *last_pos_ind)  /* 
							 * Address of the index of the last
							 * non-sorted position of sorted_array. 
							 */
{

  size_t temp_index = sorted_array[ind_to_move];
  size_t i = 0;
  if (ind_to_move < *last_pos_ind){
    for (i = ind_to_move; i < *last_pos_ind; i++) 
      sorted_array[i] = sorted_array[i + 1];
    sorted_array[(*last_pos_ind)--] = temp_index;
  }
  return RF_OPSUCC;

} /* intern__findf__rotate_buffer() */


/*
 * Sort an array of pathnames.
 * Groups sort_buffer's pathnames by file2find filenames,
 * then sort each groups alphabeticaly before 
 * reorganizing the original sort_buffer. 
 */
int intern__findf__sortp(char **sort_buffer,       /* Pathnames to sort. */
			 char **file2find,         /* Grouping sort_buf using these pathnames. */
			 size_t sizeof_sort_buf,   /* Number of pathnames in sort_buffer. */
			 size_t sizeof_file2find)  /* Number of filenames in file2find. */
{
  /* Array of whole_buf indexes, grouped by file2find filenames. */
  size_t *sort_array = NULL;
  /* Array of whole_buf indexes, grouped by file2find filenames, sorted alphabeticaly. */
  size_t *sorted_array = NULL;           
  size_t last_pos_ind = 0;               /* Position of the last non-sorted element of sorted_array. */
  size_t sizeof_sort_array = 0;          /* Number of indexes sort_array may contain. */
  size_t *final_array = NULL;            /* Array of whole_buf indexes in their final order. */
  size_t final_array_c = 0;              /* First free element of final_array. */
  size_t temp_step2 = 0;                 /* Temporary index for step 2. */
  size_t file2find_c = 0;                /* Counter, current file2find we're working on. */
  size_t S1 = 0, S2 = 0;                 /* Counters, "string 1", "string 2". */
  size_t i = 0;                          /* Convinience counter. */
  char **temp_path = NULL;               /* To reorganize sort_buf; */

#ifdef FINDF_SORT_DEBUG
  size_t debug_c = 0;                    /* For debug purposes. */
#endif /* FINDF_SORT_DEBUG */


  if ((file2find != NULL && file2find[0] != NULL) /* Testing for element 0 enough? */
      && (sort_buffer != NULL && sort_buffer[0]!= NULL)
      && sizeof_file2find > 0
      && sizeof_sort_buf > 0){
    ;
  }
  else {
    errno = EINVAL;
    return ERROR;
  }

  /* Allocate memory:  */
  if ((sort_array = calloc(sizeof_sort_buf, sizeof(size_t))) == NULL){
    intern_errormesg("Calloc failure");
    return ERROR;
  }
  if ((sorted_array = calloc(sizeof_sort_buf, sizeof(size_t))) == NULL){
    intern_errormesg("Calloc failure");
    return ERROR;
  }
  if ((final_array = calloc(sizeof_sort_buf, sizeof(size_t))) == NULL){
    intern_errormesg("Calloc failure");
    return ERROR;
  }
  if ((temp_path = calloc(sizeof_sort_buf, sizeof(char *))) == NULL){
    intern_errormesg("Calloc failure");
    return ERROR;
  }

  for (file2find_c = 0; file2find_c < sizeof_file2find; file2find_c++){
    /* Reset the size of sort_array, we're calculating the new size next. */
    sizeof_sort_array = 0;
    for (i = 0; i < sizeof_sort_buf; i++)
      if (strstr(sort_buffer[i], file2find[file2find_c]) != NULL){
	sort_array[sizeof_sort_array] = i;
	sorted_array[sizeof_sort_array] = i;
	sizeof_sort_array++;
      }
    /* 
     * Get next file2find filename in case we did not find any 
     * match in the previous loop.
     */
    if (sizeof_sort_array == 0) continue;

    /* Set our last position index to the last element of sort_array. */
    last_pos_ind = sizeof_sort_array - 1;

#ifdef FINDF_SORT_DEBUG
    printf("\nContent of sort_array:\n\"Bucketized only\"\n");
    for (debug_c = 0; debug_c < sizeof_sort_array; debug_c++)
      printf("sort_array[%zu]: [%s]\n", 
	     debug_c, sort_buffer[sort_array[debug_c]]);
#endif /* FINDF_SORT_DEBUG */
    
    /* 
     * Step one, 
     * compare each paths against each others.
     * As soon as a path bigger than the one we're comparing the others
     * against is found, rotate the buffer according to its index. 
     */
    for (S1 = 0; S1 < sizeof_sort_array; S1++){
      for (S2 = 0; S2 < sizeof_sort_array; S2++){
	if (S2 == S1) continue; /* Don't compare same paths. */
	if (strcmp(sort_buffer[sort_array[S1]],
		   sort_buffer[sorted_array[S2]]) < 0)
	  intern__findf__rotate_buffer(sorted_array,
				       S2,
				       &last_pos_ind);
      }/* for(S2=0...) */
    } /* for(S1=0...) */

#ifdef FINDF_SORT_DEBUG
    printf("\nContent of sorted_array:\nPartly sorted (between step 1 and 2)\n");
    for (debug_c = 0; debug_c < sizeof_sort_array; debug_c++)
      printf("sorted_array[%zu]: [%s]\n", 
	     debug_c, sort_buffer[sorted_array[debug_c]]);
#endif /* FINDF_SORT_DEBUG */

    /* 
     * Step two, 
     * compare each path against the one beside it 
     * in the sorted_array, switch the unordered ones and start over. 
     */
  reset_loop:
    for (S1 = 0; S1 < (sizeof_sort_array - 1); S1++){
      if (strcmp(sort_buffer[sorted_array[S1]],
		 sort_buffer[sorted_array[S1 + 1]]) <= 0){
	; /* Continue */
      }
      else{
	temp_step2 = sorted_array[S1];
	sorted_array[S1] = sorted_array[S1 + 1];
	sorted_array[S1 + 1] = temp_step2;
	goto reset_loop; /* Arround 11 lines above is the label. */
      }
    } /* for (S1=0...) step2 */

#ifdef FINDF_SORT_DEBUG
    printf("\nContent of sorted_array:\nCompletely sorted\n");
    for (debug_c = 0; debug_c < sizeof_sort_array; debug_c++)
      printf("sorted_array[%zu]: [%s]\n", 
	     debug_c, sort_buffer[sorted_array[debug_c]]);
#endif /* FINDF_SORT_DEBUG */
    
    /* We'll use this array to reorganize sort_buffer[][] */
    for (i = 0; i < sizeof_sort_array; i++){
      final_array[final_array_c++] = sorted_array[i];
    }

  } /* for(file2find_c = 0...) */
  
  /* Reorganize the buffer of pathnames given by our caller. */
  for (i = 0; i < sizeof_sort_buf; i++){
    temp_path[i] = sort_buffer[final_array[i]];
  }
  for (i = 0; i < sizeof_sort_buf; i++){
    sort_buffer[i] = temp_path[i];
  }

  /* Free used resources. */
  if (sort_array != NULL){
    free(sort_array);
    sort_array = NULL;
  }
  if (sorted_array != NULL){
    free(sorted_array);
    sorted_array = NULL;
  }
  if (final_array != NULL){
    free(final_array);
    final_array = NULL;
  }
  if (temp_path != NULL){
    free(temp_path);
    temp_path = NULL;
  }
    
  return RF_OPSUCC;
} /* intern__findf__sortp() */
