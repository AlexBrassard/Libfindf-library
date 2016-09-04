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

/* Convert an already initialized char* filename into a FRE compatible char* filename. */
char* intern__findf__string_to_regex(char *filename)
{
  /* 
   * Loop over the given filename, surround
   * every punctuation mark with '[' ']' verifying the lenght
   * every time making sure to respect POSIX's 256 char limit.
   */
  size_t filename_len = strnlen(filename, (SIZE_MAX - 1)); /* Current lenght of filename. */
  size_t token_ind = 0;                                    /* Index of the current token. */
  size_t modified_token_ind = 0;                           /* Index of current token within the modified_filename. */
  char   modified_filename[FINDF_MAX_PATTERN_LEN];                        /* The modified filename. */

  /* Important: 
   * Check cases where F_MAXNAMELEN was changed to a different value
   * than the default (255).
   */
  if (filename == NULL){
    errno = EINVAL;
    return NULL;
  }
  
  for(; token_ind < filename_len; token_ind++){
    if (ispunct(filename[token_ind])) {
      /* Make sure the pattern will respect POSIX's lenght limit. */
      if ((filename_len + 2) >= FINDF_MAX_PATTERN_LEN){
	intern_errormesg("Converting filename will break POSIX's pattern lenght limit");
	return NULL;
      }	  
      modified_filename[modified_token_ind++] = '[';
      modified_filename[modified_token_ind++] = filename[token_ind];
      modified_filename[modified_token_ind++] = ']';
      filename_len += 2;
      continue;
    }
    modified_filename[modified_token_ind++] = filename[token_ind];
  }
  /* NULL Terminate the new string. */
  modified_filename[modified_token_ind] = '\0';
  /* Add the NULL byte to the filename lenght, check if it respects the limit. */
  if ((filename_len + 1) > FINDF_MAX_PATTERN_LEN){
    intern_errormesg("Converting filename will break POSIX's pattern lenght limit");
    return NULL;
  }

  if (SU_strcpy(filename, modified_filename, FINDF_MAX_PATTERN_LEN) == NULL){
    intern_errormesg("SU_strcpy failure");
    return NULL;
  }
  return filename;

} /* intern__findf__string_to_regex() */


/* Prepare filenames and/or regex patterns to be used as sorting keys. */
findf_regex_f** intern__findf__init_fre_keys(findf_regex_f **reg_array,  /* The array of FRE objects to extend. */
					     char **filenames,           /* An array of filenames to convert into patterns. */
					     size_t *sizeof_reg_array,    /* Current number of elements in reg_array. */
					     size_t numof_filenames)     /* Number of strings in filenames. */
{
  /* _init_fre_keys() assume all its parameters are valid. */

  size_t i = 0;
  size_t new_size = ((*sizeof_reg_array) + numof_filenames);         /* Used to allocate an extended reg_array. */
  size_t freg_object_c = 0;                                       /* Counter, from the last "old reg_array" element. */
  findf_regex_f **new_reg_array = NULL;                           /* The new array to return to our caller. */

  if ((new_reg_array = realloc(reg_array, new_size * sizeof(findf_regex_f*))) == NULL){
    intern_errormesg("Failed to realloc reg_array");
    return NULL;
  }
  /* Initialize each new findf_regex_f * we just made. */
  for(freg_object_c = (*sizeof_reg_array); freg_object_c < new_size; freg_object_c++){
    if ((new_reg_array[freg_object_c] = intern__findf__init_regex()) == NULL){
      intern_errormesg("Failed call to intern__findf__init_regex");
      goto init_key_err_jmp;
    }
  }

  /* Convert filenames, if any, into FRE patterns. */
  if (numof_filenames > 0){
    freg_object_c = (*sizeof_reg_array);
    for (i = 0; i < numof_filenames; i++){
      if (SU_strcpy(new_reg_array[freg_object_c]->pat_storage->pattern1,
		    filenames[i],
		    FINDF_MAX_PATTERN_LEN) == NULL){
	intern_errormesg("Failed to copy filename into its freg_object");
	goto init_key_err_jmp;
      }
      if (intern__findf__string_to_regex(new_reg_array[freg_object_c]->pat_storage->pattern1) == NULL){
	intern_errormesg("Failed to convert filename into a regex pattern");
	goto init_key_err_jmp;
      }
      /* Compile the pattern now. */
      if (intern__findf__compile_pattern(new_reg_array[freg_object_c]) != RF_OPSUCC){
	intern_errormesg("Failed to compile converted filename");
	goto init_key_err_jmp;
      }
      /* Get next filename. */
      ++freg_object_c;
    }
  }
  /* Adjust sizeof_reg_array to new_size. */
  (*sizeof_reg_array) = new_size;
  return new_reg_array;
  

 init_key_err_jmp:
  if (new_reg_array != NULL){
    intern__findf__free_regarray(new_reg_array, new_size);
  }
  return NULL;
} /* intern__findf__init_fre_keys() */
					     

