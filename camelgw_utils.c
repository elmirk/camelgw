#define _GNU_SOURCE //for vasprintf
#include <stdio.h>

#include "camelgw_utils.h"
#include "camelgw_time.h"
#include "camelgw_logger.h"

#include <pthread.h>

#include <poll.h>

struct thr_arg {
    void *(*start_routine)(void *);
    void *data;
    char *name;
};

/*
 * on OS/X, pthread_cleanup_push() and pthread_cleanup_pop()
 * are odd macros which start and end a block, so they _must_ be
 * used in pairs (the latter with a '1' argument to call the
 * handler on exit.
 * On BSD we don't need this, but we keep it for compatibility.
 */
static void *dummy_start(void *data)
{
    void *ret;
    struct thr_arg a = *((struct thr_arg *) data);/* make a local copy */
/* #ifdef DEBUG_THREADS */
/*     struct thr_lock_info *lock_info; */
/*     pthread_mutexattr_t mutex_attr; */

/*     if (!(lock_info = ast_threadstorage_get(&thread_lock_info, sizeof(*lock_info)))) */
/* 	return NULL; */

/*     lock_info->thread_id = pthread_self(); */
/*     lock_info->lwp = ast_get_tid(); */
/*     lock_info->thread_name = ast_strdup(a.name); */

/*     pthread_mutexattr_init(&mutex_attr); */
/*     pthread_mutexattr_settype(&mutex_attr, AST_MUTEX_KIND); */
/*     pthread_mutex_init(&lock_info->lock, &mutex_attr); */
/*     pthread_mutexattr_destroy(&mutex_attr); */

/*     pthread_mutex_lock(&lock_infos_lock.mutex); /\* Intentionally not the wrapper *\/ */
/*     AST_LIST_INSERT_TAIL(&lock_infos, lock_info, entry); */
//    pthread_mutex_unlock(&lock_infos_lock.mutex); /* Intentionally not the wrapper */
//#endif /* DEBUG_THREADS */

    /* note that even though data->name is a pointer to allocated memory,
          we are not freeing it here because ast_register_thread is going to
	     keep a copy of the pointer and then ast_unregister_thread will
	        free the memory
    */
    //    ast_free(data);
    free(data);
    //    ast_register_thread(a.name);
    //pthread_cleanup_push(ast_unregister_thread, (void *) pthread_self());

    ret = a.start_routine(a.data);

    //pthread_cleanup_pop(1);

    return ret;
}




int camelgw_pthread_create_stack(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *),
			     void *data, size_t stacksize, const char *file, const char *caller,
			     int line, const char *start_fn)
{
    //#if !defined(LOW_MEMORY)
    struct thr_arg *a;
    //#endif

    if (!attr) {
	//attr = ast_alloca(sizeof(*attr));
	attr = alloca(sizeof(*attr)); //allocate memory on stack
	pthread_attr_init(attr);
    }

#if defined(__linux__) || defined(__FreeBSD__)
    /* On Linux and FreeBSD , pthread_attr_init() defaults to PTHREAD_EXPLICIT_SCHED,
          which is kind of useless. Change this here to
	     PTHREAD_INHERIT_SCHED; that way the -p option to set realtime
	        priority will propagate down to new threads by default.
		   This does mean that callers cannot set a different priority using
		      PTHREAD_EXPLICIT_SCHED in the attr argument; instead they must set
		      the priority afterwards with pthread_setschedparam(). */
    if ((errno = pthread_attr_setinheritsched(attr, PTHREAD_INHERIT_SCHED)))
	//camelgw_log(LOG_WARNING, "pthread_attr_setinheritsched: %s\n", strerror(errno));
    printf("pthread_attr_setinheritsched: %s\n", strerror(errno));

#endif

    if (!stacksize)
	stacksize = CAMELGW_STACKSIZE;

    if ((errno = pthread_attr_setstacksize(attr, stacksize ? stacksize : CAMELGW_STACKSIZE)))
	//camelgw_log(LOG_WARNING, "pthread_attr_setstacksize: %s\n", strerror(errno));
	printf("pthread_attr_setstacksize: %s\n", strerror(errno));

#if !defined(LOW_MEMORY)
    if ((a = malloc(sizeof(*a)))) {
	a->start_routine = start_routine;
	a->data = data;
	start_routine = dummy_start;
	if (camelgw_asprintf(&a->name, "%-20s started at [%5d] %s %s()",
			 start_fn, line, file, caller) < 0) {
	    a->name = NULL;
	}
	data = a;
    }
#endif /* !LOW_MEMORY */

    return pthread_create(thread, attr, start_routine, data); /* We're in ast_pthread_create, so it's okay */
}

