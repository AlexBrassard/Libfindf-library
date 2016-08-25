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


/* Allocate memory to a single findf_regex_f object. */
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
  if ((to_init->pat_storage = malloc(sizeof(findf_opstore_f))) == NULL){
    intern_errormesg("Malloc");
    goto cleanup;
  }
  if ((to_init->pat_storage->pattern1 = calloc(FINDF_MAX_PATTERN_LEN, sizeof(char))) == NULL){
    intern_errormesg("Calloc");
    goto cleanup;
  }
  if ((to_init->pat_storage->pattern2 = calloc(FINDF_MAX_PATTERN_LEN, sizeof(char))) == NULL){
    intern_errormesg("Calloc");
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
    if (to_init->pat_storage != NULL){
      if (to_init->pat_storage->pattern1 != NULL) {
	free(to_init->pat_storage->pattern1);
	to_init->pat_storage->pattern1 = NULL;
      }
      if (to_init->pat_storage->pattern2 != NULL){
	free(to_init->pat_storage->pattern2);
	to_init->pat_storage->pattern2 = NULL;
      }
      free(to_init->pat_storage);
      to_init->pat_storage = NULL;
    }
    free(to_init);
  }
  return NULL;
}


/* Deallocate memory of a single findf_regex_f object. */
int intern__findf__free_regex(findf_regex_f* to_free)
{
  /* Set errno if the findf_regex_f pointer is NULL but succeed anyway. */
  if (to_free == NULL){
    errno = ENODATA;
    return RF_OPSUCC;
  }
  //  regfree(to_free->pattern);
  if (to_free->pattern != NULL)
    free(to_free->pattern);
  to_free->pattern = NULL;
  if (to_free->pat_storage != NULL){
    if (to_free->pat_storage->pattern1 != NULL){
      free(to_free->pat_storage->pattern1);
      to_free->pat_storage->pattern1 = NULL;
    }
    if (to_free->pat_storage->pattern2 != NULL){
      free(to_free->pat_storage->pattern2);
      to_free->pat_storage->pattern2 = NULL;
    }
    free(to_free->pat_storage);
    to_free->pat_storage = NULL;
  }
  to_free->operation = NULL;
  free(to_free);
  to_free = NULL;
  return RF_OPSUCC;

}

/* Deallocate memory for an array of findf_regex_f* objects. */
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



/* 
 * MACRO. Add current token to the apropriate stack.
 * Not needed, just hoping to make things clearer.
 */
#define intern__findf__push(token, stack, tos) do { \
    stack[*tos] = token;			    \
    ++(*tos);					    \
  } while (0);

/* MACRO. Check validity of the pattern's modifier(s). */
#ifndef intern__findf__validate_modif
# define intern__findf__validate_modif(modifiers) int i = 0;		\
  while(modifiers[i] != '\0') {						\
    switch(modifiers[i]) {						\
    case 'm':								\
      freg_object->fre_modif_boleol = true;				\
      break;								\
    case 's':								\
      freg_object->fre_modif_newline = true;				\
      break;								\
    case 'i':								\
      freg_object->fre_modif_icase = true;				\
      break;								\
    case 'g':								\
      freg_object->fre_modif_global = true;				\
      break;								\
    case 'x':								\
      freg_object->fre_modif_ext = true;				\
      break;								\
    default:								\
      intern_errormesg("Unknown modifier in pattern");			\
      return ERROR;							\
    }									\
    i++;								\
  }
#endif

