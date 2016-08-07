/*
 *
 *
 *	Libfindf - File searching made easy.
 *
 *
 */

The Libfindf library is a collection of thread-safe functions to allow
a user to easely search one or more file(s) in mounted filesystem(s).
Made on Linux Debian 8.5, x86-64, tested only in Linux Debian 8.5 and Ubuntu 14.

The functions readily available to use are:

findf()                 -    "Quick" filesystem search using the library's default parameters.

findf_fg()              -    Finer-grained search using user defined parameters.
		       
findf_init_param()      -    Initialize a findf_param_f object pointer.

findf_destroy_param()   -    Free a findf_param_f object pointer.

findf_read_results()    -    Read a findf_results_f object pointer returned by a call to findf().

findf_destroy_results() -    Free a findf_results_f object pointer returned by a call to findf().

SU_strcpy()		-    Safely copy a string to a buffer.

(Optionaly)
findf_adv()             -    Finer-grained search using user defined parameters, as well as user defined
	       	             searching and sorting algorithms.


More information can be found on each routines (and more) within their respective man-pages,
made available in the "man_pages_src/" directory.

findf_adv() is not compiled by default. See man_pages_src/findf_adv.3.gz for explainations.
To use it, add  "-DCW_FINDF_ADVANCED" to the 'CFLAGS' string of the install_lib.sh script.

Before lauching the install_lib.sh script, please verify that all directory names matches
your system's requirements.
