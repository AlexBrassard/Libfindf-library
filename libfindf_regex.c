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
  size_t i = 0;

  if ((to_init = malloc(sizeof(findf_regex_f))) == NULL){
    intern_errormesg("Malloc");
    return NULL;
  }
  /* We need exactly 2 regex_t* objects. */
  if ((to_init->pattern = malloc(2 * sizeof(regex_t*))) == NULL){
    intern_errormesg("Malloc");
    goto cleanup;
  }
  for (i = 0; i < 2; i++){
    if ((to_init->pattern[i] = malloc(sizeof(regex_t))) == NULL){
      intern_errormesg("Malloc");
      goto cleanup;
    }
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
  to_init->fre_p1_compiled = false;
  to_init->fre_p2_compiled = false;


  return to_init;


 cleanup:
  if (to_init){
    if (to_init->pattern){
      for (i = 0; i < 2; i++){
	free(to_init->pattern[i]);
	to_init->pattern[i] = NULL;
      }
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
  size_t i = 0;
  
  /* Set errno if the findf_regex_f pointer is NULL but succeed anyway. */
  if (to_free == NULL){
    errno = ENODATA;
    return RF_OPSUCC;
  }
  if (to_free->pattern != NULL){
    if (to_free->fre_p1_compiled == true){
      regfree(to_free->pattern[0]);
    }
    if (to_free->fre_p2_compiled == true) {
      regfree(to_free->pattern[1]);
    }
    for (i = 0; i < 2; i++){
      free(to_free->pattern[i]);
      to_free->pattern[i] = NULL;
    }
    free(to_free->pattern);
    to_free->pattern = NULL;
  }
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


/* Verify the validity of pattern modifiers. */
int intern__findf__validate_modif(char *modifiers,
				  findf_regex_f *freg_object)
{
  size_t i = 0;								
  while(modifiers[i] != '\0') {						
    switch(modifiers[i]) {						
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
      intern_errormesg("Unknown modifier in pattern");			
      return ERROR;							
    }									
    i++;								
  }
  return RF_OPSUCC;
}


/* Skips over comment lines. */
void intern__findf__skip_comments(char *pattern,
				  size_t *cur_ind,
				  size_t *pattern_len)
{									
  int i = 0;								
  while (pattern[(*cur_ind)] != '\n' && pattern[(*cur_ind)] != '\0') { 
    (*cur_ind)++;							
    i++;								
  }									
  if (i > 0){								
    /* Substract the amount of tokens we removed from the original lenght. */ 
    (*pattern_len) -= i;						
  }									
}



/* Check for escape sequences not supported by POSIX. */
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
    if (((*buf_len) + 3) >= FINDF_MAX_PATTERN_LEN) {
      pthread_mutex_lock(&stderr_mutex);
      fprintf(stderr, "Modified pattern breaks POSIX pattern's lenght limit (%d)\nChanging '\\d' into '[0-9]\n\n",
	      FINDF_MAX_PATTERN_LEN);
      pthread_mutex_unlock(&stderr_mutex);
      return ERROR;
    }					
    while (FRE_DIGIT_RANGE[i] != '\0'){
      buffer[(*buf_ind)++] = FRE_DIGIT_RANGE[i++];
    }			
    (*buf_len) += 3;
    break;

  case 'D':
    /* 
     * + 4:    '\D'     = 2
     *         '[^0-9]' = 6
     */
    if (((*buf_len) + 4) >= FINDF_MAX_PATTERN_LEN){
      pthread_mutex_lock(&stderr_mutex);
      fprintf(stderr, "Modified pattern breaks POSIX pattern's lenght limit (%d)\nChanging '\\D' into '[^0-9]'\n\n",
	      FINDF_MAX_PATTERN_LEN);
      pthread_mutex_unlock(&stderr_mutex);
      return ERROR;
    }
    while(FRE_NOT_DIGIT_RANGE[i] != '\0'){
      buffer[(*buf_ind)++] = FRE_NOT_DIGIT_RANGE[i++];
    }
    (*buf_len) += 4;
    break;
  default:
    /* Assume supported escape sequence. */
    break;
  }

  return RF_OPSUCC;
} /* intern__findf__validate_esc_seq() */




/* Convert Perl-like regex pattern into POSIX conformant regex pattern. */
int intern__findf__perl_to_posix(char *pattern,
				 findf_regex_f *freg_object)
{
  bool pattern2_converted = false;          /* True when the 2nd pattern of a substitution has been converted. */
  size_t token_ind = 0;                     /* Index of the current token we're working on. */
  size_t npattern_tos = 0;                  /* The converted pattern tos. */
  size_t non_mod_pattern_len = 0;           /* Lenght of the unmodifier Perl-like regex pattern. */
  size_t mod_pattern_len = 0;               /* Lenght of the converted POSIX regex pattern. */
  char *new_pattern = NULL;

  mod_pattern_len = (non_mod_pattern_len = strnlen(pattern, FINDF_MAX_PATTERN_LEN));
  
#ifndef PPTOKEN
# define PPTOKEN pattern[token_ind]
#endif

  /* Why not a local variable ? */
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
      if (intern__findf__validate_esc_seq(PPTOKEN, new_pattern,
					  &npattern_tos, &mod_pattern_len) != RF_OPSUCC){
	intern_errormesg("Failed to validate an escape sequence");
	goto failure;
      }
    }

    /* If pattern is an extended pattern, remove spaces and comments. */
    else if (freg_object->fre_modif_ext == true) {
      if (isspace(PPTOKEN)) {
	; /* Get next token. */
      }
      else if (PPTOKEN == '#') {
	intern__findf__skip_comments(pattern, &token_ind, &mod_pattern_len);
      }
      /* It must be a valid character. */
      else {
	intern__findf__push(PPTOKEN, new_pattern, &npattern_tos);
      }
    }

    /* More filters will go here. */
    
    /* Valid token, add it to new_pattern. */
    else {
      intern__findf__push(PPTOKEN, new_pattern, &npattern_tos);
    }
    token_ind++;
  } /* while(1) */
      

  /* Copy the new pattern into the caller's given pattern. */
  if(SU_strcpy(pattern, new_pattern, FINDF_MAX_PATTERN_LEN) == NULL){
    intern_errormesg("SU_strcpy");
    goto failure;
  }
  /* Free new_pattern. */
  free(new_pattern);
  new_pattern = NULL;
  
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
  if (intern__findf__validate_modif(modifiers, freg_object) != RF_OPSUCC){
    intern_errormesg("Intern__findf__validate_modif");
    return ERROR;
  }
  

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
       * if token is in between 2nd and 3rd delimiter push the
       * token in pattern2, (pattern to substitute matches with)
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
  if(intern__findf__validate_modif(modifiers, freg_object) != RF_OPSUCC){
    intern_errormesg("Intern__findf__validate_modif");
    return ERROR;
  }

  
  return RF_OPSUCC;