/* verify how you call it. */
int intern__findf__validate_esc_seq(char token,
				    char *buffer,
				    size_t *buf_ind,
				    size_t *buf_len)
{

  size_t i = 0;								

  switch(token){							
  case 'd':								
    /*
     * + 3:    '\d'    = 2
     *         '[0-9]' = 5
     */
    if ((buffer_len + 3) >= FINDF_MAX_PATTERN_LEN) {
      pthread_mutex_lock(&stderr_mutex);
      fprintf(stderr, "Modified pattern breaks POSIX pattern's lenght limit (%d)\nChanging '\\d' into '[0-9]\n\n",
	      FINDF_MAX_PATTERN_LEN);
      pthread_mutex_unlock(&stderr_mutex);
      return ERROR;
    }					
    while (FRE_DIGIT_RANGE[i] != '\0'){
      buffer[(*buf_ind)++] = FRE_DIGIT_RANGE[i++];
    }			
    *buf_len += 3;
    break;
  case 'D':
    /* 
     * + 4:    '\D'     = 2
     *         '[^0-9]' = 6
     */
    if ((buffer_len + 4) >= FINDF_MAX_PATTERN_LEN){
      pthread_mutex_lock(&stderr_mutex);
      fprintf(stderr, "Modified pattern breaks POSIX pattern's lenght limit (%d)\nChanging '\\D' into '[^0-9]'\n\n",
	      FINDF_MAX_PATTERN_LEN);
      pthread_mutex_unlock(&stderr_mutex);
      return ERROR;
    }
    while(FRE_NOT_DIGIT_RANGE[i] != '\0'){
      buffer[(*buf_ind)++] = FRE_NOT_DIGIT_RANGE[i++];
    }
    *buf_len += 4;
    break
  default:
      /* Assume supported escape sequence. */
      break;
  }
} /* intern__findf__validate_esc_seq() */

		     
char *intern__findf__perl_to_posix(char *pattern,
				   findf_regex_f *freg_object)
{
  size_t token_ind = 0;
  size_t npattern_tos = 0;
  size_t mod_pattern_len = (non_mod_pattern_len = strnlen(pattern, FINDF_MAX_PATTERN_LEN));
  char *new_pattern = NULL;



#ifndef PPTOKEN
# define PPTOKEN pattern[token_ind]
#endif

  if ((new_pattern = calloc(FINDF_MAX_PATTERN_LEN, sizeof(char))) == NULL){
    intern_errormesg("Calloc");
    return ERROR;
  }
  
  /* 
   * Loop over each characters of pattern,
   * convert unsupported Perl-like escape sequence
   * into POSIX compatible constructs.
   */
  while (1){
    if (PPTOKEN == '\0'){
      intern__findf__push('\0', new_pattern, &npattern_tos);
      break;
    }
    else if (PPTOKEN == '\\'){
      /* Get next token and verify the sequence. */
      token_ind++;
      if (intern__findf__validate_esq_sequence(PPTOKEN, new_pattern,
					       &pattern_tos, &mod_pattern_len) != RF_OPSUCC){
	intern_errormesg("Failed to validate an escape sequence");
	goto failure;
      }
    }

    /* If pattern is an extended pattern, remove spaces and comments. */
    else if (freg_object->fre_modif_ext == true) {
      ;
    }

    /* Valid token, add it to new_pattern. */
    else {
      ;
    }
    
  } /* while(1) */
      


    return RF_OPSUCC;

 failure:
    if (new_pattern != NULL){
      free(new_pattern);
      new_pattern = NULL;
    }
    return ERROR;
#undef PPTOKEN
}