int camelgw_pthread_create_detached_stack(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *),
				      void *data, size_t stacksize, const char *file, const char *caller,
				      int line, const char *start_fn)
{
    unsigned char attr_destroy = 0;
    int res;

    if (!attr) {
	attr = camelgw_alloca(sizeof(*attr));
	pthread_attr_init(attr);
	attr_destroy = 1;
    }

    if ((errno = pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED)))
	camelgw_log(LOG_WARNING, "pthread_attr_setdetachstate: %s\n", strerror(errno));

    res = camelgw_pthread_create_stack(thread, attr, start_routine, data,
				   stacksize, file, caller, line, start_fn);

    if (attr_destroy)
	pthread_attr_destroy(attr);

    return res;
}




void camelgw_join_delim(char *s, size_t len, const char * const w[], unsigned int size, char delim)
{
    int x, ofs = 0;
    const char *src;

    /* Join words into a string */
    if (!s)
	return;
    for (x = 0; ofs < len && x < size && w[x] ; x++) {
	if (x > 0)
	    s[ofs++] = delim;
	for (src = w[x]; *src && ofs < len; src++)
	    s[ofs++] = *src;
    }
    if (ofs == len)
	ofs--;
    s[ofs] = '\0';
}

int camelgw_wait_for_input(int fd, int ms)
{
    struct pollfd pfd[1];

    memset(pfd, 0, sizeof(pfd));
    pfd[0].fd = fd;
    pfd[0].events = POLLIN | POLLPRI;
    return poll(pfd, 1, ms);
}

int camelgw_wait_for_output(int fd, int ms)
{
    struct pollfd pfd[1];

    memset(pfd, 0, sizeof(pfd));
    pfd[0].fd = fd;
    pfd[0].events = POLLOUT;
    return poll(pfd, 1, ms);
}

static int wait_for_output(int fd, int timeoutms)
{
    struct pollfd pfd = {
	.fd = fd,
	.events = POLLOUT,
    };
    int res;
    struct timeval start = camelgw_tvnow();
    int elapsed = 0;

    /* poll() until the fd is writable without blocking */
    while ((res = poll(&pfd, 1, timeoutms - elapsed)) <= 0) {
	if (res == 0) {
	    /* timed out. */
#ifndef STANDALONE
	    ast_debug(1, "Timed out trying to write\n");
#endif
	    return -1;
	} else if (res == -1) {
	    /* poll() returned an error, check to see if it was fatal */

	    if (errno == EINTR || errno == EAGAIN) {
		elapsed = camelgw_tvdiff_ms(camelgw_tvnow(), start);
		if (elapsed >= timeoutms) {
		    return -1;
		}
		/* This was an acceptable error, go back into poll() */
		continue;
	    }

	    /* Fatal error, bail. */
	    camelgw_log(LOG_ERROR, "poll returned error: %s\n", strerror(errno));

	    return -1;
	}
	elapsed = camelgw_tvdiff_ms(camelgw_tvnow(), start);
	if (elapsed >= timeoutms) {
	    return -1;
	}
    }

    return 0;
}



/*!
 * Try to write string, but wait no more than ms milliseconds before timing out.
 *
 * \note The code assumes that the file descriptor has NONBLOCK set,
 * so there is only one system call made to do a write, unless we actually
 * have a need to wait.  This way, we get better performance.
 * If the descriptor is blocking, all assumptions on the guaranteed
 * detail do not apply anymore.
 */
