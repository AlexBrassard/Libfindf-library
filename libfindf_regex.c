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
#include <ctype.h

#include <findf.h>
#include "libfindf_private.h"



findf_regex_f* intern__findf__init_regex(char *pattern,
					 bool fr_icase,
					 bool fr_newline,
					 bool fr_global,
					 bool fr_boleol)
{
  findf_regex_f *to_init = NULL;

  if ((to_init = malloc(sizeof(findf_regex_f))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }
  /*
   * Compile/execution Flags: 
   *
   * DEFAULT (no modifiers):
   * .    must not match newline.
   * ^ $  must match begining of string and end of string respectively.
   * \< \> in POSIX.
   * Compilation flags: REG_NEWLINE | REG_EXTENDED | REG_NOSUB
   * Execution flags:   REG_NOTBOL | REG_NOTEOL
   * Modifs? : Change an expression begining like: m/^...   into:  \<...
   * Change an expressiong ending like:   ...$/   into:  ...\>
   * 
   * If /m is used alone, (^ $ begining/ending of line)
   * Compilation flags: REG_NEWLINE | REG_EXTENDED | REG_NOSUB
   * Execution flags:   --
   * Modifs? :  --
   * 
   * if /s is used alone, (. matches newline)
   * Compilation flags: REG_EXTENDED | REG_NOSUB
   * Execution flags:   REG_NOTBOL | REG_NOTEOL
   * Modifs? Change an expression begining like: m/^...   into:  \<...       not sure
   * Change an expressiong ending like:   ...$/   into:  ...\>       not sure
   * 
   * if /ms are used together
   * Compilation flags: REG_EXTENDED | REG_NOSUB | REG_NEWLINE
   * Execution flags:   --
   */
  if (regcomp(to_init->pattern, pattern,
	      ((fr_boleol == true) ? REG_NEWLINE : 0)
	      | ((fr_icase == true) ? REG_ICASE : 0)
	      | REG_EXTENDED | REG_NOSUB) != 0){
    intern_errormesg("Failed to compile regex pattern");
    goto cleanup;
  }
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
  return NULL;
}


/* Release resources of a single findf_regex_f object. */
int intern__findf__free_regex(findf_regex_f *to_free)
{
  /* Set errno to 'no data' but return success anyway. */
  if (to_free == NULL){
    errno = ENODATA;
    return RF_OPSUCC;
  }
  regfree(to_free->pattern);
  to_free->pattern = NULL;
  to_free->operation = NULL;
  return RF_OPSUCC;
}

int intern__findf__destroy_regarray(findf_regex_f **reg_array,
				    size_t numof_patterns)
{
  size_t i = 0;
  
  /* We can tolerate being passed a NULLed out array. */
  if (reg_array == NULL){
    errno = ENODATA;
    return RF_OPSUCC;
  }
  /* But not being passed faulty numbers. */
  if (numof_patterns == 0
      || numof_patterns > FINDF_MAX_PATTERNS){
    errno = EINVAL;
    return ERROR;
  }

  for (i = 0; i < numof_patterns; i++){
    if (reg_array[i]){
      intern__findf__free_regex(reg_array[i]);
      reg_array[i] = NULL;
    }
  }
  free(reg_array);

  return RF_OPSUCC;
}



/* 
 * Perl like regular expression form:  operation delimiter expression delimiter modifier
 * ex: m/^t.*s$/i             Match any string begining with t/T, 
 *                            folowed by any characters, any amount of times, 
 *				ending with s/S.
 *		     
 *     s/(\.txt)$/\.hula/g'   On string ending in .txt, subtitute the .txt by
 *	                        .hula and do it on all found matches. 
 /*


/* Parse an array of regex patterns, returns an array of findf_regex patterns. */
findf_regex_f** intern__findf__parse_pattern(char **patterns,
					     size_t numof_patterns)
{

  /* Operation flags */
  bool fr_match = false;                /* Matching operation. */
  bool fr_substitute = false;           /* Substitution operation. */

  /* Modifier flags */
  bool fr_icase = false;                /* Case insensitivity flag. */
  bool fr_global = false;               /* "On all match" flag. */
  bool fr_newline = false;              /* Match-all character matches newline. */
  bool fr_boleol = false;               /* Begining/ending of strings matches begining/ending of lines. */
  bool fr_ext = false;                  /*  
					 * Extended pattern flag, spaces are removed, 
					 * lines begining by # ignored.
					 */

  /* Delimiter type */
  char delimiter = '\0';                 /* The pattern delimiter. */

  size_t i = 0;
  size_t patterns_count = 0;             /* Current pattern we're working on. */
  size_t pattern_char = 0;               /* Used to look through each characters of each patterns. */
  size_t tmp_pattern_c = 0;              /* Used to build the pattern to compile. */
  bool inside_comment = false;           /* True when we're inside a pattern's comment. */
  bool pattern_delimited = false;        /* True when we're sure 2 delimiters have been encountered. */
  char intern_pattern[FINDF_MAX_PATTERN_LEN];   /* We copy each patterns in this array. */
  char tmp_pattern[FINDF_MAX_PATTERN_LEN];      /* The pattern to compile. */
  char helper_pattern[FINDF_MAX_PATTERN_LEN];   /* When we must modify the pattern. */
  findf_regex_f **reg_array = NULL;      /* The array of compiled patterns to return to our caller. */


  /* 
   * No parameter checks (almost none).
   * Be extra careful if using this function directly.
   */

  if ((reg_array = malloc(numof_patterns * sizeof(findf_regex_f*))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }

  /* 
   * Loop through each patterns, make sure
   * their lenght is reasonable and force
   * null terminate each of them.
   */
  for (patterns_count = 0; patterns_count < numof_patterns; patterns_count++){
    if (SU_strcpy(intern_pattern, patterns[patterns_count], FINDF_MAX_PATTERN_LEN) == NULL){
      intern_errormesg("SU_strcpy failure");
      goto cleanup;
    }
    memset(tmp_pattern, 0, FINDF_MAX_PATTERN_LEN);

    /* Fetch the pattern's operation. */
    switch (intern_pattern[pattern_char]){
    case 'm':
      fr_match = true;
      pattern_char++;
      break;
    case 's':
      fr_substitute = true;
      pattern_char++;
      break;
    case (ispunct(intern_pattern[pattern_char])):
      /* Assume a match for patterns of the like:  '/.../' */
      fr_match = true;
      break;
    default:
      pthread_mutex_lock(&stderr_mutex);
      fprintf(stderr,"Invalid operation operator in pattern. See man 3 findf_re\n\n");
      pthread_mutex_unlock(&stderr_mutex);
      goto cleanup;
    }

    /* The current character is the pattern delimiter. */
    delimiter = intern_pattern[pattern_char];
    pattern_char++;

    /* Save the pattern itself in tmp_patter. */
    while (pattern_char < FINDF_MAX_PATTERN_LEN
	   && intern_pattern[pattern_char] != '\0'){
      if (intern_pattern[pattern_char] != delimiter){
	if (fr_ext == true){ /* Extended pattern. */
	  /* Comments terminate on newlines, check for it now. */
	  if (inside_comment == true){
	    if (intern_pattern[pattern_char] == '\n'){
	      inside_comment = false;
	      continue;
	    }
	    continue;
	  }
	  if (isspace(intern_pattern[pattern_char])){
	    continue;
	  }
	  if (intern_pattern == '#'){
	    /* Everything till the end of line is discarded. */
	    inside_comment = true;
	    continue;
	  }
	} /* if (fr_ext == true) */
	tmp_pattern[tmp_pattern_c] = intern_pattern[pattern_char];
	tmp_pattern_c++;
	pattern_char++;
	continue;
      }
      else { 
	/* 
	 * We reached the other delimiter. 
	 * NULL terminate the pattern.
	 */
	tmp_pattern[tmp_pattern_c] = '\0';
	pattern_char++;
	pattern_delimited = true;
	break; /* Only valid exit point. */
      }
    } /* while(pattern_char < ...) */

    if (pattern_delimited == false){     /* Oops.. */
      pthread_mutex_lock(&stderr_mutex);
      fprintf(stderr, "Invalid pattern.\nCould not find second delimiter [%c] in pattern [%s]\n\n",
	      delimiter, intern_pattern);
      pthread_mutex_unlock(&stderr_mutex);
      goto cleanup;
    }

    
  } /* for(patterns_count...) */


  return reg_array; /* Success ! */
  
 cleanup:
  if (reg_array){
    intern__findf__destroy_regarray(reg_array);
    free(reg_array);
    reg_array = NULL;
  }

  return NULL;   /* Oops */


  
}