/* Parse a match-style pattern. */  
int intern__findf__parse_match(char *pattern,
			       size_t token_ind,
			       findf_regex_f *freg_object)
{
  size_t freg_object_tos = 0;
  size_t modifiers_tos = 0;
  size_t numof_seen_delimiter = 1;
  char modifiers[FINDF_MAX_PATTERN_LEN];
  
#ifndef MTOKEN
# define MTOKEN pattern[token_ind]
#endif

  memset(modifiers, 0, FINDF_MAX_PATTERN_LEN);
  while (1) {
    /* 
     * Check if we reach the end of pattern,
     * verify we've encoutered the required numbers
     * of pattern delimiters.
     */
    if (MTOKEN == '\0'){
      if (numof_seen_delimiter == FRE_MATCH_EXPECT_DELIMITER){
	/* NULL terminate the strings. */
	intern__findf__push('\0', freg_object->pat_storage->pattern1, &freg_object_tos);
	intern__findf__push('\0', modifiers, &modifiers_tos);
	break;
      }
      else {
	/* Error, we should have the exact number of expected delimiters. */
	pthread_mutex_lock(&stderr_mutex);
	fprintf(stderr, "ERROR: Expected exactly [%d] delimiters;\nSeen [%zu] delimiters\n\n",
		FRE_MATCH_EXPECT_DELIMITER, numof_seen_delimiter);
	pthread_mutex_unlock(&stderr_mutex);
	return ERROR;
      }
    }
    /* 
     * Check if MTOKEN is the pattern's delimiter 
     * and increment the counter. 
     */
    if (MTOKEN == freg_object->delimiter){
      numof_seen_delimiter++;
      token_ind++;
      continue;
    }
    else {
      /* 
       * If the expected number of delimiters is not reach,
       * MTOKEN is part of the regex pattern.
       */
      if (numof_seen_delimiter < FRE_MATCH_EXPECT_DELIMITER){
	intern__findf__push(MTOKEN, freg_object->pat_storage->pattern1, &freg_object_tos);
      }
      /* Else it must be a modifier. */
      else if (numof_seen_delimiter == FRE_MATCH_EXPECT_DELIMITER){
	intern__findf__push(MTOKEN, modifiers, &modifiers_tos);
      }
      else { /* This should never happen. */
	intern_errormesg("numof_seen_delimiter does not match expectations.");
	return ERROR;
      }
    }
    token_ind++;
  } /* while (1) */

  /* Make sure all gathered modifiers are supported. */
  intern__findf__validate_modif(modifiers);
  

  return RF_OPSUCC;

#undef MTOKEN
} /* intern__findf__parse_match() */


/* Parse a substitution-style pattern. */
int intern__findf__parse_substitute(char *pattern,
				    size_t token_ind,
				    findf_regex_f *freg_object)
{
  size_t pattern_tos = 0;
  size_t substitute_tos = 0;
  size_t modifiers_tos = 0;
  size_t numof_seen_delimiter = 1;
  char modifiers[FINDF_MAX_PATTERN_LEN];
  
#ifndef STOKEN
# define STOKEN pattern[token_ind]
#endif

  memset(modifiers, 0, FINDF_MAX_PATTERN_LEN);

  
  while(1){
    /* 
     * Make sure we've seen all required delimiters
     * when the current token is a NULL byte.
     */
    if (STOKEN == '\0') {
      if (numof_seen_delimiter == FRE_SUBST_EXPECT_DELIMITER){
	/* NULL terminate all strings. */
	intern__findf__push('\0', freg_object->pat_storage->pattern1, &pattern_tos);
	intern__findf__push('\0', freg_object->pat_storage->pattern2, &substitute_tos);
	intern__findf__push('\0', modifiers, &modifiers_tos);
	break;
      }
      else {
	/* Error, we should have the exact number of expected delimiters. */
	pthread_mutex_lock(&stderr_mutex);
	fprintf(stderr, "ERROR: Expected exactly [%d] delimiters;\nSeen [%zu] delimiters\n\n",
		FRE_MATCH_EXPECT_DELIMITER, numof_seen_delimiter);
	pthread_mutex_unlock(&stderr_mutex);
	return ERROR;
      }
    }
    else if (STOKEN == freg_object->delimiter){
      numof_seen_delimiter++;
    }
    else {
      /* 
       * If token is in between 1st and 2nd delimiter push the
       * token in pattern1, (pattern to match)
       * if token is in between 2nd and 3rd delimiter push then
       * token in pattern2, (pattern to substitute match with)
       * else push the token in modifiers.
       */
      if (numof_seen_delimiter < (FRE_SUBST_EXPECT_DELIMITER - 1)) {
	intern__findf__push(STOKEN, freg_object->pat_storage->pattern1, &pattern_tos);
      }
      else if (numof_seen_delimiter < FRE_SUBST_EXPECT_DELIMITER){
	intern__findf__push(STOKEN, freg_object->pat_storage->pattern2, &substitute_tos);
      }
      else {
	intern__findf__push(STOKEN, modifiers, &modifiers_tos);
      }
    }
    ++token_ind;
  } /* while(1) */

  /* We must validate the pattern's modifier(s). */
  intern__findf__validate_modif(modifiers);
  
  return RF_OPSUCC;
#undef STOKEN

} /* intern__findf__parse_substitute() */


