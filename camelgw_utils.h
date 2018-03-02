#ifndef UTILS_H
#define UTILS_H


//#include <stdlib.h>

//#define _GNU_SOURCE //vasprintf
#include <stdio.h>  //vasprintf
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdint.h>  //uint64_t

#include <stdlib.h> //malloc
#include <string.h>


#include "camelgw2.h"


#include "inline_api.h"

#define MAXSLEEP 8

//for htonll function
#define TYP_INIT 0 
#define TYP_SMLE 1 
#define TYP_BIGE 2 

#define ARRAY_LEN(a) (size_t) (sizeof(a) / sizeof(0[a]))





long int camelgw_random(void);

/*!
 * \brief Returns a random number between 0.0 and 1.0, inclusive.
 * \since 12
 */
//#define ast_random_double() (((double)ast_random()) / RAND_MAX)






//#define attribute_malloc __attribute__((malloc))

/*!
\note \verbatim
   Note:
   It is very important to use only unsigned variables to hold
   bit flags, as otherwise you can fall prey to the compiler's
   sign-extension antics if you try to use the top two bits in
   your variable.

   The flag macros below use a set of compiler tricks to verify
   that the caller is using an "unsigned int" variable to hold
   the flags, and nothing else. If the caller uses any other
   type of variable, a warning message similar to this:

   warning: comparison of distinct pointer types lacks cast
   will be generated.

   The "dummy" variable below is used to make these comparisons.

   Also note that at -O2 or above, this type-safety checking
   does _not_ produce any additional object code at all.
 \endverbatim
*/

extern unsigned int __unsigned_int_flags_dummy;

#define ast_test_flag(p,flag) ({ \
	typeof ((p)->flags) __p = (p)->flags; \
	typeof (__unsigned_int_flags_dummy) __x = 0; \
	(void) (&__p == &__x); \
	((p)->flags & (flag)); \
    })

#define ast_set_flag(p,flag) do { \
    typeof ((p)->flags) __p = (p)->flags; \
    typeof (__unsigned_int_flags_dummy) __x = 0; \
    (void) (&__p == &__x); \
    ((p)->flags |= (flag)); \
    } while(0)

#define ast_clear_flag(p,flag) do { \
    typeof ((p)->flags) __p = (p)->flags; \
    typeof (__unsigned_int_flags_dummy) __x = 0; \
    (void) (&__p == &__x); \
    ((p)->flags &= ~(flag)); \
    } while(0)

#define ast_copy_flags(dest,src,flagz)do { \
    typeof ((dest)->flags) __d = (dest)->flags; \
    typeof ((src)->flags) __s = (src)->flags; \
    typeof (__unsigned_int_flags_dummy) __x = 0; \
    (void) (&__d == &__x); \
    (void) (&__s == &__x); \
    (dest)->flags &= ~(flagz); \
    (dest)->flags |= ((src)->flags & (flagz)); \
    } while (0)

#define ast_set2_flag(p,value,flag)do { \
    typeof ((p)->flags) __p = (p)->flags; \
    typeof (__unsigned_int_flags_dummy) __x = 0; \
    (void) (&__p == &__x); \
    if (value) \
	(p)->flags |= (flag); \
    else \
	(p)->flags &= ~(flag); \
    } while (0)

#define ast_set_flags_to(p,flag,value)do { \
    typeof ((p)->flags) __p = (p)->flags; \
    typeof (__unsigned_int_flags_dummy) __x = 0; \
    (void) (&__p == &__x); \
    (p)->flags &= ~(flag); \
    (p)->flags |= (value); \
    } while (0)


/* The following 64-bit flag code can most likely be erased after app_dial
   is reorganized to either reduce the large number of options, or handle
   them in some other way. At the time of this writing, app_dial would be
   the only user of 64-bit option flags */

extern uint64_t __unsigned_int_flags_dummy64;

#define ast_test_flag64(p,flag) ({ \
	typeof ((p)->flags) __p = (p)->flags; \
	typeof (__unsigned_int_flags_dummy64) __x = 0; \
	(void) (&__p == &__x); \
	((p)->flags & (flag)); \
    })

#define ast_set_flag64(p,flag) do { \
    typeof ((p)->flags) __p = (p)->flags; \
    typeof (__unsigned_int_flags_dummy64) __x = 0; \
    (void) (&__p == &__x); \
    ((p)->flags |= (flag)); \
    } while(0)