/* Rotate a buffer of array's indexes "counter-clockwise" */
inline int intern__findf__rotate_buffer(size_t *sorted_array,  /* Buffer to rotate. */
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
 * Split the source buffer into buckets, keyed either by char* filenames or
 * regex_t* patterns. The size of each buckets depends on how many token matches each keys.
 * If a token matches more than 1 key, its token index will be in each matched key's bucket.
 * Let _sortp() check for dupplicates before reorganizing all tokens of every buckets back into
 * the caller's given source buffer.
 */
inline size_t** intern__findf__bucketize(char **buf_tosort,
					 void **keys,
					 size_t numof_keys,            
					 size_t sizeof_buf_tosort,
					 bool FRE_KEYS)  /* True == (keys = Findf-regex keys). */
{
  size_t  i = 0;
  size_t  bbucket_ind = 0;            /* Big bucket's index. */
  size_t  tbucket_ind = 0;            /* Tiny buckets' index. */
  size_t  keys_ind = 0;               /* Keys' index. */
  size_t  tsort_ind = 0;              /* buf_tosort's index. */
  size_t  **big_bucket = NULL;        /* The array of buckets to return to our caller. */
  char    **filenames = NULL;         /* If keys are filenames. */
  regex_t **patterns = NULL;          /* If keys are REGEX patterns. */

  /* _bucketize() assumes all its parameters are valid. */
  
  if ((big_bucket = malloc(numof_keys * sizeof(size_t*))) == NULL){
    intern_errormesg("Malloc failure");
    return NULL;
  }
  for (i = 0; i < numof_keys; i++){
    if ((big_bucket[i] = calloc(sizeof_buf_tosort, sizeof(size_t))) == NULL){
      intern_errormesg("Calloc failure");
      goto bkt_err_jmp;
    }
  }
  
  /* Check whether keys are filenames or REGEX patterns. */
  if (FRE_KEYS == true){
    patterns = ((void*)keys);
    for(keys_ind = 0; keys_ind < numof_keys; keys_ind++){
      for(tsort_ind = 0; tsort_ind < sizeof_buf_tosort; tsort_ind++){
	if (regexec(patterns[keys_ind], buf_tosort[tsort_ind], 0, NULL, 0) == 0) { /* Match ! */
	  big_bucket[bbucket_ind][tbucket_ind++] = tsort_ind;
	}
      }
      /* 
       * NULL terminate the current tiny bucket,
       * reset the tiny bucket's index, 
       * increment the big bucket's index and get the next key.
       */
      big_bucket[bbucket_ind][tbucket_ind] = '\0';
      tbucket_ind = 0;
      ++bbucket_ind;
    }
  }

  /* Filename keys. */
  else {
    filenames = ((void*)keys);
    for (keys_ind = 0; keys_ind < numof_keys; keys_ind++){
      for (tsort_ind = 0; tsort_ind < sizeof_buf_tosort; tsort_ind++){
	if (strstr(buf_tosort[tsort_ind], filenames[keys_ind]) != NULL){ /* Match ! */
	  big_bucket[bbucket_ind][tbucket_ind++] = tsort_ind;
	}
      }
      /* See comment above, this is the same. */
      big_bucket[bbucket_ind][tbucket_ind] = '\0';
      tbucket_ind = 0;
      ++bbucket_ind;
    }
  }
      
  /* Return the big_bucket to our caller. */
  return big_bucket;


 bkt_err_jmp:
  if (big_bucket != NULL){
    for (i = 0; i < numof_keys; i++){
      if(big_bucket[i] != NULL){
	free(big_bucket[i]);
	big_bucket[i] = NULL;
      }
    }
    free(big_bucket);
    big_bucket = NULL;
  }
  return NULL;
  
} /* intern__findf__bucketize() */


/*
 * Sort an array of pathnames.
 * Groups sort_buffer's pathnames by file2find filenames,
 * then sort each groups alphabeticaly before 
 * reorganizing the original sort_buffer. 
 */
int intern__findf__sortp(char **sort_buffer,       /* Pathnames to sort. */
			 char **file2find,         /* filenames or regex pattern to bucketize pathnames. */
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
  bool ERR = false;                      /* True on error. */
  /*goto label sort_err_jmp;                Cleanup on exit. */


  /*
   * Every time strstr matches a pathname, put its index in matched_buffer.
   *
   */

  
#ifdef FINDF_SORT_DEBUG
  size_t debug_c = 0;                    /* For debug purposes. */
#endif /* FINDF_SORT_DEBUG */

  if (file2find != NULL
      && sizeof_file2find > 0
      && sort_buffer != NULL
      && sizeof_sort_buf > 0){
    ;
  }
  else{
    errno = EINVAL;
    ERR = true;
    goto sort_err_jmp;
  }


  /* Allocate memory:  */
  if ((sort_array = calloc(sizeof_sort_buf, sizeof(size_t))) == NULL){
    intern_errormesg("Calloc failure");
    ERR = true;
    goto sort_err_jmp;
  }
  if ((sorted_array = calloc(sizeof_sort_buf, sizeof(size_t))) == NULL){
    intern_errormesg("Calloc failure");
    ERR = true;
    goto sort_err_jmp;
  }
  if ((final_array = calloc(sizeof_sort_buf, sizeof(size_t))) == NULL){
    intern_errormesg("Calloc failure");
    ERR = true;
    goto sort_err_jmp;
  }
  if ((temp_path = calloc(sizeof_sort_buf, sizeof(char *))) == NULL){
    intern_errormesg("Calloc failure");
    ERR = true;
    goto sort_err_jmp;
  }

  for (file2find_c = 0; file2find_c < sizeof_file2find; file2find_c++){
    /* Reset the size of sort_array, we're calculating the new size next. */
    sizeof_sort_array = 0;
    for (i = 0; i < sizeof_sort_buf; i++){
      if (strstr(sort_buffer[i], file2find[file2find_c]) != NULL){
	sort_array[sizeof_sort_array] = i;
	sorted_array[sizeof_sort_array] = i;
	sizeof_sort_array++;
      }
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

 sort_err_jmp:
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
  return (ERR == false) ? RF_OPSUCC : ERROR;

} /* intern__findf__sortp() */


