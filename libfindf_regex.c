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

/* Initialize a findf_regex_f object. */
findf_regex_f* intern__findf__init_regex(char *pattern,     /* The pattern to compile. */
					 bool compw_icase,  /* Compile the pattern with REG_ICASE. */
					 bool compw_newline)/* Compile the pattern with REG_NEWLINE. */
{
  findf_regex_f *to_init = NULL;
  char error_string[F_MAXNAMELEN];
  int reg_errorcode = 0;

  if ((to_init = malloc(sizeof(findf_regex_f))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }
  if ((to_init->pattern = malloc(sizeof(regex_t))) == NULL){
    intern_errormesg("Malloc");
    goto cleanup;
  }
  /* Compile the pattern. */
  if ((reg_errorcode = regcomp(to_init->pattern,
			       pattern,
			       (compw_newline == true) ? REG_NEWLINE : 0 |
			       (compw_icase == true) ? REG_ICASE : 0 |
			       REG_EXTENDED |
			       REG_NOSUB)) != 0){
    
    regerror(reg_errorcode, to_init->pattern, error_string, F_MAXNAMELEN);
    intern_errormesg(error_string);
    goto cleanup;
  }

  return to_init;

 cleanup:
  if (to_init){
    if (to_init->pattern){
      regfree(to_init->pattern);
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
  regfree(to_free->pattern);
  if (to_free->pattern)
    free(to_free->pattern);
  to_free->pattern = NULL;
  to_free->operation = NULL;
  free(to_free);
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
  for (; i < numof_patterns; i++)
    intern__findf__free_regex(to_free[i]);
  free(to_free);

  return RF_OPSUCC;
}


/* 
 * Parse the Perl-like regex pattern.
 * Modify it into a fully conformant POSIX
 * extended regular expression.
 */

findf_regex_f** intern__findf__parse_patterns(char **patterns,
					      size_t numof_patterns)
{
  /* Operation flags. */
  bool fre_op_match = false;                            /* Match operation flag. */
  bool fre_op_substitute = false;                       /* Substitution operation flag. */
  bool fre_op_transliterate = false;                    /* Transliterate operation flag. */

  /* Modifier flags. */
  bool fre_modif_icase = false;      /* Activated with '/i', Case insensitive match. */
  bool fre_modif_global = false;     /* Activated with '/g', "On all matches" flag. */
  bool fre_modif_ext = false;        /* Activated with '/x', Turn on comments '#', skip all kinds of spaces. */
  bool fre_modif_newline = false;    /* Activated with '/s', Match-all character '.' also matches newlines. */
  bool fre_modif_boleol = false;     /* Activated with '/m', ^ and $ anchors to begining/ending of line respectively. */

  /* The delimiter. */
  char delimiter = '\0';                                 /* The caller's pattern delimiter. */
  
  size_t i = 0;
  size_t patterns_c = 0;                                 /* Current pattern we're working on. */
  size_t pattern_char = 0;                               /* Used to loop through each characters of each patterns. */
  size_t helper_pattern_c = 0;                           /* When modifiying the regular expression. */
  size_t helper_pattern_len = 0;                         /* When modifiying the regular expression. */
  size_t tmp_pattern_c = 0;                              /* Used to build the pattern to compile, a char at a time. */
  size_t tmp_pattern_len = 0;                            /* Lenght of the unmodified pattern, stripped from op/modif flags. */
  bool   reach_delimiter = false;                        /* True when we reach the 2nd delimiter of an expression. */
  char   intern_pattern[FINDF_MAX_PATTERN_LEN];          /* We copy each patterns in this array. */
  char   tmp_pattern[FINDF_MAX_PATTERN_LEN];             /* The pattern without op/modif flags, to be compiled. */
  char   helper_pattern[FINDF_MAX_PATTERN_LEN];          /* When we must modify the pattern. */
  findf_regex_f **reg_array = NULL;                      /* The array of compiled pattern to return to our caller. */

  
  
  if ((reg_array = malloc(numof_patterns * sizeof(findf_regex_f*))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }

  /* 
   * Loop through all patterns of the callers given 'patterns',
   * begin by verifying if the pattern is non-NULL.
   */
  for(patterns_c = 0; patterns_c < numof_patterns; patterns_c++){
    if (patterns[patterns_c] == NULL){
      errno = EINVAL;
      intern_errormesg("NULL parameter, adjust numof_patterns or input valid patterns.");
      goto cleanup;
    }
    /* Make a copy of the current patern so we can work on it. */
    if (SU_strcpy(intern_pattern, patterns[patterns_c], FINDF_MAX_PATTERN_LEN) == NULL){
      intern_errormesg("SU_strcpy failure");
      goto cleanup;
    }
    /* Reset all flags and counters now. */
    reach_delimiter = false;
    fre_modif_boleol = false;
    fre_modif_newline = false;
    fre_modif_ext = false;
    fre_modif_global = false;
    fre_modif_icase = false;
    fre_op_match = false;
    fre_op_substitute = false;
    fre_op_transliterate = false;
    tmp_pattern_c = 0;
    tmp_pattern_len = 0;
    helper_pattern_c = 0;
    helper_pattern_len = 0;
    memset(tmp_pattern, 0, FINDF_MAX_PATTERN_LEN);
    memset(helper_pattern, 0, FINDF_MAX_PATTERN_LEN);

    /* Fetch the operation. */
    switch (intern_pattern[pattern_char]){
    case 'm':
      fre_op_match = true;
      pattern_char++;
      break;

    case 's':
      fre_op_substitute = true;
      pattern_char++;
      break;

    case 't':
      pattern_char++;
      if (intern_pattern[pattern_char] != 'r'){
	pthread_mutex_lock(&stderr_mutex);
	fprintf(stderr, "Unknow operation [%c%c] in pattern [%s]\n",
		intern_pattern[pattern_char - 1], intern_pattern[pattern_char],
		intern_pattern);
	pthread_mutex_unlock(&stderr_mutex);
	goto cleanup;
      }
      fre_op_transliterate = true;
      pattern_char++;
      break;

    default:
      /* 
       * We assume a match operation if the pattern begins 
       * with any punctuation mark. That mark will be set as the delimiter.
       */
      if (ispunct(intern_pattern[pattern_char])){
	fre_op_match = true;
	break;
      }
      else {
	pthread_mutex_lock(&stderr_mutex);
	fprintf(stderr, "Unknown operation [%c] in pattern [%s]\n",
		intern_pattern[pattern_char], intern_pattern);
	pthread_mutex_unlock(&stderr_mutex);
	goto cleanup;
      }
    }

    /* Set the delimiter now. */
    delimiter = intern_pattern[pattern_char++];

    /* Fetch the pattern now. */
    while(intern_pattern[pattern_char] != '\0'
	  && pattern_char < FINDF_MAX_PATTERN_LEN){
      if (intern_pattern[pattern_char] != delimiter){
	tmp_pattern[tmp_pattern_c++] = intern_pattern[pattern_char++];
	continue;
      }
      else { 
	/* 
	 * We found the second delimiter. 
	 * NULL terminate the string, raise up the reach_delimiter flag.
	 */
	tmp_pattern[tmp_pattern_c] = '\0';
	pattern_char++;
	reach_delimiter = true;
	/* Save tmp_pattern's lenght. */
	tmp_pattern_len = tmp_pattern_c;
	break;
      }
    }

    /* Make sure we found both delimiter, else it's a syntax error. */
    if (reach_delimiter == false){
      pthread_mutex_lock(&stderr_mutex);
      fprintf(stderr, "Could not find a required second delimiter [%c] in pattern [%s]\n",
	      delimiter, intern_pattern);
      pthread_mutex_unlock(&stderr_mutex);
      goto cleanup;
    }

    /* Fetch the modifier(s). */
    while(intern_pattern[pattern_char] != '\0'){
      switch(intern_pattern[pattern_char]){
      case 'm':
	fre_modif_boleol = true;
	break;
      case 's':
	fre_modif_newline = true;
	break;
      case 'i':
	fre_modif_icase = true;
	break;
      case 'g':
	fre_modif_global = true;
	break;
      case 'x':
	fre_modif_ext = true;
      default:
	pthread_mutex_lock(&stderr_mutex);
	fprintf(stderr,"Unknown modifier [%c] in pattern [%s]\n",
		intern_pattern[pattern_char], intern_pattern);
	pthread_mutex_unlock(&stderr_mutex);
	goto cleanup;
      }
      pattern_char++;
    }
    

  }/* for(patterns_c = 0... */

  return reg_array;

 cleanup:
  if (reg_array){
    intern__findf__free_regarray(reg_array, numof_patterns);
  }
  return NULL;
  
}
