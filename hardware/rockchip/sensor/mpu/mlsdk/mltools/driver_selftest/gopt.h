/* gopt.h  */

#ifndef GOPT_H_INCLUDED
#define GOPT_H_INCLUDED

#define GOPT_ONCE   0
#define GOPT_REPEAT 1
#define GOPT_NOARG  0
#define GOPT_ARG    2

#define gopt_start(...)  (const void*)( const struct { int k; int f; const char *s; const char*const*l; }[]){ __VA_ARGS__, {0}}
#define gopt_option(k,f,s,l)    { k, f, s, l }
#define gopt_shorts( ... )      (const char*)(const char[]){ __VA_ARGS__, 0 }
#define gopt_longs( ... )       (const char**)(const char*[]){ __VA_ARGS__, NULL }


void *gopt_sort( int *argc, const char **argv, const void *opt_specs );
/* returns a pointer for use in the following calls
 * prints to stderr and call exit() on error
 */
size_t gopt( const void *opts, int key );
/* returns the number of times the option was specified
 * which will be 0 or 1 unless GOPT_REPEAT was used
 */
size_t gopt_arg( const void *opts, int key, const char **arg );
/* returns the number of times the option was specified
 * writes a pointer to the option argument from the first (or only) occurance to *arg
 */
const char *gopt_arg_i( const void *opts, int key, size_t i );
/* returns a pointer to the ith (starting at zero) occurance
 * of the option, or NULL if it was not specified that many times
 */
size_t gopt_args( const void *opts, int key, const char **args, size_t args_len );
/* returns the number of times the option was specified
 * writes pointers to the option arguments in the order of occurance to args[].
 * writes at most args_len pointers
 * if the return value is less than args_len, also writes a null pointer
 */
void gopt_free( void *opts );
/* releases memory allocated in the corresponding call to gopt_sort()
 * opts can no longer be used 
 */
#endif /* GOPT_H_INCLUDED */
