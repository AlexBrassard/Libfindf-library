/*
 *
 *
 *  Libfindf.so  -  Regex manipulation utilities.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <regex.h>

#include <findf.h>
#include "libfindf_private.h"



findf_regex_f* intern__findf__init_regex(char *pattern,
					 bool fr_icase,
					 bool fr_newline,
					 bool fr_ext,
					 bool fr_global,
					 bool fr_boleol)
{
  findf_regex_t *to_init = NULL;

  if ((to_init = malloc(sizeof(findf_regex_f))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }
  if (regcomp(to_init->pattern, pattern,
	      0, NULL,
	      fr_boleol == true ? REG_NEWLINE : 0
	      | fr_icase == true ? REG_ICASE : 0
	      | REG_EXTENDED | REG_NOSUB) != 0){
    intern_errormesg("Failed to compile regex pattern");
    goto cleanup;
  }
  /* 
   * See Regex_functions.txt for explaination.
   * It would be a good idea to make a description here. 
   */
  if (fr_boleol == false
      && fr_newline == true)
    to_init->fr_boleol = true;
  else
    to_init->fr_boleol = false;
  to_init->fr_global = fr_global;
  
  
  return to_init;

 cleanup:

  if (to_init){
    if (to_init->pattern){
      regfree(to_init->pattern);
      to_init->pattern = NULL;
    }
    free(to_init);
    to_init = NULL;
  }
  
}