int camelgw_carefulwrite(int fd, char *s, int len, int timeoutms)
{
    struct timeval start = camelgw_tvnow();
    int res = 0;
    int elapsed = 0;

    while (len) {
	if (wait_for_output(fd, timeoutms - elapsed)) {
	    return -1;
	}

	res = write(fd, s, len);

	if (res < 0 && errno != EAGAIN && errno != EINTR) {
	    /* fatal error from write() */
	    if (errno == EPIPE) {
#ifndef STANDALONE
		ast_debug(1, "write() failed due to reading end being closed: %s\n", strerror(errno));
#endif
	    } else {
		camelgw_log(LOG_ERROR, "write() returned error: %s\n", strerror(errno));
	    }
	    return -1;
	}

	if (res < 0) {
	    /* It was an acceptable error */
	    res = 0;
	}

	/* Update how much data we have left to write */
	len -= res;
	s += res;
	res = 0;

	elapsed = camelgw_tvdiff_ms(camelgw_tvnow(), start);
	if (elapsed >= timeoutms) {
	    /* We've taken too long to write
	     * This is only an error condition if we haven't finished writing. */
	    res = len ? -1 : 0;
	    break;
	}
    }

    return res;
}

/* get thread id */

int camelgw_get_tid(void)
{
    int ret = -1;
#if defined (__linux) && defined(SYS_gettid)
    ret = syscall(SYS_gettid); /* available since Linux 1.4.11 */
#elif defined(__sun)
    ret = pthread_self();
#elif defined(__APPLE__)
    ret = mach_thread_self();
    mach_port_deallocate(mach_task_self(), ret);
#elif defined(__FreeBSD__) && defined(HAVE_SYS_THR_H)
    long lwpid;
    thr_self(&lwpid); /* available since sys/thr.h creation 2003 */
    ret = lwpid;
#endif
    return ret;
}

static int dev_urandom_fd = -1;


#ifndef linux
AST_MUTEX_DEFINE_STATIC(randomlock);
#endif

long int camelgw_random(void)
{
    long int res;

    if (dev_urandom_fd >= 0) {
	int read_res = read(dev_urandom_fd, &res, sizeof(res));
	if (read_res > 0) {
	    long int rm = RAND_MAX;
	    res = res < 0 ? ~res : res;
	    rm++;
	    return res % rm;
	}
    }

    /* XXX - Thread safety really depends on the libc, not the OS.
     *
     * But... popular Linux libc's (uClibc, glibc, eglibc), all have a
     * somewhat thread safe random(3) (results are random, but not
     * reproducible). The libc's for other systems (BSD, et al.), not so
     * much.
     */
#ifdef linux
    res = random();
#else
    camelgw_mutex_lock(&randomlock);
    res = random();
    camelgw_mutex_unlock(&randomlock);
#endif
    return res;
}


/* static void utils_shutdown(void) */
/* { */
/*     close(dev_urandom_fd); */
/*     dev_urandom_fd = -1; */
/* #if defined(DEBUG_THREADS) && !defined(LOW_MEMORY) */
/*     ast_cli_unregister_multiple(utils_cli, ARRAY_LEN(utils_cli)); */
/* #endif */
/* } */


/* int camelgw_utils_init(void) */
/* { */
/*     dev_urandom_fd = open("/dev/urandom", O_RDONLY); */
/*     base64_init(); */
/* #ifdef DEBUG_THREADS */
/* #if !defined(LOW_MEMORY) */
/*     ast_cli_register_multiple(utils_cli, ARRAY_LEN(utils_cli)); */
/* #endif */
/* #endif */
/*     ast_register_cleanup(utils_shutdown); */
/*     return 0; */
/* } */

#ifndef __AST_DEBUG_MALLOC
int __camelgw_asprintf(const char *file, int lineno, const char *func, char **ret, const char *fmt, ...)
{
    int res;
    va_list ap;

    va_start(ap, fmt);
    if ((res = vasprintf(ret, fmt, ap)) == -1) {
	//	MALLOC_FAILURE_MSG;
    }
    va_end(ap);

    return res;
}
#endif