#define ast_clear_flag64(p,flag) do { \
    typeof ((p)->flags) __p = (p)->flags; \
    typeof (__unsigned_int_flags_dummy64) __x = 0; \
    (void) (&__p == &__x); \
    ((p)->flags &= ~(flag)); \
    } while(0)

#define ast_copy_flags64(dest,src,flagz)do { \
    typeof ((dest)->flags) __d = (dest)->flags; \
    typeof ((src)->flags) __s = (src)->flags; \
    typeof (__unsigned_int_flags_dummy64) __x = 0; \
    (void) (&__d == &__x); \
    (void) (&__s == &__x); \
    (dest)->flags &= ~(flagz); \
    (dest)->flags |= ((src)->flags & (flagz)); \
    } while (0)

#define ast_set2_flag64(p,value,flag)do { \
    typeof ((p)->flags) __p = (p)->flags; \
    typeof (__unsigned_int_flags_dummy64) __x = 0; \
    (void) (&__p == &__x); \
    if (value) \
	(p)->flags |= (flag); \
    else \
	(p)->flags &= ~(flag); \
    } while (0)

#define ast_set_flags_to64(p,flag,value)do { \
    typeof ((p)->flags) __p = (p)->flags; \
    typeof (__unsigned_int_flags_dummy64) __x = 0; \
    (void) (&__p == &__x); \
    (p)->flags &= ~(flag); \
    (p)->flags |= (value); \
    } while (0)


/* Non-type checking variations for non-unsigned int flags.  You
   should only use non-unsigned int flags where required by
   protocol etc and if you know what you're doing :)  */
#define ast_test_flag_nonstd(p,flag) \
    ((p)->flags & (flag))

#define ast_set_flag_nonstd(p,flag) do { \
	((p)->flags |= (flag)); \
    } while(0)

#define ast_clear_flag_nonstd(p,flag) do { \
	((p)->flags &= ~(flag)); \
    } while(0)

#define ast_copy_flags_nonstd(dest,src,flagz)do { \
	(dest)->flags &= ~(flagz); \
	(dest)->flags |= ((src)->flags & (flagz)); \
    } while (0)

#define ast_set2_flag_nonstd(p,value,flag)do { \
	if (value) \
	    (p)->flags |= (flag); \
	else \
	    (p)->flags &= ~(flag); \
    } while (0)

#define AST_FLAGS_ALL UINT_MAX

/*! \brief Structure used to handle boolean flags */
struct ast_flags {
    unsigned int flags;
};

/*! \brief Structure used to handle a large number of boolean flags == used only in app_dial? */
struct ast_flags64 {
    uint64_t flags;
};

//struct ast_hostent {
//  struct hostent hp;
//  char buf[1024];
//};



/*!
 * \brief free() wrapper
 *
 * ast_free_ptr should be used when a function pointer for free() needs to be passed
 * as the argument to a function. Otherwise, astmm will cause seg faults.
 */
#define camelgw_free free
#define camelgw_free_ptr camelgw_free


unsigned long long arrtonum(unsigned char *data, int length);
ssize_t sendn_test(int fd, const void *vptr, size_t n);
int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen);
unsigned long long htonll(unsigned long long src);
int reversearray(unsigned char *data, int plen);

#define CAMELGW_STACKSIZE     (((sizeof(void *) * 8 * 8) - 16) * 1024)
#define CAMELGW_BACKGROUND_STACKSIZE     (((sizeof(void *) * 8 * 8) - 16) * 1024)

//#define camelgw_pthread_create_background(a, b, c, d) camelgw_pthread_create_stack(a, b, c, d, CAMELGW_BACKGROUND_STACKSIZE, __FILE__, __FUNCTION__, __LINE__, #c)


/*!
 * \brief Try to write string, but wait no more than ms milliseconds
 * before timing out.
 *
 * \note If you are calling ast_carefulwrite, it is assumed that you are calling
 * it on a file descriptor that _DOES_ have NONBLOCK set.  This way,
 * there is only one system call made to do a write, unless we actually
 * have a need to wait.  This way, we get better performance.
 */
int camelgw_carefulwrite(int fd, char *s, int len, int timeoutms);


void ast_register_thread(char *name);
void ast_unregister_thread(void *id);

int camelgw_pthread_create_stack(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *),
			     void *data, size_t stacksize, const char *file, const char *caller,
			     int line, const char *start_fn);