#undef STOKEN

} /* intern__findf__parse_substitute() */


/* Compile a regex pattern. */
int intern__findf__compile_pattern(findf_regex_f *freg_object)
{
  /* No matter the operation, always compile the first pattern. */
  if (regcomp(freg_object->pattern[0],
	      freg_object->pat_storage->pattern1,
	      (freg_object->fre_modif_icase == true) ? REG_ICASE : 0 |
	      (freg_object->fre_modif_newline == true) ? 0 : REG_NEWLINE |
	      REG_EXTENDED |
	      REG_NOSUB) != 0) {
    intern_errormesg("Regcomp");
    return ERROR;
  }
  freg_object->fre_p1_compiled = true; /* To ease freeing */
  /* If it's a substitution, compile the second pattern as well. */
  if (freg_object->fre_op_substitute == true){
    if (regcomp(freg_object->pattern[1],
		freg_object->pat_storage->pattern2,
		(freg_object->fre_modif_icase == true) ? REG_ICASE : 0 |
		(freg_object->fre_modif_newline == true) ? 0 : REG_NEWLINE |
		REG_EXTENDED |
		REG_NOSUB) != 0) {
      intern_errormesg("Regcomp");
      return ERROR;
    }
    freg_object->fre_p2_compiled = true; /* To ease freeing. */
  }

  return RF_OPSUCC;
}

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
    else if (freg_object->fre_op_substitute == true
	     || freg_object->fre_op_transliterate == true){
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
     * a fully POSIX ERE conformant pattern only if it's not
     * for a transliteration operation.
     */
    if (freg_object->fre_op_transliterate == false){
      if (intern__findf__perl_to_posix(freg_object->pat_storage->pattern1,
				       freg_object) != RF_OPSUCC){
	intern_errormesg("Intern__findf__perl_to_posix");
	goto failure;
      }
      /* 
       * If the object's operation is a substitution,
       * send the object's second pattern into _perl_to_posix().
       */
      if (freg_object->fre_op_substitute == true)	{
	if (intern__findf__perl_to_posix(freg_object->pat_storage->pattern2,
					 freg_object) != RF_OPSUCC){
	  intern_errormesg("Intern__findf__perl_to_posix");
	  goto failure;
	}
      }
      /* Compile the pattern. */
      if (intern__findf__compile_pattern(freg_object) != RF_OPSUCC){
	intern_errormesg("Intern__findf__compile_pattern");
	goto failure;
      }
    }
    
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