/* Initialize the regex parsing operation. */
findf_regex_f** intern__findf__init_parser(char **patterns,
					   size_t numof_patterns)
{
  size_t i = 0;
  size_t patterns_c = 0;                /* Current pattern. */
  size_t token_ind = 0;                 /* Index of the current token. */
  findf_regex_f **reg_array = NULL;     /* For the findf_param_f->reg_array field of findf_re's parameter. */

  findf_regex_f *freg_object = NULL;    /* Convinience, not needed. */


  /* Allocate memory for our internal array of findf_regex_f objects. */
  if ((reg_array = malloc(numof_patterns * sizeof(findf_regex_f*))) == NULL){
    intern_errormesg("Malloc failure");
    return NULL;
  }
  for (i = 0; i < numof_patterns; i++){
    if ((reg_array[i] = intern__findf__init_regex()) == NULL){
      intern_errormesg("Intern__findf__init_regex failure");
      goto failure;
    }
  }


  /* 
   * Go over each patterns.
   * Begin by fetching the current pattern's operation. 
   */
  for (patterns_c = 0; patterns_c < numof_patterns; patterns_c++){
    token_ind = 0;
    freg_object = reg_array[patterns_c];

#ifndef ITOKEN
# define ITOKEN patterns[patterns_c][token_ind]
#endif

    /* Fetch the operation */
    switch(ITOKEN){
    case 'm': /* Match m/, / */
      freg_object->fre_op_match = true;
      /* Adjust token */
      token_ind++;
      break;
    case 's': /* Substitution s/ */
      freg_object->fre_op_substitute = true;
      token_ind++;
      break;
    case 't': /* Transliteration tr/ */
      if (patterns[patterns_c][token_ind + 1] == 'r'){
	freg_object->fre_op_transliterate = true;
	token_ind += 2;
	break;
      }
      else {
	intern_errormesg("Unkown operation in pattern");
	goto failure;
      }
    default:
      if (ispunct(ITOKEN)){
	/* Assume a match operation. */
	freg_object->fre_op_match = true;
	break;
      }
      intern_errormesg("Unknown operation in pattern");
      goto failure;
    }  

    /* 
     * Make sure the current token is a punctuation mark,
     * if so, this token is the pattern's delimiter.
     */
    if (ispunct(ITOKEN)){
      freg_object->delimiter = ITOKEN;
      ++token_ind;
    }
    else {
      /* No delimiter, syntax error. */
      intern_errormesg("No delimiter found in pattern");
      goto failure;
    }
     
    /* Parse the remaining of the pattern. */
    if (freg_object->fre_op_match == true){
      if (intern__findf__parse_match(patterns[patterns_c],
				     token_ind,
				     reg_array[patterns_c]) != RF_OPSUCC){
	intern_errormesg("Intern__findf__parse_match failure");
	goto failure;
      }
    }
    else if (freg_object->fre_op_substitute == true){
      if (intern__findf__parse_substitute(patterns[patterns_c],
					  token_ind,
					  reg_array[patterns_c]) != RF_OPSUCC){
	intern_errormesg("Intern__findf__parse_substitute");
	goto failure;
      }
    }
    else if (freg_object->fre_op_transliterate == true){
      if (intern__findf__parse_substitute(patterns[patterns_c],
					  token_ind,
					  reg_array[patterns_c]) != RF_OPSUCC){
	intern_errormesg("Intern__findf__parse_substitute");
	goto failure;
      }
    }
    else { 
      intern_errormesg("Unknown operation in pattern");
      goto failure;
    }

    /* 
     * Convert the stripped off Perl-like pattern into
     * a fully POSIX ERE conformant pattern.
     */

    
  } /* for(patterns_c = 0...(each patterns)) */
  


  return reg_array;

 failure:
  if (freg_object != NULL)
    freg_object = NULL;
  if (reg_array != NULL){
    for (i = 0; i < numof_patterns; i++){
      if(reg_array[i] != NULL) {
	intern__findf__free_regex(reg_array[i]);
	reg_array[i] = NULL;
      }
    }
    free(reg_array);
    reg_array = NULL;
  }
  return NULL;
#undef ITOKEN
}