int camelgw_pthread_create_detached_stack(pthread_t *thread, pthread_attr_t *attr, void*(*start_routine)(void *),
				      void *data, size_t stacksize, const char *file, const char *caller,
				      int line, const char *start_fn);

#define camelgw_pthread_create(a, b, c, d) \
    camelgw_pthread_create_stack(a, b, c, d,\
			     0, __FILE__, __FUNCTION__, __LINE__, #c)

#define camelgw_pthread_create_detached(a, b, c, d)\
    camelgw_pthread_create_detached_stack(a, b, c, d,\
				      0, __FILE__, __FUNCTION__, __LINE__, #c)

#define camelgw_pthread_create_background(a, b, c, d)\
    camelgw_pthread_create_stack(a, b, c, d,\
			     CAMELGW_BACKGROUND_STACKSIZE,\
			     __FILE__, __FUNCTION__, __LINE__, #c)

#define camelgw_pthread_create_detached_background(a, b, c, d)\
    camelgw_pthread_create_detached_stack(a, b, c, d,\
				      CAMELGW_BACKGROUND_STACKSIZE,\
				      __FILE__, __FUNCTION__, __LINE__, #c)

/* End of thread management support */












//#if defined(AST_IN_CORE)
//#define MALLOC_FAILURE_MSG						\
//  ast_log_safe(LOG_ERROR, "Memory Allocation Failure in function %s at line %d of %s\n", func, lineno, file)
//#else
//#define MALLOC_FAILURE_MSG						\
//  ast_log(LOG_ERROR, "Memory Allocation Failure in function %s at line %d of %s\n", func, lineno, file)
//#endif

AST_INLINE_API(
	       void * attribute_malloc __camelgw_malloc(size_t len, const char *file, int lineno, const char *func),
	       {
		   void *p;

		   // DEBUG_CHAOS_RETURN(DEBUG_CHAOS_ALLOC_CHANCE, NULL);

		   if (!(p = malloc(len))) {
		       //  MALLOC_FAILURE_MSG;
		   }

		   return p;
	       }
	       )

AST_INLINE_API(
	       void * attribute_malloc __camelgw_calloc(size_t num, size_t len, const char *file, int lineno, const char *func),
	       {
		   void *p;

		   //DEBUG_CHAOS_RETURN(DEBUG_CHAOS_ALLOC_CHANCE, NULL);

		   if (!(p = calloc(num, len))) {
		       //  MALLOC_FAILURE_MSG;
		   }

		   return p;
	       }
	       )

AST_INLINE_API(
	       void * attribute_malloc __camelgw_realloc(void *p, size_t len, const char *file, int lineno, const char *func),
	       {
		   void *newp;

		   // DEBUG_CHAOS_RETURN(DEBUG_CHAOS_ALLOC_CHANCE, NULL);

		   if (!(newp = realloc(p, len))) {
		       //  MALLOC_FAILURE_MSG;
		   }

		   return newp;
	       }
	       )

AST_INLINE_API(
	       char * attribute_malloc __camelgw_strdup(const char *str, const char *file, int lineno, const char *func),
	       {
		   char *newstr = NULL;

		   // DEBUG_CHAOS_RETURN(DEBUG_CHAOS_ALLOC_CHANCE, NULL);

		   if (str) {
		       if (!(newstr = strdup(str))) {
			   //	   MALLOC_FAILURE_MSG;
		       }
		   }

		   return newstr;
	       }
	       )

AST_INLINE_API(
	       char * attribute_malloc __camelgw_strndup(const char *str, size_t len, const char *file, int lineno, const char *func),
	       {
		   char *newstr = NULL;

		   // DEBUG_CHAOS_RETURN(DEBUG_CHAOS_ALLOC_CHANCE, NULL);

		   if (str) {
		       if (!(newstr = strndup(str, len))) {
			   //	   MALLOC_FAILURE_MSG;
		       }
		   }

		   return newstr;
	       }
	       )

int __attribute__((format(printf, 5, 6)))
__camelgw_asprintf(const char *file, int lineno, const char *func, char **ret, const char *fmt, ...);

AST_INLINE_API(
	       __attribute__((format(printf, 2, 0)))
	       int __camelgw_vasprintf(char **ret, const char *fmt, va_list ap, const char *file, int lineno, const char *func),
	       {
		   int res;

		   //   DEBUG_CHAOS_RETURN(DEBUG_CHAOS_ALLOC_CHANCE, -1);

		   if ((res = vasprintf(ret, fmt, ap)) == -1) {
		       // MALLOC_FAILURE_MSG;
		   }

		   return res;
	       }
	       )






/*!
 * \brief A wrapper for malloc()
 *
 * ast_malloc() is a wrapper for malloc() that will generate an Asterisk log
 * message in the case that the allocation fails.
 *
 * The argument and return value are the same as malloc()
 */
#define camelgw_malloc(len) __camelgw_malloc((len), __FILE__, __LINE__, __PRETTY_FUNCTION__)

/*!
 * \brief A wrapper for calloc()
 *
 * ast_calloc() is a wrapper for calloc() that will generate an Asterisk log
 * message in the case that the allocation fails.
 *
 * The arguments and return value are the same as calloc()
 */
#define camelgw_calloc(num, len) __camelgw_calloc((num), (len), __FILE__, __LINE__, __PRETTY_FUNCTION__)

/*!
 * \brief A wrapper for calloc() for use in cache pools
 *
 * ast_calloc_cache() is a wrapper for calloc() that will generate an Asterisk log
 * message in the case that the allocation fails. When memory debugging is in use,
 * the memory allocated by this function will be marked as 'cache' so it can be
 * distinguished from normal memory allocations.
 *
 * The arguments and return value are the same as calloc()
 */
#define camelgw_calloc_cache(num, len) __camelgw_calloc((num), (len), __FILE__, __LINE__, __PRETTY_FUNCTION__)

/*!
 * \brief A wrapper for realloc()
 *
 * ast_realloc() is a wrapper for realloc() that will generate an Asterisk log
 * message in the case that the allocation fails.
 *
 * The arguments and return value are the same as realloc()
 */
#define camelgw_realloc(p, len) __camelgw_realloc((p), (len), __FILE__, __LINE__, __PRETTY_FUNCTION__)

/*!
 * \brief A wrapper for strdup()
 *
 * ast_strdup() is a wrapper for strdup() that will generate an Asterisk log
 * message in the case that the allocation fails.
 *
 * ast_strdup(), unlike strdup(), can safely accept a NULL argument. If a NULL
 * argument is provided, ast_strdup will return NULL without generating any
 * kind of error log message.
 *
 * The argument and return value are the same as strdup()
 */
#define camelgw_strdup(str) __camelgw_strdup((str), __FILE__, __LINE__, __PRETTY_FUNCTION__)

/*!
 * \brief A wrapper for strndup()
 *
 * ast_strndup() is a wrapper for strndup() that will generate an Asterisk log
 * message in the case that the allocation fails.
 *
 * ast_strndup(), unlike strndup(), can safely accept a NULL argument for the
 * string to duplicate. If a NULL argument is provided, ast_strdup will return
 * NULL without generating any kind of error log message.
 *
 * The arguments and return value are the same as strndup()
 */
#define ast_strndup(str, len) __ast_strndup((str), (len), __FILE__, __LINE__, __PRETTY_FUNCTION__)

/*!
 * \brief A wrapper for asprintf()
 *
 * ast_asprintf() is a wrapper for asprintf() that will generate an Asterisk log
 * message in the case that the allocation fails.
 *
 * The arguments and return value are the same as asprintf()
 */
#define camelgw_asprintf(ret, fmt, ...) __camelgw_asprintf(__FILE__, __LINE__, __PRETTY_FUNCTION__, (ret), (fmt), __VA_ARGS__)

/*!
 * \brief A wrapper for vasprintf()
 *
 * ast_vasprintf() is a wrapper for vasprintf() that will generate an Asterisk log
 * message in the case that the allocation fails.
 *
 * The arguments and return value are the same as vasprintf()
 */
#define camelgw_vasprintf(ret, fmt, ap) __camelgw_vasprintf((ret), (fmt), (ap), __FILE__, __LINE__, __PRETTY_FUNCTION__)

/*!
  \brief call __builtin_alloca to ensure we get gcc builtin semantics
  \param size The size of the buffer we want allocated

  This macro will attempt to allocate memory from the stack.  If it fails
  you won't get a NULL returned, but a SEGFAULT if you're lucky.
*/
#define camelgw_alloca(size) __builtin_alloca(size)



/*!
 * \brief duplicate a string in memory from the stack
 * \param s The string to duplicate
 *
 * This macro will duplicate the given string.  It returns a pointer to the stack
 * allocatted memory for the new string.
 */
#define camelgw_strdupa(s)                                                    \
    (__extension__                                                    \
     ({                                                                \
	 const char *__old = (s);                                  \
	 size_t __len = strlen(__old) + 1;                         \
	 char *__new = __builtin_alloca(__len);                    \
	 memcpy (__new, __old, __len);                             \
	 __new;                                                    \
     }))


/* AST_INLINE_API( */
/* 	       char * attribute_malloc __camelgw_strdup(const char *str, const char *file, int lineno, const char *func), */
/* 	       { */
/* 		   char *newstr = NULL; */

/* 		   //		   DEBUG_CHAOS_RETURN(DEBUG_CHAOS_ALLOC_CHANCE, NULL); */

/* 		   if (str) { */
/* 		       if (!(newstr = strdup(str))) { */
/* 			   //	   MALLOC_FAILURE_MSG; */
/* 		       } */
/* 		   } */

/* 		   return newstr; */
/* 	       } */
/* 	       ) */


/*!
 * \brief Get current thread ID
 * \return the ID if platform is supported, else -1
 */
int camelgw_get_tid(void);


/*!
 * \brief Return the number of bytes used in the alignment of type.
 * \param type
 * \return The number of bytes required for alignment.
 *
 * This is really just __alignof__(), but tucked away in this header so we
 * don't have to look at the nasty underscores in the source.
 */
#define ast_alignof(type) __alignof__(type)

/*!
 * \brief Increase offset so it is a multiple of the required alignment of type.
 * \param offset The value that should be increased.
 * \param type The data type that offset should be aligned to.
 * \return The smallest multiple of alignof(type) larger than or equal to offset.
 * \see ast_make_room_for()
 *
 * Many systems prefer integers to be stored on aligned on memory locations.
 * This macro will increase an offset so a value of the supplied type can be
 * safely be stored on such a memory location.
 *
 * Examples:
 * ast_align_for(0x17, int64_t) ==> 0x18
 * ast_align_for(0x18, int64_t) ==> 0x18
 * ast_align_for(0x19, int64_t) ==> 0x20
 *
 * Don't mind the ugliness, the compiler will optimize it.
 */
#define ast_align_for(offset, type) (((offset + __alignof__(type) - 1) / __alignof__(type)) * __alignof__(type))

/*!
 * \brief Increase offset by the required alignment of type and make sure it is
 *        a multiple of said alignment.
 * \param offset The value that should be increased.
 * \param type The data type that room should be reserved for.
 * \return The smallest multiple of alignof(type) larger than or equal to offset
 *         plus alignof(type).
 * \see ast_align_for()
 *
 * A use case for this is when prepending length fields of type int to a buffer.
 * If you keep the offset a multiple of the alignment of the integer type,
 * a next block of length+buffer will have the length field automatically
 * aligned.
 *
 * Examples:
 * ast_make_room_for(0x17, int64_t) ==> 0x20
 * ast_make_room_for(0x18, int64_t) ==> 0x20
 * ast_make_room_for(0x19, int64_t) ==> 0x28
 *
 * Don't mind the ugliness, the compiler will optimize it.
 */
#define ast_make_room_for(offset, type) (((offset + (2 * __alignof__(type) - 1)) / __alignof__(type)) * __alignof__(type))


#ifdef DO_CRASH
#define DO_CRASH_NORETURN attribute_noreturn
#else
#define DO_CRASH_NORETURN
#endif

void DO_CRASH_NORETURN __ast_assert_failed(int condition, const char *condition_str,
					   const char *file, int line, const char *function);

#ifdef AST_DEVMODE
#define ast_assert(a) _ast_assert(a, # a, __FILE__, __LINE__, __PRETTY_FUNCTION__)
static void force_inline _ast_assert(int condition, const char *condition_str, const char *file, int line, const char *function)
{
    if (__builtin_expect(!condition, 1)) {
	__ast_assert_failed(condition, condition_str, file, line, function);
    }
}
#else
#define ast_assert(a)
#endif

/*!
 * \brief Force a crash if DO_CRASH is defined.
 *
 * \note If DO_CRASH is not defined then the function returns.
 *
 * \return Nothing
 */
 void DO_CRASH_NORETURN ast_do_crash(void);














#endif
