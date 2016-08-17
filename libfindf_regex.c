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
 * Parse the Perl-like regex pattern
 * change it into a fully conformant POSIX
 * extended regular expression.
 */

