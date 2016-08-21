/*
 *
 *
 *  Libfindf.so  -  Regular expression utilities.
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <regex.h>
#include <findf.h>

#include "libfindf_private.h"

/* Dummy */
void *dummy(void *dumdum){
  printf("inside dummy\n\n");
  return ((void*)1);
}

/* Initialize a findf_regex_f object. */
findf_regex_f* intern__findf__init_regex(void)
{
  findf_regex_f *to_init = NULL;

  if ((to_init = malloc(sizeof(findf_regex_f))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }
  if ((to_init->pattern = malloc(sizeof(regex_t))) == NULL){
    intern_errormesg("Malloc");
    goto cleanup;
  }

  to_init->operation = NULL;
  to_init->fre_modif_boleol = false;
  to_init->fre_modif_newline = false;
  to_init->fre_modif_icase = false;
  to_init->fre_modif_ext = false;
  to_init->fre_modif_global = false;
  to_init->fre_op_match = false;
  to_init->fre_op_substitute = false;
  to_init->fre_op_transliterate = false;


  return to_init;

   cleanup:
  if (to_init){
    if (to_init->pattern){
      free(to_init->pattern);
      to_init->pattern = NULL;
    }
    free(to_init);
  }
  return NULL;
}


int intern__findf__free_regex(findf_regex_f* to_free)
{
  /* Set errno if the findf_regex_f pointer is NULL but succeed anyway. */
  if (to_free == NULL){
    errno = ENODATA;
    return RF_OPSUCC;
  }
  /*if (to_free->pattern != NULL){ */
  regfree(to_free->pattern);
  if (to_free->pattern != NULL)
    free(to_free->pattern);
  to_free->pattern = NULL;
    /*}*/
  to_free->operation = NULL;
  free(to_free);
  to_free = NULL;
  return RF_OPSUCC;

}

int intern__findf__free_regarray(findf_regex_f** to_free,
				 size_t numof_patterns)
{
  size_t i = 0;
  /* Set errno if we're passed a NULL valued pointer but succeed anyway. */
  if (to_free == NULL){
    errno = ENODATA;
    return RF_OPSUCC;
  }
  for (i = 0; i < numof_patterns; i++){
    if (to_free[i] != NULL){
      intern__findf__free_regex(to_free[i]);
      to_free[i] = NULL;
    }
  }
  free(to_free);
  to_free = NULL;
  
  return RF_OPSUCC;
}

findf_regex_f** intern__findf__copy_regarray(findf_regex_f **reg_array, size_t numof_elements)
{
  size_t i = 0;
  findf_regex_f **to_init = NULL;
  
  if (reg_array == NULL
      || numof_elements == 0){
    errno = EINVAL;
    return NULL;
  }

  if ((to_init = malloc(numof_elements *sizeof(findf_regex_f*))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }
  for (i = 0; i < numof_elements; i++){
    to_init[i] = intern__findf__init_regex();
    to_init[i]->pattern = reg_array[i]->pattern;
    to_init[i]->operation = reg_array[i]->operation;
    to_init[i]->fre_modif_newline = reg_array[i]->fre_modif_newline;
    to_init[i]->fre_modif_global = reg_array[i]->fre_modif_global;
    to_init[i]->fre_modif_icase = reg_array[i]->fre_modif_icase;
    to_init[i]->fre_modif_ext = reg_array[i]->fre_modif_ext;
    to_init[i]->fre_modif_boleol = reg_array[i]->fre_modif_boleol;
    to_init[i]->fre_op_match = reg_array[i]->fre_op_match;
    to_init[i]->fre_op_substitute = reg_array[i]->fre_op_substitute;
    to_init[i]->fre_op_transliterate = reg_array[i]->fre_op_transliterate;
  }

  return to_init;
}

  

/*
 * Removes the operation, delimiters and modifiers
 * of a given expression and returns the stripped pattern.
 */
char* intern__findf__strip_pattern(char *intern_pattern,
				   char *stripped_pattern,
				   findf_regex_f *freg_object)
{
  bool reach_delimiter = false;
  size_t pattern_char = 0;
  size_t stripped_pattern_c = 0;
  /*  char *stripped_pattern = NULL;*/
  char delimiter;

  /*  if ((stripped_pattern = calloc(FINDF_MAX_PATTERN_LEN, sizeof(char))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
    }*/
  memset(stripped_pattern, 0, FINDF_MAX_PATTERN_LEN);
  /* Fetch the operation. */
  switch(intern_pattern[0]){
  case 'm':
    freg_object->fre_op_match = true;
    pattern_char++;
    break;
  case 's':
    freg_object->fre_op_substitute = true;
    pattern_char++;
    break;
  case 't':
    if (intern_pattern[1] == 'r'){
      freg_object->fre_op_transliterate = true;
      pattern_char = 2;
      break;
    }
    else {
      intern_errormesg("Unknown operation in pattern");
      goto cleanup;
    }
  default:
    /* 
     * Check if the current character is a punctuation mark.
     * If yes, we assume a match operation and use this mark as
     * the pattern's delimiter.
     */
    if (ispunct(intern_pattern[pattern_char])){
      freg_object->fre_op_match = true;
      break;
    }
    else { /* Error */
      intern_errormesg("Unknow operation in pattern");
      goto cleanup;
    }
  }

  /* Set the delimiter. */
  delimiter = intern_pattern[pattern_char];
  pattern_char++;

  while(intern_pattern[pattern_char] != '\0'){
    if (reach_delimiter == false){
      if (intern_pattern[pattern_char] == delimiter){
	stripped_pattern[stripped_pattern_c] = '\0';
	pattern_char++;
	reach_delimiter = true;
	
      }
      else{
	stripped_pattern[stripped_pattern_c] = intern_pattern[pattern_char];
	stripped_pattern_c++;
	pattern_char++;
      }
    }
    else {
      switch(intern_pattern[pattern_char]){
      case 'm':
	freg_object->fre_modif_boleol = true;
	break;
      case 's':
	freg_object->fre_modif_newline = true;
	break;
      case 'i':
	freg_object->fre_modif_icase = true;
	break;
      case 'g':
	freg_object->fre_modif_global = true;
	break;
      case 'x':
	freg_object->fre_modif_ext = true;
	break;
      default:
	intern_errormesg("Syntax error, Unknown modifier in pattern");
	goto cleanup;
      }
      pattern_char++;
    }  
	  
  } 

  /* Make sure we found the second delimiter. */
  if (reach_delimiter == true){
    /* Return the stripped off pattern. */
    return stripped_pattern;
  }
  else {
    /* Or an error ! */
    intern_errormesg("Syntax error: Second pattern delimiter not found.");
    goto cleanup;
  }
	
  
 cleanup:
  if (stripped_pattern){
    free(stripped_pattern);
    stripped_pattern = NULL;
  }
  return NULL;
}
      
      
/* 
 * Parse the Perl-like regex pattern.
 * Modify it into a fully conformant POSIX
 * extended regular expression.
 */

findf_regex_f** intern__findf__parse_patterns(char **patterns,
					      size_t numof_patterns)
{
  size_t i = 0;
  size_t patterns_c = 0;                                 /* Current pattern we're working on. */
  size_t pattern_char = 0;                               /* Used to loop through each characters of each patterns. */
  size_t tmp_pattern_c = 0;                              /* Used to build the pattern to compile, a char at a time. */
  size_t tmp_pattern_len = 0;                            /* Lenght of the unmodified pattern, stripped from op/modif flags. */
  size_t intern_pattern_len = 0;                         /* Lenght of intern_pattern. */
  bool   reach_delimiter = false;                        /* True when we reach the 2nd delimiter of an expression. */
  char   *intern_pattern = NULL;                         /* We copy each patterns in this array. */
  char   *tmp_pattern = NULL;                            /* The pattern without op/modif flags, to be compiled. */
  findf_regex_f **reg_array = NULL;                      /* The array of compiled pattern to return to our caller. */

  /* findf_re() verified the numof_patterns already. Allocate memory.*/
  if ((reg_array = malloc(numof_patterns * sizeof(findf_regex_f*))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }
  for (i = 0; i < numof_patterns; i++){
    if ((reg_array[i] = intern__findf__init_regex()) == NULL){
      intern_errormesg("intern__findf__init_regex");
      goto cleanup;
    }
  }
  if ((intern_pattern = calloc(FINDF_MAX_PATTERN_LEN, sizeof(char))) == NULL){
    intern_errormesg("Calloc");
    goto cleanup;
  }
  if ((tmp_pattern = calloc(FINDF_MAX_PATTERN_LEN, sizeof(char))) == NULL){
    intern_errormesg("Calloc");
    goto cleanup;
  }

  /* Loop over each of caller's patterns.  */
  for (patterns_c = 0; patterns_c < numof_patterns; patterns_c++){
    if (SU_strcpy(intern_pattern, patterns[patterns_c], FINDF_MAX_PATTERN_LEN) == NULL){
      intern_errormesg("SU_strcpy failure");
      goto cleanup;
    }
    /* Strip the pattern of all its operation, delimiter and modifier characters. */
    if (intern__findf__strip_pattern(intern_pattern, tmp_pattern, reg_array[patterns_c]) == NULL){
      intern_errormesg("Could not strip pattern");
      goto cleanup;
    }

    tmp_pattern_len =  strlen(intern_pattern);
    intern_pattern_len = strlen(intern_pattern);
    /* 
     * Loop over each characters of the pattern.
     * Look for escape sequences, comments, spaces etc..
     */
    for (pattern_char = 0, tmp_pattern_c = 0; pattern_char <= intern_pattern_len ; pattern_char++){
      /* 
       * NULL terminate the modified pattern when 
       * a NULL byte is encountered in intern_pattern.
       */
      if (intern_pattern[pattern_char] == '\0') {
	tmp_pattern[tmp_pattern_c] = '\0';
	break;
      }
      /* Look for escape sequences not supported by POSIX. */
      if (intern_pattern[pattern_char] == '\\'){
	switch(intern_pattern[pattern_char + 1]){
	case 'd':
	  /* Make sure the modified pattern stays within POSIX's lenght limits. */
	  if ((tmp_pattern_len + 3) >= FINDF_MAX_PATTERN_LEN){
	    /* 
	     * +3:   '\d' = 2;
	     *       '[0-9]= 5;
	     */ 
	    intern_errormesg("Modified pattern breaking POSIX's lenght limit");
	    goto cleanup;
	  }
	  for (i = 0; FRE_DIGIT_RANGE[i] != '\0'; i++, tmp_pattern_c++)
	    tmp_pattern[tmp_pattern_c] = FRE_DIGIT_RANGE[i];
	  /* Adjust the lenght. */
	  tmp_pattern_len += 3;
	  break;

	case 'D':
	  /* Make sure the modified pattern stays within POSIX's lenght limits. */
	  if ((tmp_pattern_len + 4) >= FINDF_MAX_PATTERN_LEN){
	    /* 
	     * +4:    '\D' = 2;
	     *        '[^0-9] = 6;
	     */
	    intern_errormesg("Modified pattern breaking POSIX's lenght limit");
	    goto cleanup;
	  }
	  for (i = 0; FRE_NOT_DIGIT_RANGE[i] != '\0'; i++, tmp_pattern_c++)
	    tmp_pattern[tmp_pattern_c] = FRE_NOT_DIGIT_RANGE[i];
	  /* Adjust the lenght. */
	  tmp_pattern_len += 4;
	  break;

	  /* Other escape sequences go here ! */

	default:
	  /* 
	   * Ignore escape sequences not listed above, 
	   * either they are invalid or already supported by POSIX.
	   */
	  break;
	}
      } /* If (... == '\\') */

      /* 
       * Check if the '/x' modifier was used,
       * if so, ignore any spaces of any kinds and
       * skip any lines begining by a litteral '#'.
       */
      else if (reg_array[patterns_c]->fre_modif_ext == true){
	if (isspace(intern_pattern[pattern_char])){
	  continue;
	}
	else if (intern_pattern[pattern_char] == '#') {
	  for (; intern_pattern[pattern_char] != '\0'; pattern_char++){
	    /* 
	     * The compiler will very likely optimize-merge this loop 
	     * tecnicaly it's the same as the outer loop. 
	     */
	    __asm__(""); 
	    if (intern_pattern[pattern_char] == '\n')
	      break;
	    else
	      continue;
	  }
	}
      }
      else {
	tmp_pattern[tmp_pattern_c++] = intern_pattern[pattern_char];
      }
      
    }
    /* Ready to compile the pattern. */
    if (regcomp(reg_array[patterns_c]->pattern,
		tmp_pattern,
		(reg_array[patterns_c]->fre_modif_newline == true) ? 0 : REG_NEWLINE |
		(reg_array[patterns_c]->fre_modif_icase == true) ? REG_ICASE : 0 |
		REG_EXTENDED |
		REG_NOSUB) != 0) {
      intern_errormesg("Regcomp failed to compile the pattern.");
      goto cleanup;
    }
    
    /* Plugin the appropriate operation now. */
    if (reg_array[patterns_c]->fre_op_match == true)
      reg_array[patterns_c]->operation = dummy;
    else if (reg_array[patterns_c]->fre_op_substitute == true)
      reg_array[patterns_c]->operation = dummy;
    else if (reg_array[patterns_c]->fre_op_transliterate == true)
      reg_array[patterns_c]->operation = dummy;
    else {
      intern_errormesg("Unknown operation in pattern.");
      goto cleanup;
    }
    
    /* Get next pattern. */
    continue;
  } /* For patterns_c = 0... */

  /* 
   * When the previous loop terminate, 
   * the array of compiled pattern is ready to be used. 
   *
   * Cleanup behind ourself. 
   */
  if(tmp_pattern){
    free(tmp_pattern);
    tmp_pattern = NULL;
  }
  if (intern_pattern){
    free(intern_pattern);
    intern_pattern = NULL;
  }

  return reg_array;
  
  
 cleanup:
  if (reg_array){
    for (i = 0; i < numof_patterns; i++){
      if (reg_array[i]){
	intern__findf__free_regex(reg_array[i]);
	reg_array[i] = NULL;
      }

    }
    free(reg_array);
  }
  if (intern_pattern)
    free(intern_pattern);
  
  if (tmp_pattern)
    free(tmp_pattern);
  
  return NULL;
}