/***************************************************************************************************************
arrtonum function:

convert array of int element into unsigned long long value
test in =   unsigned char test_data[]= {2,5,0,2,7,1,2,3,4,5,6,7,8,9,0};
test out = unsigned long long value = 250271234567890
lentgh - number of digitst converted to value
in case of IMSI length = 15

*****************************************************************************************************************/
unsigned long long arrtonum(unsigned char *data, int length) {

    int i;
	    const unsigned long long PowersOf10[] = { // use lookup to reduce multiplications
		1, 10,
		100, 1000,
		10000, 100000,
		1000000, 10000000,
		100000000, 1000000000, // actually the table can end here since A[i] is of type int
		10000000000, 100000000000,
		1000000000000, 10000000000000,
		100000000000000 };

	    unsigned long long *p10, n = 0; // or unsigned long long if uint64_t not available
	    int N = length -1;
	    //  for (i = 0; i < sizeof(A)/sizeof(A[0]); i++)
  for (i = 0; i < length; i++)
		{
		    //  p10 = PowersOf10;
		    // while (*p10 < A[i]) p10++;
		    //n *= *p10;
		    //n += A[i];
		    n = n + (*(data+i)*PowersOf10[N--]);
		}

	return n;
}


ssize_t                         /* send "n" bytes to a socket descriptor. */
sendn_test(int fd, const void *vptr, size_t n) { 
    size_t nleft;
    ssize_t nsent;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {

	nsent = send(fd, ptr, nleft, MSG_NOSIGNAL);

		if ( nsent <= 0) {
	    //	printf("nsent = %d\n", nsent);
	    //		perror("send");
		// if (nsent < 0 && errno == EPIPE) {
		//		printf("nsent = %d\n", nsent);
		//perror("send");
		//	return EPIPE;   /* and call send() again */

		// }

	    if (nsent < 0 && errno == EINTR) {
		//		printf("nsent = %d\n", nsent);
		//perror("send");
		nsent = 0;   /* and call send() again */

	    }
            else
		return errno;    /* error */
         }
	nleft -= nsent;
	ptr += nsent;
	}
   return (n);
}

int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen) {
    int nsec;
    /*
     * Попытаться установить соединение с экспоненциальной задержкой.
     */
    for (nsec = 1; nsec <= MAXSLEEP; nsec <<= 1) {
	printf("trying to connect .....\n");
	if (connect(sockfd, addr, alen) == 0) {
	    /*
	     * Соединение установлено.
	     */
	    return 0;
	}
	/*
	 * Задержка перед следующей попыткой.
	 */
	if (nsec <= MAXSLEEP/2)
	    sleep(nsec);
    }
    return (-1);
}

 
unsigned long long htonll(unsigned long long src) { 
    static int typ = TYP_INIT; 
    unsigned char c; 
    union { 
	unsigned long long ull; 
	unsigned char c[8]; 
    } x; 
    if (typ == TYP_INIT) { 
	x.ull = 0x01; 
	typ = (x.c[7] == 0x01ULL) ? TYP_BIGE : TYP_SMLE; 
    } 
    if (typ == TYP_BIGE) 
	return src; 
    x.ull = src; 
    c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c; 
    c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c; 
    c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c; 
    c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c; 
    return x.ull; 
} 

/***************************************************************************************************************
reversearray function:

mirror-like reverse elements in unsigned char array 
test in =    unsigned char data[] = {0xab, 0xcd, 0xef, 0x08, 0xcf, 0xea, 0, 0}; and plen = 6
test out = unsigned char data[] = {0xea, 0xcf, 0x08, 0xef, 0xcd, 0xab, 0, 0}
plen - number of element which should be reversed

*****************************************************************************************************************/
 
int reversearray(unsigned char *data, int plen)

{

 unsigned char tmp;
 int i = 0;
 //plen--;
 unsigned char nIters = plen >> 1;
 plen--;
 while ( i < nIters)
     {
	 tmp = data[i];
    data[i++] = data[plen];
 data[plen--] = tmp;
 //++i;
    //plen--;
}



 return 0;
}


