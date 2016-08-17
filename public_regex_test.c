#include <stdio.h>
#include <stdlib.h>

#include <findf.h>

int main(void)
{
  findf_param_f *sparam = NULL;
  char **patterns = NULL;
  char **file2find = NULL;
  char **search_roots = NULL;
  size_t numof_file2find = 0;
  size_t i = 0;
  size_t numof_search_roots = 1;
  size_t numof_patterns = 10;
  findf_regex_f **test_regex = NULL;

  /*  if ((file2ind = calloc(numof_file2find, sizeof(char*))) == NULL){
    findf_perror("Calloc failure.");
    goto cleanup;
  }
  for(i = 0; i < numof_file2find; i++){
    if ((file2find[i] = calloc(F_MAXNAMELEN, sizeof(char))) == NULL){
      findf_perror("Calloc failure.");
      goto cleanup;
    }
    }*/

  /* Search's root. */
  if ((search_roots = calloc(numof_search_roots, sizeof(char*))) == NULL){
    findf_perror("Calloc failure");
    goto cleanup;
  }
  for(i = 0; i < numof_search_roots; i++){
    if ((search_roots[i] = calloc(F_MAXPATHLEN, sizeof(char))) == NULL){
      findf_perror("Calloc failure");
      goto cleanup;
    }
  }
  if (SU_strcpy(search_roots[0], "/", F_MAXPATHLEN) == NULL){
    findf_perror("SU_strcpy failure");
    goto cleanup;
  }
  /* Patterns. */
  if ((patterns = calloc(numof_patterns, sizeof(char *))) == NULL){
    findf_perror("Calloc failure");
    goto cleanup;
  }
  for (i = 0; i < numof_patterns; i++){
    if ((patterns[i] = calloc(FINDF_MAX_PATTERN_LEN, sizeof(char))) == NULL){
      findf_perror("Calloc failure");
      goto cleanup;
    }
  }
  if (SU_strcpy(patterns[0], "/time/", FINDF_MAX_PATTERN_LEN) == NULL){
    findf_perror("SU_strcpy failure");
    goto cleanup;
  }
  /* Quick test intern__findf__init_regex(). */
  if ((test_regex = malloc(10 * sizeof(findf_regex_f*))) == NULL){
    findf_perror("Malloc");
    goto cleanup;
  }
  for (i = 0; i < numof_patterns; i++){
    if ((test_regex[i] = intern__findf__init_regex(patterns[0], false, true)) == NULL){
      findf_perror("intern__findf__init_regex");
      goto cleanup;
    }
  }

  /*
  if ((sparam = findf_init_param(file2find, search_roots,
				 numof_file2find, numof_search_roots,
				 0, IDBFS, SORTP)) == NULL){
    findf_perror("Failed to initialize a search parameter");
    goto cleanup;
  }
  
  if (findf_re(sparam, patterns, numof_patterns) == -1){
    findf_perror("Failed to execute the search");
    goto cleanup;
    }*/
  
 cleanup:
  if (file2find){
    for (i = 0; i < numof_file2find; i++){
      if (file2find[i]){
	free(file2find[i]);
	file2find[i] = NULL;
      }
    }
    free(file2find);
    file2find = NULL;
  }
  if (search_roots){
    for (i = 0; i < numof_search_roots; i++){
      if (search_roots[i]){
	free(search_roots[i]);
	search_roots[i] = NULL;
      }
    }
    free(search_roots);
    search_roots = NULL;
  }
  if (patterns){
    for (i = 0; i < numof_patterns; i++){
      if(patterns[i]){
	free(patterns[i]);
	patterns[i] = NULL;
      }
    }
    free(patterns);
    patterns = NULL;
  }
  if (sparam){
    findf_destroy_param(sparam);
    sparam = NULL;
  }
  if (test_regex){
    intern__findf__free_regarray(test_regex, numof_patterns);
    test_regex = NULL;
  }

  return -1;
}
