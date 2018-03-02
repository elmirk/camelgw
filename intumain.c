/*
              Copyright (C) Dialogic Corporation 1998-2006. All Rights Reserved.

   Name:          intumain.c

this is running on test camel server


   Description:   Command line interface to intu. See the main body of intu
                  in file intu.c.

                  Files in this module:

                  intu_def.h - INTU module specific definitions
                  intu.c     - Main control loop and handling procedures
                  intumain.c - Reading and checking of command line options
                  intu_trc.c - Procedures for tracing and displaying output
                  intu_sys.c - Messages sending and formatting procedures


   Functions:     main()

   -----  ---------  -----  ---------------------------------------------
   Issue    Date      By                     Changes
   -----  ---------  -----  ---------------------------------------------
     A    22-Dec-98  JET    - Initial code based on ctu.
     1    09-Sep-04  GNK    - Eliminate warnings - declare static functions.
                            - Update copyright owner & year.
     2    13-Dec-06  ML     - Change to use of Dialogic Corporation copyright.
*/
#define _GNU_SOURCE //need for structure  struct ucred, for checking conneced unix domain client credentials


#define __GNUC_STDC_INLINE__

#include <ctype.h>  //isspace
#include <histedit.h>  //editline, for CLI interface
#include <grp.h>  //getgrid
#include <pwd.h>  //get pwduid
//#include <sys/types.h>
#include <errno.h>


#include "intu_def.h"
#include "camelgw_prepaid.h"
#include "camelgw_conf.h"
#include "camelgw_init.h"
#include "camelgw_utils.h"
#include "camelgw_strings.h"
#include "camelgw_time.h"
#include "camelgw_version.h"  //get_version
#include "lock.h"
#include "cli.h"

#include "inline_api.h"
#include "camelgw_logger.h"
//#include "camelgw_strings.h"
#include "cli.h"
#include "threadstorage.h"
#include "localtime.h"

#include "term.h"


#include <poll.h>


#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#define PF_LOCAL PF_UNIX
#endif

#define CAMELGW_MAX_CONNECTS 12
#define NUM_MSGS 64


/*! \brief Welcome message when starting a CLI interface */
#define WELCOME_MESSAGE \
    ast_verbose("Asterisk %s, Copyright (C) 1999 - 2016, Digium, Inc. and others.\n" \
                "Created by Mark Spencer <markster@digium.com>\n" \
                "Asterisk comes with ABSOLUTELY NO WARRANTY; type 'core show warranty' for details.\n" \
                "This is free software, with components licensed under the GNU General Public\n" \
                "License version 2 and other licenses; you are welcome to redistribute it under\n" \
                "certain conditions. Type 'core show license' for details.\n" \
                "=========================================================================\n", camelgw_get_version()) \



//#define MAXHOSTNAMELEN 128

//#if defined(__linux__)
//#include <sys/prctl.h>
//#endif

pid_t camelgw_mainpid;
static int camelgw_socket = -1; /* !< UNIX Socket for allowing remote control */

static int ast_consock = -1;/*!< UNIX Socket for controlling another asterisk */



/*! Maximum active system verbosity level. */
int ast_verb_sys_level;


int option_verbose;               /*  !< Verbosity level */
int option_debug;                  /* !< Debug level  */

/*! \defgroup main_options Main Configuration Options
 * \brief Main configuration options from asterisk.conf or OS command line on starting Asterisk.
 * \arg \ref Config_ast "asterisk.conf"
 * \note Some of them can be changed in the CLI
 */
/*! @{ */

//struct ast_flags ast_options = { AST_DEFAULT_OPTIONS };
struct ast_flags ast_options = { 0 };


struct console {
    int fd;/*!< File descriptor */
    int p[2];/*!< Pipe */
    pthread_t t;/*!< Thread of handler */
    int mute;/*!< Is the console muted for logs */
    int uid;/*!< Remote user ID. */
    int gid;/*!< Remote group ID. */
    int levels[NUMLOGLEVELS];/*!< Which log levels are enabled for the console */
    /*! Verbosity level of this console. */
    int option_verbose;
};

struct console consoles[CAMELGW_MAX_CONNECTS];

static History *el_hist;
static EditLine *el;
static char *remotehostname;


/*
 * Module variables, updated by command line options in intu_main
 */
extern u8  intu_mod_id;      /* The task id of this module */
extern u8  inap_mod_id;      /* The task id of the INAP binary module*/
extern u16 num_dialogues;    /* The number of dialogues to support*/
extern u32 base_dialogue_id; /* The base number of the supported dialogue IDs */
extern u16 intu_options;     /* Defines which tracing options are configured */
//extern int socket_d;


//static pthread_t lthread;

/*
 * Declare static functions
 */

  static int read_command_line_params(int argc, char *argv[], int *arg_index);
  static void show_syntax(void);
  static int read_option(char *args);
      
//      #include <sys/types.h>
//      #include <syslog.h>     
//to compile: gcc  -pthread filename.c -o filename	

//	 struct alarm {
//              int severity;
//              char *descriptor;
//       };

//	static void sig_handler(int sig, siginfo_t *si, void *uc) {

            /* Note: calling printf() from a signal handler is not              
		strictly correct, since printf() is not async-signal-safe;
              see signal(7) */
	    
	    //int sid = si->si_value.sival_int;
	    //char * msg = si->si_value.sival_ptr; 
	    //int i = sid - 1;
            //printf("Caught signal %d\n", sig);
	    //printf("Action ID is %d\n", si->si_value.sival_int);
	    //printf("descriptor ptr = %p\n",( (struct alarm *) (si->si_value.sival_ptr) )->descriptor);
	    //struct alarm *ptr;
	    //printf("sival  int in handler = %d\n", si->si_value.sival_int);
//	      printf("string in handler = %s\n", si->si_value.sival_ptr);
	      //	    ptr = si->si_value.sival_ptr;
	      //printf("descriptor ptr = %p\n", si->si_value.sival_ptr);
	      //printf("severity in handler = %i\n", ptr->severity);
	    //  ptr = si->si_value.sival_ptr;
	      //printf("descriptor ptr in handler = %p\n", ptr->descriptor);
	    //printf("%s\n",((struct alarm *)(si->si_value.sival_ptr))->descriptor);
	    
	    // vova edition : openlog ("camelgw", LOG_PID | LOG_NDELAY, LOG_USER);
//	    openlog ("camelgw", LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	    //syslog (LOG_DEBUG,"%s", messages[i]);
	    // syslog (((struct alarm *)(si->si_value.sival_ptr))->severity, "%s", ( (struct alarm *) (si->si_value.sival_ptr) )-> descriptor);
//	     syslog (LOG_DEBUG, "%s", (si->si_value.sival_ptr));
//          closelog ();
            
//	}

//       static void * thread_func(void *arg){
//         sigset_t set;
//	   struct sigaction sa;
//         int s, sig;
//	   int counter;
        
	   // Unblock expected signal
//	   sigemptyset(&set);
//	   sigaddset(&set,SIGRTMIN);
//	   s = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
           
	   //Looking forward to signal and process it by handler
//	   memset(&sa,0,sizeof(sa));
//	   sa.sa_flags = SA_SIGINFO;
//         sa.sa_sigaction = sig_handler;
//         sa.sa_mask;
 
//         sigaction(SIGRTMIN, &sa, NULL);
 	
//		while(1){
//	   }
//     }

//const char *ast_config_AST_SOCKET= cfg_paths.socket_path;
const char *camelgw_config_CAMELGW_SOCKET = "/var/run/camelgw/cli_socket";
//const char *ast_config_AST_LOG_DIR= cfg_paths.log_dir;
const char *camelgw_config_CAMELGW_LOG_DIR= "/opt/camelgw/log";
const char *camelgw_config_CAMELGW_SYSTEM_NAME="system_asparagus";

static char *_argv[256];

typedef enum {
    /*! Normal operation */
    NOT_SHUTTING_DOWN,
    /*! Committed to shutting down.  Final phase */
    SHUTTING_DOWN_FINAL,
    /*! Committed to shutting down.  Initial phase */
    SHUTTING_DOWN,
    /*!
     * Valid values for quit_handler() niceness below.
     * These shutdown/restart levels can be cancelled.
     *
     * Remote console exit right now
     */
    SHUTDOWN_FAST,
    /*! core stop/restart now */
    SHUTDOWN_NORMAL,
    /*! core stop/restart gracefully */
    SHUTDOWN_NICE,
    /*! core stop/restart when convenient */
SHUTDOWN_REALLY_NICE
} shutdown_nice_t;

static shutdown_nice_t shuttingdown = NOT_SHUTTING_DOWN;

/*! Prevent new channel allocation for shutdown. */
static int shutdown_pending;

static int restartnow;
static pthread_t consolethread = CAMELGW_PTHREADT_NULL;
static pthread_t mon_sig_flags;
static int canary_pid = 0;
static char canary_filename[128];
static int multi_thread_safe;

static char randompool[256];


static void set_header(char *outbuf, int maxout, char level)
{
const char *cmp;
char date[40];

switch (level) {
 case 0: cmp = NULL;
break;
 case 1: cmp = VERBOSE_PREFIX_1;
break;
 case 2: cmp = VERBOSE_PREFIX_2;
break;
 case 3: cmp = VERBOSE_PREFIX_3;
break;
 default: cmp = VERBOSE_PREFIX_4;
break;
}

if (ast_opt_timestamp) {
struct ast_tm tm;
struct timeval now = camelgw_tvnow();
ast_localtime(&now, &tm, NULL);
ast_strftime(date, sizeof(date), ast_logger_get_dateformat(), &tm);
}

snprintf(outbuf, maxout, "%s%s%s%s%s%s",
	     ast_opt_timestamp ? "[" : "",
	     ast_opt_timestamp ? date : "",
	     ast_opt_timestamp ? "] " : "",
	     cmp ? ast_term_color(COLOR_GRAY, 0) : "",
	     cmp ? cmp : "",
	     cmp ? ast_term_reset() : "");
}

/* Sending commands from consoles back to the daemon requires a terminating NULL */
static int fdsend(int fd, const char *s)
{
    return write(fd, s, strlen(s) + 1);
}


/* Sending messages from the daemon back to the display requires _excluding_ the terminating NULL */
static int fdprint(int fd, const char *s)
{
    return write(fd, s, strlen(s));
}

/*!
 * \brief enable or disable a logging level to a specified console
 */

void ast_console_toggle_loglevel(int fd, int level, int state)
{
int x;

if (level >= NUMLOGLEVELS) {
level = NUMLOGLEVELS - 1;
}

for (x = 0;x < CAMELGW_MAX_CONNECTS; x++) {
if (fd == consoles[x].fd) {
/*
 * Since the logging occurs when levels are false, set to
 * flipped iinput because this function accepts 0 as off and 1 as on
 */
consoles[x].levels[level] = state ? 0 : 1;
return;
}
}
}

/*!
 * \brief mute or unmute a console from logging
 */
void camelgw_console_toggle_mute(int fd, int silent)
{
    int x;
    for (x = 0;x < CAMELGW_MAX_CONNECTS; x++) {
	if (fd == consoles[x].fd) {
	    if (consoles[x].mute) {
		consoles[x].mute = 0;
		if (!silent)
		    camelgw_cli(fd, "Console is not muted anymore.\n");
	    } else {
		consoles[x].mute = 1;
		if (!silent)
		    camelgw_cli(fd, "Console is muted.\n");
	    }
	    return;
	}
    }
    camelgw_cli(fd, "Couldn't find remote console.\n");
}

/*!
 * \brief log the string to all attached network console clients
 */
static void camelgw_network_puts_mutable(const char *string, int level, int sublevel)
{
int x;

for (x = 0; x < CAMELGW_MAX_CONNECTS; ++x) {
if (consoles[x].fd < 0
|| consoles[x].mute
|| consoles[x].levels[level]
|| (level == __LOG_VERBOSE && consoles[x].option_verbose < sublevel)) {
continue;
}
fdprint(consoles[x].p[1], string);
}
}

/*!
 * \brief log the string to the root console, and all attached
 * network console clients
 */
void camelgw_console_puts_mutable(const char *string, int level)
{
    camelgw_console_puts_mutable_full(string, level, 0);
}

static int console_print(const char *s);

void camelgw_console_puts_mutable_full(const char *message, int level, int sublevel)
{
    /* Send to the root console */
    console_print(message);
    
    /* Wake up a poll()ing console */
    if (ast_opt_console && consolethread != CAMELGW_PTHREADT_NULL) {
	pthread_kill(consolethread, SIGURG);
    }
    
    /* Send to any network console clients */
    camelgw_network_puts_mutable(message, level, sublevel);
}

/*!
 * \brief write the string to all attached console clients
 */
static void camelgw_network_puts(const char *string)
{
int x;

for (x = 0; x < CAMELGW_MAX_CONNECTS; ++x) {
if (consoles[x].fd < 0) {
continue;
}
fdprint(consoles[x].p[1], string);
}
}

/*!
 * \brief write the string to the root console, and all attached
 * network console clients
 */
void camelgw_console_puts(const char *string)
{
/* Send to the root console */
fputs(string, stdout);
fflush(stdout);

/* Send to any network console clients */
camelgw_network_puts(string);
}

static pthread_t lthread;





/*!
* \brief read() function supporting the reception of user credentials.
 *
 * \param fd Socket file descriptor.
 * \param buffer Receive buffer.
 * \param size 'buffer' size.
 * \param con Console structure to set received credentials
 * \retval -1 on error
 * \retval the number of bytes received on success.
 */
static int read_credentials(int fd, char *buffer, size_t size, struct console *con)
{
#if defined(SO_PEERCRED)
#ifdef HAVE_STRUCT_SOCKPEERCRED_UID
#define HAVE_STRUCT_UCRED_UID
    struct sockpeercred cred;
#else
    struct ucred cred;
#endif
    socklen_t len = sizeof(cred);
#endif
#if defined(HAVE_GETPEEREID)
    uid_t uid;
    gid_t gid;
#else
    int uid, gid;
#endif
    int result;

    result = read(fd, buffer, size);
    if (result < 0) {
	return result;
    }

#if defined(SO_PEERCRED) && (defined(HAVE_STRUCT_UCRED_UID) || defined(HAVE_STRUCT_UCRED_CR_UID))
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len)) {
	return result;
    }
#if defined(HAVE_STRUCT_UCRED_UID)
    uid = cred.uid;
    gid = cred.gid;
#else /* defined(HAVE_STRUCT_UCRED_CR_UID) */
    uid = cred.cr_uid;
    gid = cred.cr_gid;
#endif /* defined(HAVE_STRUCT_UCRED_UID) */

#elif defined(HAVE_GETPEEREID)
    if (getpeereid(fd, &uid, &gid)) {
	return result;
    }
#else
    return result;
#endif
    con->uid = uid;
    con->gid = gid;

    return result;
}


/* This is the thread running the remote console on the main process. */
static void *netconsole(void *vconsole)
{
    struct console *con = vconsole;
    char hostname[MAXHOSTNAMELEN] = "";
    char inbuf[512];
    char outbuf[512];
    const char * const end_buf = inbuf + sizeof(inbuf);
    char *start_read = inbuf;
    int res;
    struct pollfd fds[2];

    if (gethostname(hostname, sizeof(hostname)-1))
	camelgw_copy_string(hostname, "<Unknown>", sizeof(hostname));

    //find ast_get version code!
        snprintf(outbuf, sizeof(outbuf), "%s/%ld/%s\n", hostname, (long)camelgw_mainpid, camelgw_get_version());
    
snprintf(outbuf, sizeof(outbuf), "%s/%ld\n", hostname, (long)camelgw_mainpid);

fdprint(con->fd, outbuf);
//todo!   ast_verb_console_register(&con->option_verbose);
    for (;;) {
	fds[0].fd = con->fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = con->p[0];
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	//todo - ast poll vs poll!
	//res = ast_poll(fds, 2, -1);
	res = poll(fds, 2, -1);
	if (res < 0) {
	    if (errno != EINTR)
		//	ast_log(LOG_WARNING, "poll returned < 0: %s\n", strerror(errno));
		printf("poll returned < 0: %s\n", strerror(errno));
	    continue;
	}
	if (fds[0].revents) {
	    int cmds_read, bytes_read;
	    if ((bytes_read = read_credentials(con->fd, start_read, end_buf - start_read, con)) < 1) {
		break;
	    }
	    /* XXX This will only work if it is the first command, and I'm not sure fixing it is worth the effort. */
	    if (strncmp(inbuf, "cli quit after ", 15) == 0) {
		camelgw_cli_command_multiple_full(con->uid, con->gid, con->fd, bytes_read - 15, inbuf + 15);
		break;
	    }
	    /* ast_cli_command_multiple_full will only process individual commands terminated by a
	     * NULL and not trailing partial commands. */
	    if (!(cmds_read = camelgw_cli_command_multiple_full(con->uid, con->gid, con->fd, bytes_read + start_read - inbuf, inbuf))) {
		/* No commands were read. We either have a short read on the first command
		 * with space left, or a command that is too long */
		if (start_read + bytes_read < end_buf) {
		    start_read += bytes_read;
		} else {
		    //ast_log(LOG_ERROR, "Command too long! Skipping\n");
		    printf("Command too long! Skipping\n");
		    start_read = inbuf;
		}
		continue;
	    }
	    if (start_read[bytes_read - 1] == '\0') {
		/* The read ended on a command boundary, start reading again at the head of inbuf */
		start_read = inbuf;
		continue;
	    }
	    /* If we get this far, we have left over characters that have not been processed.
	     * Advance to the character after the last command read by ast_cli_command_multiple_full.
	     * We are guaranteed to have at least cmds_read NULLs */
	    while (cmds_read-- && (start_read = strchr(start_read, '\0'))) {
		start_read++;
	    }
	    memmove(inbuf, start_read, end_buf - start_read);
	    start_read = end_buf - start_read + inbuf;
	}
	if (fds[1].revents) {
	    res = read_credentials(con->p[0], outbuf, sizeof(outbuf), con);
	    if (res < 1) {
		//		ast_log(LOG_ERROR, "read returned %d\n", res);
		printf("bla bla\n");		
break;
	    }
	    res = write(con->fd, outbuf, res);
	    if (res < 1)
		break;
	}
    }
    ast_verb_console_unregister();

    /* if (!ast_opt_hide_connect) { */
    /* 	ast_verb(3, "Remote UNIX connection disconnected\n"); */
    /* } */

    close(con->fd);
    close(con->p[0]);
    close(con->p[1]);
    con->fd = -1;

    return NULL;
}





/*thread function to listen CLI connects */

static void *listener(void *unused)
{
    struct sockaddr_un sunaddr;
    int s;
    socklen_t len;
    int x;
    int poll_result;
    struct pollfd fds[1];

    for (;;)
	{

	if (camelgw_socket < 0) {
	    return NULL;
	}

	fds[0].fd = camelgw_socket;
	fds[0].events = POLLIN;
	//poll_result = ast_poll(fds, 1, -1);
	poll_result = poll(fds, 1, -1);
	pthread_testcancel();
	if (poll_result < 0) {
	    if (errno != EINTR) {
		//ast_log(LOG_WARNING, "poll returned error: %s\n", strerror(errno));
	    }
	    continue;
	}
	len = sizeof(sunaddr);
	s = accept(camelgw_socket, (struct sockaddr *)&sunaddr, &len);
	
	if (s < 0) {
	    if (errno != EINTR)
		//ast_log(LOG_WARNING, "Accept returned %d: %s\n", s, strerror(errno));
		printf("Accept returned %d: %s\n", s, strerror(errno));	
}
	else
	    {
#if defined(SO_PASSCRED)
	    int sckopt = 1;
	    /* turn on socket credentials passing. */
	    if (setsockopt(s, SOL_SOCKET, SO_PASSCRED, &sckopt, sizeof(sckopt)) < 0) {
		//ast_log(LOG_WARNING, "Unable to turn on socket credentials passing\n");
		close(s);
	    } else
#endif
		{
		    for (x = 0; x < CAMELGW_MAX_CONNECTS; x++) {
			if (consoles[x].fd >= 0) {
			    continue;
			}
			if (socketpair(AF_LOCAL, SOCK_STREAM, 0, consoles[x].p)) {
			    //ast_log(LOG_ERROR, "Unable to create pipe: %s\n", strerror(errno));
			    //fdprint(s, "Server failed to create pipe\n");
			    close(s);
			    break;
			}
			//ast_fd_set_flags(consoles[x].p[1], O_NONBLOCK);
			consoles[x].mute = 1; /* Default is muted, we will un-mute if necessary */
			/* Default uid and gid to -2, so then in cli.c/cli_has_permissions() we will be able
			   to know if the user didn't send the credentials. */
			consoles[x].uid = -2;
			consoles[x].gid = -2;
			/* Server default of remote console verbosity level is OFF. */
			consoles[x].option_verbose = 0;
			consoles[x].fd = s;
			if (camelgw_pthread_create_detached_background(&consoles[x].t, NULL, netconsole, &consoles[x])) {
			    consoles[x].fd = -1;
			    camelgw_log(LOG_ERROR, "Unable to spawn thread to handle connection: %s\n", strerror(errno));
			    close(consoles[x].p[0]);
			    close(consoles[x].p[1]);
			    fdprint(s, "Server failed to spawn thread\n");
			    close(s);
			}
			break;
		    }
		    if (x >= CAMELGW_MAX_CONNECTS) {
			//fdprint(s, "No more connections allowed\n");
			//ast_log(LOG_WARNING, "No more connections allowed\n");
			close(s);
		    } 
		    //todo analyze		    else if ((consoles[x].fd > -1) && (!ast_opt_hide_connect)) {
		    //	ast_verb(3, "Remote UNIX connection\n");
		    //}
		}
	}
    }
    return NULL;
}


static int camelgw_makesocket(void)
{
    struct sockaddr_un sunaddr;
    int res;
    int x;
    uid_t uid = -1;
    gid_t gid = -1;

    for (x = 0; x < CAMELGW_MAX_CONNECTS; x++)
	{
	    consoles[x].fd = -1;
	}

    //   if (ast_socket_is_sd) {
    //	ast_socket = ast_sd_get_fd_un(SOCK_STREAM, ast_config_AST_SOCKET);
    //
    //	goto start_lthread;
    // }

    unlink(camelgw_config_CAMELGW_SOCKET);
    camelgw_socket = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (camelgw_socket < 0) {
	//	ast_log(LOG_WARNING, "Unable to create control socket: %s\n", strerror(errno));
	printf("unable to chreate socket!\n");
	return -1;
    }
    memset(&sunaddr, 0, sizeof(sunaddr));
    sunaddr.sun_family = AF_LOCAL;
    //    ast_copy_string(sunaddr.sun_path, ast_config_AST_SOCKET, sizeof(sunaddr.sun_path));
    strncpy(sunaddr.sun_path, camelgw_config_CAMELGW_SOCKET, sizeof(sunaddr.sun_path)-1);

    res = bind(camelgw_socket, (struct sockaddr *)&sunaddr, sizeof(sunaddr));
    if (res) {
	//	ast_log(LOG_WARNING, "Unable to bind socket to %s: %s\n", ast_config_AST_SOCKET, strerror(errno));

	printf("unable to bind socket!: %s\n", strerror(errno));
	close(camelgw_socket);
	camelgw_socket = -1;
	return -1;
    }
    res = listen(camelgw_socket, 2);
    if (res < 0) {
	//	ast_log(LOG_WARNING, "Unable to listen on socket %s: %s\n", ast_config_AST_SOCKET, strerror(errno));
	printf("unable to listen socket!\n");
	close(camelgw_socket);

	camelgw_socket = -1;
	return -1;
    }

    //start_lthread:
    if (camelgw_pthread_create_background(&lthread, NULL, listener, NULL)) {
	//ast_log(LOG_WARNING, "Unable to create listener thread.\n");
	close(camelgw_socket);
	return -1;
    }


    /* if (ast_socket_is_sd) { */
    /* 	/\* owner/group/permissions are set by systemd, we might not even have access */
    /* 	 * to socket file so leave it alone *\/ */
    /* 	return 0; */
    /* } */

    /* if (!ast_strlen_zero(ast_config_AST_CTL_OWNER)) { */
    /* 	struct passwd *pw; */
    /* 	if ((pw = getpwnam(ast_config_AST_CTL_OWNER)) == NULL) */
    /* 	    ast_log(LOG_WARNING, "Unable to find uid of user %s\n", ast_config_AST_CTL_OWNER); */
    /* 	else */
    /* 	    uid = pw->pw_uid; */
    /* } */

    /* if (!ast_strlen_zero(ast_config_AST_CTL_GROUP)) { */
    /* 	struct group *grp; */
    /* 	if ((grp = getgrnam(ast_config_AST_CTL_GROUP)) == NULL) */
    /* 	    ast_log(LOG_WARNING, "Unable to find gid of group %s\n", ast_config_AST_CTL_GROUP); */
    /* 	else */
    /* 	    gid = grp->gr_gid; */
    /* } */

    //    if (chown(camelgw_config_CAMELGW_SOCKET, uid, gid) < 0)
    //	ast_log(LOG_WARNING, "Unable to change ownership of %s: %s\n", ast_config_AST_SOCKET, strerror(errno));

    /* if (!ast_strlen_zero(ast_config_AST_CTL_PERMISSIONS)) { */
    /* 	unsigned int p1; */
    /* 	mode_t p; */
    /* 	sscanf(ast_config_AST_CTL_PERMISSIONS, "%30o", &p1); */
    /* 	p = p1; */
    /* 	if ((chmod(ast_config_AST_SOCKET, p)) < 0) */
    /* 	    ast_log(LOG_WARNING, "Unable to change file permissions of %s: %s\n", ast_config_AST_SOCKET, strerror(errno)); */
    /* } */

    return 0;
}

static int camelgw_tryconnect(void)
{
    struct sockaddr_un sunaddr;
    int res;
    ast_consock = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (ast_consock < 0) {
	fprintf(stderr, "Unable to create socket: %s\n", strerror(errno));
	return 0;
    }
    memset(&sunaddr, 0, sizeof(sunaddr));
    sunaddr.sun_family = AF_LOCAL;
    camelgw_copy_string(sunaddr.sun_path, camelgw_config_CAMELGW_SOCKET, sizeof(sunaddr.sun_path));
    res = connect(ast_consock, (struct sockaddr *)&sunaddr, sizeof(sunaddr));
    if (res) {
	close(ast_consock);
	ast_consock = -1;
	return 0;
    } else
	return 1;
}


struct console_state_data {
    char verbose_line_level;
};

static int console_state_init(void *ptr)
{
    struct console_state_data *state = ptr;
    state->verbose_line_level = 0;
    return 0;
}

AST_THREADSTORAGE_CUSTOM(console_state, console_state_init, camelgw_free_ptr);

static int console_print(const char *s)
{
struct console_state_data *state =
    ast_threadstorage_get(&console_state, sizeof(*state));

 char prefix[80];
 const char *c;
 int num, res = 0;
 unsigned int newline;

 do {
     if (VERBOSE_HASMAGIC(s)) {

	 /* always use the given line's level, otherwise
	    we'll use the last line's level */
	 state->verbose_line_level = VERBOSE_MAGIC2LEVEL(s);

	 /* move past magic */
	 s++;

	 set_header(prefix, sizeof(prefix), state->verbose_line_level);
     } else {
	 *prefix = '\0';
     }
     c = s;

     /* for a given line separate on verbose magic, newline, and eol */
     if ((s = strchr(c, '\n'))) {
	 ++s;
	 newline = 1;
     } else {
	 s = strchr(c, '\0');
	 newline = 0;
     }

     /* check if we should write this line after calculating begin/end
	   so we process the case of a higher level line embedded within
	   two lower level lines */
     if (state->verbose_line_level > option_verbose) {
	 continue;
     }

     if (!camelgw_strlen_zero(prefix)) {
	 fputs(prefix, stdout);
     }

     num = s - c;
     if (fwrite(c, sizeof(char), num, stdout) < num) {
	 break;
     }

     if (!res) {
	 /* if at least some info has been written
	    we'll want to return true */
	 res = 1;
     }
 } while (*s);

 if (newline) {
     /* if ending on a newline then reset last level to zero
	since what follows may be not be logging output */
     state->verbose_line_level = 0;
 }

 if (res) {
     fflush(stdout);
 }

 return res;
}

static int camelgw_all_zeros(const char *s)
{
    while (*s) {
	if (*s > 32)
	    return 0;
	s++;
    }
    return 1;
}


/*! \brief NULL handler so we can collect the child exit status */
static void _null_sig_handler(int sig)
{
}

static struct sigaction null_sig_handler = {
    .sa_handler = _null_sig_handler,
    .sa_flags = SA_RESTART,
};

static struct sigaction ignore_sig_handler = {
    .sa_handler = SIG_IGN,
};

CAMELGW_MUTEX_DEFINE_STATIC(safe_system_lock);
/*! \brief Keep track of how many threads are currently trying to wait*() on
 *  a child process
 */
static unsigned int safe_system_level = 0;
static struct sigaction safe_system_prev_handler;


/* void ast_replace_sigchld(void) */
/* { */
/*     unsigned int level; */

/*     ast_mutex_lock(&safe_system_lock); */
/*     level = safe_system_level++; */

/*     /\* only replace the handler if it has not already been done *\/ */
/*     if (level == 0) { */
/* 	sigaction(SIGCHLD, &null_sig_handler, &safe_system_prev_handler); */
/*     } */

/*     ast_mutex_unlock(&safe_system_lock); */
/* } */

/* void ast_unreplace_sigchld(void) */
/* { */
/*     unsigned int level; */

/*     ast_mutex_lock(&safe_system_lock); */
/*     level = --safe_system_level; */

/*     /\* only restore the handler if we are the last one *\/ */
/*     if (level == 0) { */
/* 	sigaction(SIGCHLD, &safe_system_prev_handler, NULL); */
/*     } */

/*     ast_mutex_unlock(&safe_system_lock); */
/* } */

/*! \brief fork and perform other preparations for spawning applications */
static pid_t safe_exec_prep(int dualfork)
{
    pid_t pid;

 #if defined(HAVE_WORKING_FORK) || defined(HAVE_WORKING_VFORK) 
    //ast_replace_sigchld(); 

/* #ifdef HAVE_WORKING_FORK */
     pid = fork();
/* #else */
/*     pid = vfork(); */
/* #endif */

/*     if (pid == 0) { */
/* #ifdef HAVE_CAP */
/* 	cap_t cap = cap_from_text("cap_net_admin-eip"); */

/* 	if (cap_set_proc(cap)) { */
/* 	    /\* Careful with order! Logging cannot happen after we close FDs *\/ */
/* 	    ast_log(LOG_WARNING, "Unable to remove capabilities.\n"); */
/* 	} */
/* 	cap_free(cap); */
/* #endif */
/* #ifdef HAVE_WORKING_FORK */
/* 	if (ast_opt_high_priority) { */
/* 	    ast_set_priority(0); */
/* 	} */
/* 	/\* Close file descriptors and launch system command *\/ */
/* 	ast_close_fds_above_n(STDERR_FILENO); */
/* #endif */
/* 	if (dualfork) { */
/* #ifdef HAVE_WORKING_FORK */
/* 	    pid = fork(); */
/* #else */
/* 	    pid = vfork(); */
/* #endif */
/* 	    if (pid < 0) { */
/* 		/\* Second fork failed. *\/ */
/* 		/\* No logger available. *\/ */
/* 		_exit(1); */
/* 	    } */

/* 	    if (pid > 0) { */
/* 		/\* This is the first fork, exit so the reaper finishes right away. *\/ */
/* 		_exit(0); */
/* 	    } */

/* 	    /\* This is the second fork.  The first fork will exit immediately so */
/* 	     * Asterisk doesn't have to wait for completion. */
/* 	     * ast_safe_system("cmd &") would run in the background, but the '&' */
/* 	     * cannot be added with ast_safe_execvp, so we have to double fork. */
/* 	     *\/ */
/* 	} */
/*     } */

     if (pid < 0) { 
 	camelgw_log(LOG_WARNING, "Fork failed: %s\n", strerror(errno)); 
     } 
	  #else 
     camelgw_log(LOG_WARNING, "Fork failed: %s\n", strerror(ENOTSUP)); 
     pid = -1; 
	  #endif 

    return pid;
}

/*! \brief wait for spawned application to complete and unreplace sigchld */
static int safe_exec_wait(pid_t pid)
{
    int res = -1;

/* #if defined(HAVE_WORKING_FORK) || defined(HAVE_WORKING_VFORK) */
/*     if (pid > 0) { */
/* 	for (;;) { */
/* 	    int status; */

/* 	    res = waitpid(pid, &status, 0); */
/* 	    if (res > -1) { */
/* 		res = WIFEXITED(status) ? WEXITSTATUS(status) : -1; */
/* 		break; */
/* 	    } */
/* 	    if (errno != EINTR) { */
/* 		break; */
/* 	    } */
/* 	} */
/*     } */

/*     ast_unreplace_sigchld(); */
/* #endif */

    return res;
}

int ast_safe_execvp(int dualfork, const char *file, char *const argv[])
{
    pid_t pid = safe_exec_prep(dualfork);

    if (pid == 0) {
	execvp(file, argv);
	_exit(1);
	/* noreturn from _exit */
    }

    return safe_exec_wait(pid);
}



int ast_safe_system(const char *s)
{
    pid_t pid = safe_exec_prep(0);

    if (pid == 0) {
	execl("/bin/sh", "/bin/sh", "-c", s, (char *) NULL);
	_exit(1);
	/* noreturn from _exit */
    }

    return safe_exec_wait(pid);
}




/*  static struct ast_str *prompt = NULL; */

/*  static char *cli_prompt(EditLine *editline) */
/*  { */
/*      char tmp[100]; */
/*      char *pfmt; */
/*      int color_used = 0; */
/*      static int cli_prompt_changes = 0; */
/*      struct passwd *pw; */
/*      struct group *gr; */

/*      if (prompt == NULL) { */
/* 	 prompt = camelgw_str_create(100); */
/*      } else if (!cli_prompt_changes) { */
/* 	 return camelgw_str_buffer(prompt); */
/*      } else { */
/* 	 ast_str_reset(prompt); */
/*      } */

/*      if ((pfmt = getenv("ASTERISK_PROMPT"))) { */
/* 	 char *t = pfmt; */
/* 	 struct timeval ts = camelgw_tvnow(); */
/* 	 while (*t != '\0') { */
/* 	     if (*t == '%') { */
/* 		 char hostname[MAXHOSTNAMELEN] = ""; */
/* 		 int i, which; */
/* 		 struct ast_tm tm = { 0, }; */
/* 		 int fgcolor = COLOR_WHITE, bgcolor = COLOR_BLACK; */

/* 		 t++; */
/* 		 switch (*t) { */
/* 		 case 'C': /\* color *\/ */
/* 		     t++; */
/* 		     if (sscanf(t, "%30d;%30d%n", &fgcolor, &bgcolor, &i) == 2) { */
/* 			 ast_term_color_code(&prompt, fgcolor, bgcolor); */
/* 			 t += i - 1; */
/* 		     } else if (sscanf(t, "%30d%n", &fgcolor, &i) == 1) { */
/* 			 ast_term_color_code(&prompt, fgcolor, 0); */
/* 			 t += i - 1; */
/* 		     } */

/* 		     /\* If the color has been reset correctly, then there's no need to reset it later *\/ */
/* 		     color_used = ((fgcolor == COLOR_WHITE) && (bgcolor == COLOR_BLACK)) ? 0 : 1; */
/* 		     break; */
/* 		 case 'd': /\* date *\/ */
/* 		     if (ast_localtime(&ts, &tm, NULL)) { */
/* 			 ast_strftime(tmp, sizeof(tmp), "%Y-%m-%d", &tm); */
/* 			 ast_str_append(&prompt, 0, "%s", tmp); */
/* 			 cli_prompt_changes++; */
/* 		     } */
/* 		     break; */
/* 		 case 'g': /\* group *\/ */
/* 		     if ((gr = getgrgid(getgid()))) { */
/* 			 ast_str_append(&prompt, 0, "%s", gr->gr_name); */
/* 		     } */
/* 		     break; */
/* 		 case 'h': /\* hostname *\/ */
/* 		     if (!gethostname(hostname, sizeof(hostname) - 1)) { */
/* 			 ast_str_append(&prompt, 0, "%s", hostname); */
/* 		     } else { */
/* 			 ast_str_append(&prompt, 0, "%s", "localhost"); */
/* 		     } */
/* 		     break; */
/* 		 case 'H': /\* short hostname *\/ */
/* 		     if (!gethostname(hostname, sizeof(hostname) - 1)) { */
/* 			 char *dotptr; */
/* 			 if ((dotptr = strchr(hostname, '.'))) { */
/* 			     *dotptr = '\0'; */
/* 			 } */
/* 			 ast_str_append(&prompt, 0, "%s", hostname); */
/* 		     } else { */
/* 			 ast_str_append(&prompt, 0, "%s", "localhost"); */
/* 		     } */
/* 		     break; */
/* #ifdef HAVE_GETLOADAVG */
/* 		 case 'l': /\* load avg *\/ */
/* 		     t++; */
/* 		     if (sscanf(t, "%30d", &which) == 1 && which > 0 && which <= 3) { */
/* 			 double list[3]; */
/* 			 getloadavg(list, 3); */
/* 			 ast_str_append(&prompt, 0, "%.2f", list[which - 1]); */
/* 			 cli_prompt_changes++; */
/* 		     } */
/* 		     break; */
/* #endif */
/* 		 case 's': /\* Asterisk system name (from asterisk.conf) *\/ */
/* 		     ast_str_append(&prompt, 0, "%s", camelgw_config_CAMELGW_SYSTEM_NAME); */
/* 		     break; */
/* 		 case 't': /\* time *\/ */
/* 		     if (ast_localtime(&ts, &tm, NULL)) { */
/* 			 ast_strftime(tmp, sizeof(tmp), "%H:%M:%S", &tm); */
/* 			 ast_str_append(&prompt, 0, "%s", tmp); */
/* 			 cli_prompt_changes++; */
/* 		     } */
/* 		     break; */
/* 		 case 'u': /\* username *\/ */
/* 		     if ((pw = getpwuid(getuid()))) { */
/* 			 ast_str_append(&prompt, 0, "%s", pw->pw_name); */
/* 		     } */
/* 		     break; */
/* 		 case '#': /\* process console or remote? *\/ */
/* 		     ast_str_append(&prompt, 0, "%c", ast_opt_remote ? '>' : '#'); */
/* 		     break; */
/* 		 case '%': /\* literal % *\/ */
/* 		     ast_str_append(&prompt, 0, "%c", '%'); */
/* 		     break; */
/* 		 case '\0': /\* % is last character - prevent bug *\/ */
/* 		     t--; */
/* 		     break; */
/* 		 } */
/* 	     } else { */
/* 		 ast_str_append(&prompt, 0, "%c", *t); */
/* 	     } */
/* 	     t++; */
/* 	 } */
/* 	 if (color_used) { */
/* 	     /\* Force colors back to normal at end *\/ */
/* 	     ast_term_color_code(&prompt, 0, 0); */
/* 	 } */
/*      } else { */
/* 	 ast_str_set(&prompt, 0, "%s%s", */
/* 		     remotehostname ? remotehostname : "", */
/* 		     CAMELGW_PROMPT); */
/*      } */

/*      return camelgw_str_buffer(prompt); */
/*  } */





 /* static int ast_el_initialize(void) */
 /* { */
 /*     HistEvent ev; */
 /*     char *editor, *editrc = getenv("EDITRC"); */

 /*     if (!(editor = getenv("AST_EDITMODE"))) { */
 /* 	 if (!(editor = getenv("AST_EDITOR"))) { */
 /* 	     editor = "emacs"; */
 /* 	 } */
 /*     } */

 /*     if (el != NULL) */
 /* 	 el_end(el); */
 /*     if (el_hist != NULL) */
 /* 	 history_end(el_hist); */

 /*     el = el_init("asterisk", stdin, stdout, stderr); */
 /*     el_set(el, EL_PROMPT, cli_prompt); */

 /*     el_set(el, EL_EDITMODE, 1); */
 /*     el_set(el, EL_EDITOR, editor); */
 /*     el_hist = history_init(); */
 /*     if (!el || !el_hist) */
 /* 	 return -1; */

 /*     /\* setup history with 100 entries *\/ */
 /*     history(el_hist, &ev, H_SETSIZE, 100); */

 /*     el_set(el, EL_HIST, history, el_hist); */

 /*     el_set(el, EL_ADDFN, "ed-complete", "Complete argument", cli_complete); */
 /*     /\* Bind <tab> to command completion *\/ */
 /*     el_set(el, EL_BIND, "^I", "ed-complete", NULL); */
 /*     /\* Bind ? to command completion *\/ */
 /*     el_set(el, EL_BIND, "?", "ed-complete", NULL); */
 /*     /\* Bind ^D to redisplay *\/ */
 /*     el_set(el, EL_BIND, "^D", "ed-redisplay", NULL); */
 /*     /\* Bind Delete to delete char left *\/ */
 /*     el_set(el, EL_BIND, "\\e[3~", "ed-delete-next-char", NULL); */
 /*     /\* Bind Home and End to move to line start and end *\/ */
 /*     el_set(el, EL_BIND, "\\e[1~", "ed-move-to-beg", NULL); */
 /*     el_set(el, EL_BIND, "\\e[4~", "ed-move-to-end", NULL); */
 /*     /\* Bind C-left and C-right to move by word (not all terminals) *\/ */
 /*     el_set(el, EL_BIND, "\\eOC", "vi-next-word", NULL); */
 /*     el_set(el, EL_BIND, "\\eOD", "vi-prev-word", NULL); */

 /*     if (editrc) { */
 /* 	 el_source(el, editrc); */
 /*     } */

 /*     return 0; */
 /* } */

/* #define MAX_HISTORY_COMMAND_LENGTH 256 */

/*  static int ast_el_add_history(const char *buf) */
/*  { */
/*      HistEvent ev; */
/*      char *stripped_buf; */

/*      if (el_hist == NULL || el == NULL) { */
/* 	 ast_el_initialize(); */
/*      } */
/*      if (strlen(buf) > (MAX_HISTORY_COMMAND_LENGTH - 1)) { */
/* 	 return 0; */
/*      } */

/*      stripped_buf = camelgw_strip(camelgw_strdupa(buf)); */

/*      /\* HISTCONTROL=ignoredups *\/ */
/*      if (!history(el_hist, &ev, H_FIRST) && strcmp(ev.str, stripped_buf) == 0) { */
/* 	 return 0; */
/*      } */

/*      return history(el_hist, &ev, H_ENTER, stripped_buf); */
/*  } */



/*  static int ast_el_write_history(const char *filename) */
/*  { */
/*      HistEvent ev; */

/*      if (el_hist == NULL || el == NULL) */
/* 	 ast_el_initialize(); */

/*      return (history(el_hist, &ev, H_SAVE, filename)); */
/*  } */

/*  static int ast_el_read_history(const char *filename) */
/*  { */
/*      HistEvent ev; */

/*      if (el_hist == NULL || el == NULL) { */
/* 	 ast_el_initialize(); */
/*      } */

/*      return history(el_hist, &ev, H_LOAD, filename); */
/*  } */

/*  static void ast_el_read_default_histfile(void) */
/*  { */
/*      char histfile[80] = ""; */
/*      const char *home = getenv("HOME"); */

/*      if (!camelgw_strlen_zero(home)) { */
/* 	 snprintf(histfile, sizeof(histfile), "%s/.asterisk_history", home); */
/* 	 ast_el_read_history(histfile); */
/*      } */
/*  } */

/*  static void ast_el_write_default_histfile(void) */
/*  { */
/*      char histfile[80] = ""; */
/*      const char *home = getenv("HOME"); */

/*      if (!camelgw_strlen_zero(home)) { */
/* 	 snprintf(histfile, sizeof(histfile), "%s/.asterisk_history", home); */
/* 	 ast_el_write_history(histfile); */
/* } */
/* } */






















/* This is the main console CLI command handler.  Run by the main() thread. */
/* static void consolehandler(const char *s) */
/* { */
/*     printf("%s", term_end()); */
/*     fflush(stdout); */

/*     /\* Called when readline data is available *\/ */
/*     if (!camelgw_all_zeros(s)) */
/* 	ast_el_add_history(s); */
/*     /\* The real handler for bang *\/ */
/*     if (s[0] == '!') { */
/* 	if (s[1]) */
/* 	    ast_safe_system(s+1); */
/* 	else */
/* 	    ast_safe_system(getenv("SHELL") ? getenv("SHELL") : "/bin/sh"); */
/*     } else */
/* 	camelgw_cli_command(STDOUT_FILENO, s); */
/* } */



 static int can_safely_quit(shutdown_nice_t niceness, int restart);
 static void really_quit(int num, shutdown_nice_t niceness, int restart);

 static void quit_handler(int num, shutdown_nice_t niceness, int restart)
{
    if (can_safely_quit(niceness, restart)) {
	really_quit(num, niceness, restart);
	/* No one gets here. */
    }
    /* It wasn't our time. */
}


#define SHUTDOWN_TIMEOUT 15 /* Seconds */
 /* internal
      \brief Wait for all channels to die, a timeout, or shutdown cancelled.
			      \since 13.3.0
			      
			      \param niceness Shutdown niceness in effect
			      \param seconds Number of seconds to wait or less than zero if indefinitely.
			      
			      \retval zero if waiting wasn't necessary.  We were idle.
						   \retval non-zero if we had to wait.
						  */
 static int wait_for_channels_to_die(shutdown_nice_t niceness, int seconds)
 {
     time_t start;
     time_t now;
     int waited = 0;
     
     time(&start);
     for (;;) {
	 //ast_undestroyed_channels - where defined!&&&&&	 
//	 if (!ast_undestroyed_channels() || shuttingdown != niceness) {
	 //  break;
	 //}
	 if (seconds < 0) {
	     /* No timeout so just poll every second */
	     sleep(1);
	 } else {
	     time(&now);
	     
	     /* Wait up to the given seconds for all channels to go away */
	     if (seconds < (now - start)) {
		 break;
	     }
	     
	     /* Sleep 1/10 of a second */
	     usleep(100000);
	 }
	 waited = 1;
     }
     return waited;
 }

 static int can_safely_quit(shutdown_nice_t niceness, int restart)
 {
     int waited = 0;
     
     /* Check if someone else isn't already doing this. */
     /* camelgw_mutex_lock(&safe_system_lock); */
     /* if (shuttingdown != NOT_SHUTTING_DOWN && niceness >= shuttingdown) { */
     /* 	 /\* Already in progress and other request was less nice. *\/ */
     /* 	 ast_mutex_unlock(&safe_system_lock); */
     /* 	 ast_verbose("Ignoring asterisk %s request, already in progress.\n", restart ? "restart" : "shutdown"); */
     /* 	 return 0; */
     /* } */
     /* shuttingdown = niceness; */
     /* ast_mutex_unlock(&safe_system_lock); */

     /* /\* Try to get as many CDRs as possible submitted to the backend engines */
     /*  * (if in batch mode). really_quit happens to call it again when running */
     /*  * the atexit handlers, otherwise this would be a bit early. *\/ */
     /* ast_cdr_engine_term(); */
     
     /* /\* */
     /*  * Shutdown the message queue for the technology agnostic message channel. */
     /*  * This has to occur before we pause shutdown pending ast_undestroyed_channels. */
     /*  * */
     /*  * XXX This is not reversed on shutdown cancel. */
     /*  *\/ */
     /* ast_msg_shutdown(); */
     
     /* if (niceness == SHUTDOWN_NORMAL) { */
     /* 	 /\* Begin shutdown routine, hanging up active channels *\/ */
     /* 	 ast_begin_shutdown(); */
     /* 	 if (ast_opt_console) { */
     /* 	     ast_verb(0, "Beginning asterisk %s....\n", restart ? "restart" : "shutdown"); */
     /* 	 } */
     /* 	 ast_softhangup_all(); */
     /* 	 waited |= wait_for_channels_to_die(niceness, SHUTDOWN_TIMEOUT); */
     /* } else if (niceness >= SHUTDOWN_NICE) { */
     /* 	 if (niceness != SHUTDOWN_REALLY_NICE) { */
     /* 	     ast_begin_shutdown(); */
     /* 	 } */
     /* 	 if (ast_opt_console) { */
     /* 	     ast_verb(0, "Waiting for inactivity to perform %s...\n", restart ? "restart" : "halt"); */
     /* 	 } */
     /* 	 waited |= wait_for_channels_to_die(niceness, -1); */
     /* } */
     
     /* /\* Re-acquire lock and check if someone changed the niceness, in which */
     /*  * case someone else has taken over the shutdown. */
     /*  *\/ */
     /* ast_mutex_lock(&safe_system_lock); */
     /* if (shuttingdown != niceness) { */
     /* 	 if (shuttingdown == NOT_SHUTTING_DOWN && ast_opt_console) { */
     /* 	     ast_verb(0, "Asterisk %s cancelled.\n", restart ? "restart" : "shutdown"); */
     /* 	 } */
     /* 	 ast_mutex_unlock(&safe_system_lock); */
     /* 	 return 0; */
     /* } */
     
     /* if (niceness >= SHUTDOWN_REALLY_NICE) { */
     /* 	 shuttingdown = SHUTTING_DOWN; */
     /* 	 ast_mutex_unlock(&safe_system_lock); */
	 
     /* 	 /\* No more Mr. Nice guy.  We are committed to shutting down now. *\/ */
     /* 	 ast_begin_shutdown(); */
     /* 	 ast_softhangup_all(); */
     /* 	 waited |= wait_for_channels_to_die(SHUTTING_DOWN, SHUTDOWN_TIMEOUT); */
	 
     /* 	 ast_mutex_lock(&safe_system_lock); */
     /* } */
     /* shuttingdown = SHUTTING_DOWN_FINAL; */
     /* ast_mutex_unlock(&safe_system_lock); */
     
     /* if (niceness >= SHUTDOWN_NORMAL && waited) { */
     /* 							 /\* */
     /* 							  * We were not idle.  Give things in progress a chance to */
     /* 							  * recognize the final shutdown phase. */
     /* 							  *\/ */
     /* 	 sleep(1); */
//     }
     return 1;
 }
 
 /*! Called when exiting is certain. */
 static void really_quit(int num, shutdown_nice_t niceness, int restart)
 {
     int active_channels;
     /* struct ast_json *json_object = NULL; */
     /* int run_cleanups = niceness >= SHUTDOWN_NICE; */
     
     /* if (run_cleanups && modules_shutdown()) { */
     /* 	 ast_verb(0, "Some modules could not be unloaded, switching to fast shutdown\n"); */
     /* 	 run_cleanups = 0; */
     /* } */
     
     /* if (!restart) { */
     /* 	 ast_sd_notify("STOPPING=1"); */
     /* } */
     /* if (ast_opt_console || (ast_opt_remote && !ast_opt_exec)) { */
     /* 	 ast_el_write_default_histfile(); */
     /* 	 if (consolethread == AST_PTHREADT_NULL || consolethread == pthread_self()) { */
     /* 	     /\* Only end if we are the consolethread, otherwise there's a race with that thread. *\/ */
     /* 	     if (el != NULL) { */
     /* 		 el_end(el); */
     /* 	     } */
     /* 	     if (el_hist != NULL) { */
     /* 		 history_end(el_hist); */
     /* 	     } */
     /* 	 } else if (mon_sig_flags == pthread_self()) { */
     /* 	     if (consolethread != AST_PTHREADT_NULL) { */
     /* 		 pthread_kill(consolethread, SIGURG); */
     /* 	     } */
     /* 	 } */
     /* } */
     /* active_channels = ast_active_channels(); */
     /* /\* Don't publish messages if we're a remote console - we won't have all of the Stasis */
     /*  * topics or message types */
     /*  *\/ */
     /* if (!ast_opt_remote) { */
     /* 	 json_object = ast_json_pack("{s: s, s: s}", */
     /* 				     "Shutdown", active_channels ? "Uncleanly" : "Cleanly", */
     /* 				     "Restart", restart ? "True" : "False"); */
     /* 	 ast_manager_publish_event("Shutdown", EVENT_FLAG_SYSTEM, json_object); */
     /* 	 ast_json_unref(json_object); */
     /* 	 json_object = NULL; */
     /* } */
     /* ast_verb(0, "Asterisk %s ending (%d).\n", */
     /* 	      active_channels ? "uncleanly" : "cleanly", num); */
     
     /* ast_verb(0, "Executing last minute cleanups\n"); */
     /* ast_run_atexits(run_cleanups); */
     
     /* ast_debug(1, "Asterisk ending (%d).\n", num); */
     /* if (ast_socket > -1) { */
     /* 	 pthread_cancel(lthread); */
     /* 	 close(ast_socket); */
     /* 	 ast_socket = -1; */
     /* 	 if (!ast_socket_is_sd) { */
     /* 	     unlink(ast_config_AST_SOCKET); */
     /* 	 } */
     /* 	 pthread_kill(lthread, SIGURG); */
     /* 	 pthread_join(lthread, NULL); */
     /* } */
     /* if (ast_consock > -1) */
     /* 	 close(ast_consock); */
     /* if (!ast_opt_remote) */
     /* 	 unlink(ast_config_AST_PID); */
     /* if (sig_alert_pipe[0]) */
     /* 	 close(sig_alert_pipe[0]); */
     /* if (sig_alert_pipe[1]) */
     /* 	 close(sig_alert_pipe[1]); */
     /* printf("%s", term_quit()); */
     /* if (restart) { */
     /* 	 int i; */
     /* 	 ast_verb(0, "Preparing for Asterisk restart...\n"); */
     /* 	 /\* Mark all FD's for closing on exec *\/ */
     /* 	 for (i = 3; i < 32768; i++) { */
     /* 	     fcntl(i, F_SETFD, FD_CLOEXEC); */
     /* 	 } */
     /* 	 ast_verb(0, "Asterisk is now restarting...\n"); */
     /* 	 restartnow = 1; */
	 
     /* 	 /\* close logger *\/ */
     /* 	 close_logger(); */
     /* 	 clean_time_zones(); */
	 
     /* 	 /\* If there is a consolethread running send it a SIGHUP */
     /* 	    so it can execvp, otherwise we can do it ourselves *\/ */
     /* 	 if ((consolethread != AST_PTHREADT_NULL) && (consolethread != pthread_self())) { */
     /* 	     pthread_kill(consolethread, SIGHUP); */
     /* 	     /\* Give the signal handler some time to complete *\/ */
     /* 	     sleep(2); */
     /* 	 } else */
     /* 	     execvp(_argv[0], _argv); */
	 
     /* } else { */
     /* 	 /\* close logger *\/ */
     /* 	 close_logger(); */
     /* 	 clean_time_zones(); */
     /* } */
     
     /* exit(0); */
 }
 

/*  static void __quit_handler(int num) */
/*  { */
/*      int a = 0; */
/*      sig_flags.need_quit = 1; */
/*      if (sig_alert_pipe[1] != -1) { */
/*     if (write(sig_alert_pipe[1], &a, sizeof(a)) < 0) { */
/*     fprintf(stderr, "quit_handler: write() failed: %s\n", strerror(errno)); */
/* } */
/* } */
/*      /\* There is no need to restore the signal handler here, since the app */
/*       * is going to exit *\/ */
/* } */
 
/*  static void __remote_quit_handler(int num) */
/*  { */
/*      sig_flags.need_quit = 1; */
/* } */







/* static int remoteconsolehandler(const char *s) */
/* { */
/*     int ret = 0; */

/*     /\* Called when readline data is available *\/ */
/*     if (!camelgw_all_zeros(s)) */
/* 	ast_el_add_history(s); */

/*     while (isspace(*s)) { */
/* 	s++; */
/*     } */

/*     /\* The real handler for bang *\/ */
/*     if (s[0] == '!') { */
/* 	if (s[1]) */
/* 	    ast_safe_system(s+1); */
/* 	else */
/* 	    ast_safe_system(getenv("SHELL") ? getenv("SHELL") : "/bin/sh"); */
/* 	ret = 1; */
/*     } else if ((strncasecmp(s, "quit", 4) == 0 || strncasecmp(s, "exit", 4) == 0) && */
/* 	       (s[4] == '\0' || isspace(s[4]))) { */
/* 	quit_handler(0, SHUTDOWN_FAST, 0); */
/* 	ret = 1; */
/*     } else if (s[0]) { */
/* 	char *shrunk = camelgw_strdupa(s); */
/* 	char *cur; */
/* 	char *prev; */

/* 	/\* */
/* 	 * Remove duplicate spaces from shrunk for matching purposes. */
/* 	 * */
/* 	 * shrunk has at least one character in it to start with or we */
/* 	 * couldn't get here. */
/* 	 *\/ */
/* 	for (prev = shrunk, cur = shrunk + 1; *cur; ++cur) { */
/* 	    if (*prev == ' ' && *cur == ' ') { */
/* 		/\* Skip repeated space delimiter. *\/ */
/* 		continue; */
/* 	    } */
/* 	    *++prev = *cur; */
/* 	} */
/* 	*++prev = '\0'; */

/* 	if (strncasecmp(shrunk, "core set verbose ", 17) == 0) { */
/* 	    /\* */
/* 	     * We need to still set the rasterisk option_verbose in case we are */
/* 	     * talking to an earlier version which doesn't prefilter verbose */
/* 	     * levels.  This is really a compromise as we should always take */
/* 	     * whatever the server sends. */
/* 	     *\/ */

/* 	    if (!strncasecmp(shrunk + 17, "off", 3)) { */
/* 		ast_verb_console_set(0); */
/* 	    } else { */
/* 		int verbose_new; */
/* 		int atleast; */

/* 		atleast = 8; */
/* 		if (strncasecmp(shrunk + 17, "atleast ", atleast)) { */
/* 		    atleast = 0; */
/* 		} */

/* 		if (sscanf(shrunk + 17 + atleast, "%30d", &verbose_new) == 1) { */
/* 		    if (!atleast || ast_verb_console_get() < verbose_new) { */
/* 			ast_verb_console_set(verbose_new); */
/* 		    } */
/* 		} */
/* 	    } */
/* 	} */
/*     } */

/*     return ret; */
/* } */


/* #ifdef HAVE_LIBEDIT_IS_UNICODE */
/*  static int ast_el_read_char(EditLine *editline, wchar_t *cp) */
/* #else */
/*      static int ast_el_read_char(EditLine *editline, char *cp) */
/* #endif */
/*  { */
/*      int num_read = 0; */
/*      int lastpos = 0; */
/*      struct pollfd fds[2]; */
/*      int res; */
/*      int max; */
/* #define EL_BUF_SIZE 512 */
/*      char buf[EL_BUF_SIZE]; */

/*      for (;;) { */
/* 	 max = 1; */
/* 	 fds[0].fd = ast_consock; */
/* 	 fds[0].events = POLLIN; */
/* 	 if (!ast_opt_exec) { */
/* 	     fds[1].fd = STDIN_FILENO; */
/* 	     fds[1].events = POLLIN; */
/* 	     max++; */
/* 	 } */
/* 	 res = poll(fds, max, -1); */
/* 	 if (res < 0) { */
/* 	     //    if (sig_flags.need_quit || sig_flags.need_quit_handler) */
/* 	     //break; */
/*     if (errno == EINTR) */
/* 	continue; */
/*     fprintf(stderr, "poll failed: %s\n", strerror(errno)); */
/*     break; */
/* 	 } */

/* 	 if (!ast_opt_exec && fds[1].revents) { */
/* 	     char c = '\0'; */
/* 	     num_read = read(STDIN_FILENO, &c, 1); */
/* 	     if (num_read < 1) { */
/* 		 break; */
/* 	     } else { */
/* #ifdef HAVE_LIBEDIT_IS_UNICODE */
/* 		 *cp = btowc(c); */
/* #else */
/* 		 *cp = c; */
/* #endif */
/* 		 return (num_read); */
/* 	     } */
/* 	 } */
/* 	 if (fds[0].revents) { */
/* 	     res = read(ast_consock, buf, sizeof(buf) - 1); */
/* 	     /\* if the remote side disappears exit *\/ */
/* 	     if (res < 1) { */
/* 		 fprintf(stderr, "\nDisconnected from Asterisk server\n"); */
/* 		 if (!ast_opt_reconnect) { */
/* 		     quit_handler(0, SHUTDOWN_FAST, 0); */
/* 		 } else { */
/* 		     int tries; */
/* 		     int reconnects_per_second = 20; */
/* 		     fprintf(stderr, "Attempting to reconnect for 30 seconds\n"); */
/* 		     for (tries = 0; tries < 30 * reconnects_per_second; tries++) { */
/* 			 if (camelgw_tryconnect()) { */
/* 			     fprintf(stderr, "Reconnect succeeded after %.3f seconds\n", 1.0 / reconnects_per_second * tries); */
/* 			     printf("%s", term_quit()); */
/* 			     WELCOME_MESSAGE; */
/* 			     send_rasterisk_connect_commands(); */
/* 			     break; */
/* 			 } else */
/* 			     usleep(1000000 / reconnects_per_second); */
/* 		     } */
/* 		     if (tries >= 30 * reconnects_per_second) { */
/* 			 fprintf(stderr, "Failed to reconnect for 30 seconds.  Quitting.\n"); */
/* 			 quit_handler(0, SHUTDOWN_FAST, 0); */
/* 		     } */
/* 		 } */
/* 		 continue; */
/* 	     } */

/* 	     buf[res] = '\0'; */

/* 	     /\* Write over the CLI prompt *\/ */
/* 	     if (!ast_opt_exec && !lastpos) { */
/* 		 if (write(STDOUT_FILENO, "\rK", 5) < 0) { */
/* 		 } */
/* 	     } */

/* 	     console_print(buf); */

/* 	     if ((res < EL_BUF_SIZE - 1) && ((buf[res-1] == '\n') || (res >= 2 && buf[res-2] == '\n'))) { */
/* #ifdef HAVE_LIBEDIT_IS_UNICODE */
/* 		 *cp = btowc(CC_REFRESH); */
/* #else */
/* 		 *cp = CC_REFRESH; */
/* #endif */
/* 		 return(1); */
/* 	     } else { */
/* 		 lastpos = 1; */
/* 	     } */
/* 	 } */
/*      } */

/* #ifdef HAVE_LIBEDIT_IS_UNICODE */
/*      *cp = btowc('\0'); */
/* #else */
/*      *cp = '\0'; */
/* #endif */

/*      return (0); */
/*  } */

/*  static struct ast_str *prompt = NULL; */

/*  static char *cli_prompt(EditLine *editline) */
/*  { */
/*      char tmp[100]; */
/*      char *pfmt; */
/*      int color_used = 0; */
/*      static int cli_prompt_changes = 0; */
/*      struct passwd *pw; */
/*      struct group *gr; */

/*      if (prompt == NULL) { */
/* 	 prompt = camelgw_str_create(100); */
/*      } else if (!cli_prompt_changes) { */
/* 	 return camelgw_str_buffer(prompt); */
/*      } else { */
/* 	 ast_str_reset(prompt); */
/*      } */

/*      if ((pfmt = getenv("ASTERISK_PROMPT"))) { */
/* 	 char *t = pfmt; */
/* 	 struct timeval ts = ast_tvnow(); */
/* 	 while (*t != '\0') { */
/* 	     if (*t == '%') { */
/* 		 char hostname[MAXHOSTNAMELEN] = ""; */
/* 		 int i, which; */
/* 		 struct ast_tm tm = { 0, }; */
/* 		 int fgcolor = COLOR_WHITE, bgcolor = COLOR_BLACK; */

/* 		 t++; */
/* 		 switch (*t) { */
/* 		 case 'C': /\* color *\/ */
/* 		     t++; */
/* 		     if (sscanf(t, "%30d;%30d%n", &fgcolor, &bgcolor, &i) == 2) { */
/* 			 ast_term_color_code(&prompt, fgcolor, bgcolor); */
/* 			 t += i - 1; */
/* 		     } else if (sscanf(t, "%30d%n", &fgcolor, &i) == 1) { */
/* 			 ast_term_color_code(&prompt, fgcolor, 0); */
/* 			 t += i - 1; */
/* 		     } */

/* 		     /\* If the color has been reset correctly, then there's no need to reset it later *\/ */
/* 		     color_used = ((fgcolor == COLOR_WHITE) && (bgcolor == COLOR_BLACK)) ? 0 : 1; */
/* 		     break; */
/* 		 case 'd': /\* date *\/ */
/* 		     if (ast_localtime(&ts, &tm, NULL)) { */
/* 			 ast_strftime(tmp, sizeof(tmp), "%Y-%m-%d", &tm); */
/* 			 ast_str_append(&prompt, 0, "%s", tmp); */
/* 			 cli_prompt_changes++; */
/* 		     } */
/* 		     break; */
/* 		 case 'g': /\* group *\/ */
/* 		     if ((gr = getgrgid(getgid()))) { */
/* 			 ast_str_append(&prompt, 0, "%s", gr->gr_name); */
/* 		     } */
/* 		     break; */
/* 		 case 'h': /\* hostname *\/ */
/* 		     if (!gethostname(hostname, sizeof(hostname) - 1)) { */
/* 			 ast_str_append(&prompt, 0, "%s", hostname); */
/* 		     } else { */
/* 			 ast_str_append(&prompt, 0, "%s", "localhost"); */
/* 		     } */
/* 		     break; */
/* 		 case 'H': /\* short hostname *\/ */
/* 		     if (!gethostname(hostname, sizeof(hostname) - 1)) { */
/* 			 char *dotptr; */
/* 			 if ((dotptr = strchr(hostname, '.'))) { */
/* 			     *dotptr = '\0'; */
/* 			 } */
/* 			 ast_str_append(&prompt, 0, "%s", hostname); */
/* 		     } else { */
/* 			 ast_str_append(&prompt, 0, "%s", "localhost"); */
/* 		     } */
/* 		     break; */
/* #ifdef HAVE_GETLOADAVG */
/* 		 case 'l': /\* load avg *\/ */
/* 		     t++; */
/* 		     if (sscanf(t, "%30d", &which) == 1 && which > 0 && which <= 3) { */
/* 			 double list[3]; */
/* 			 getloadavg(list, 3); */
/* 			 ast_str_append(&prompt, 0, "%.2f", list[which - 1]); */
/* 			 cli_prompt_changes++; */
/* 		     } */
/* 		     break; */
/* #endif */
/* 		 case 's': /\* Asterisk system name (from asterisk.conf) *\/ */
/* 		     ast_str_append(&prompt, 0, "%s", ast_config_AST_SYSTEM_NAME); */
/* 		     break; */
/* 		 case 't': /\* time *\/ */
/* 		     if (ast_localtime(&ts, &tm, NULL)) { */
/* 			 ast_strftime(tmp, sizeof(tmp), "%H:%M:%S", &tm); */
/* 			 ast_str_append(&prompt, 0, "%s", tmp); */
/* 			 cli_prompt_changes++; */
/* 		     } */
/* 		     break; */
/* 		 case 'u': /\* username *\/ */
/* 		     if ((pw = getpwuid(getuid()))) { */
/* 			 ast_str_append(&prompt, 0, "%s", pw->pw_name); */
/* 		     } */
/* 		     break; */
/* 		 case '#': /\* process console or remote? *\/ */
/* 		     ast_str_append(&prompt, 0, "%c", ast_opt_remote ? '>' : '#'); */
/* 		     break; */
/* 		 case '%': /\* literal % *\/ */
/* 		     ast_str_append(&prompt, 0, "%c", '%'); */
/* 		     break; */
/* 		 case '\0': /\* % is last character - prevent bug *\/ */
/* 		     t--; */
/* 		     break; */
/* 		 } */
/* 	     } else { */
/* 		 ast_str_append(&prompt, 0, "%c", *t); */
/* 	     } */
/* 	     t++; */
/* 	 } */
/* 	 if (color_used) { */
/* 	     /\* Force colors back to normal at end *\/ */
/* 	     ast_term_color_code(&prompt, 0, 0); */
/* 	 } */
/*      } else { */
/* 	 ast_str_set(&prompt, 0, "%s%s", */
/* 		     remotehostname ? remotehostname : "", */
/* 		     ASTERISK_PROMPT); */
/*      } */

/*      return camelgw_str_buffer(prompt); */
/*  } */

 static void destroy_match_list(char **match_list, int matches)
 {
     if (match_list) {
	 int idx;

	 for (idx = 0; idx < matches; ++idx) {
	     camelgw_free(match_list[idx]);
	 }
	 camelgw_free(match_list);
     }
 }

 static char **ast_el_strtoarr(char *buf)
 {
     char *retstr;
     char **match_list = NULL;
     char **new_list;
     size_t match_list_len = 1;
     int matches = 0;

     while ((retstr = strsep(&buf, " "))) {
	 if (!strcmp(retstr, CAMELGW_CLI_COMPLETE_EOF)) {
	     break;
	 }
	 if (matches + 1 >= match_list_len) {
	     match_list_len <<= 1;
	     new_list = camelgw_realloc(match_list, match_list_len * sizeof(char *));
	     if (!new_list) {
		 destroy_match_list(match_list, matches);
		 return NULL;
	     }
	     match_list = new_list;
	 }

	 retstr = camelgw_strdup(retstr);
	 if (!retstr) {
	     destroy_match_list(match_list, matches);
	     return NULL;
	 }
	 match_list[matches++] = retstr;
     }

     if (!match_list) {
	 return NULL;
     }

     if (matches >= match_list_len) {
	 new_list = camelgw_realloc(match_list, (match_list_len + 1) * sizeof(char *));
	 if (!new_list) {
	     destroy_match_list(match_list, matches);
	     return NULL;
	 }
	 match_list = new_list;
     }

     match_list[matches] = NULL;

     return match_list;
 }

 static int ast_el_sort_compare(const void *i1, const void *i2)
 {
     char *s1, *s2;

     s1 = ((char **)i1)[0];
     s2 = ((char **)i2)[0];

     return strcasecmp(s1, s2);
 }

 static int ast_cli_display_match_list(char **matches, int len, int max)
 {
     int i, idx, limit, count;
     int screenwidth = 0;
     int numoutput = 0, numoutputline = 0;

     //TODO WTF?     screenwidth = ast_get_termcols(STDOUT_FILENO);

     /* find out how many entries can be put on one line, with two spaces between strings */
     limit = screenwidth / (max + 2);
     if (limit == 0)
	 limit = 1;

     /* how many lines of output */
     count = len / limit;
     if (count * limit < len)
	 count++;

     idx = 1;

     qsort(&matches[0], (size_t)(len), sizeof(char *), ast_el_sort_compare);

     for (; count > 0; count--) {
	 numoutputline = 0;
	 for (i = 0; i < limit && matches[idx]; i++, idx++) {

	     /* Don't print dupes */
	     if ( (matches[idx+1] != NULL && strcmp(matches[idx], matches[idx+1]) == 0 ) ) {
		 i--;
		 camelgw_free(matches[idx]);
		 matches[idx] = NULL;
		 continue;
	     }

	     numoutput++;
	     numoutputline++;
	     fprintf(stdout, "%-*s  ", max, matches[idx]);
	     camelgw_free(matches[idx]);
	     matches[idx] = NULL;
	 }
	 if (numoutputline > 0)
	     fprintf(stdout, "\n");
     }

     return numoutput;
 }


 static char *cli_complete(EditLine *editline, int ch)
 {
     int len = 0;
     char *ptr;
     int nummatches = 0;
     char **matches;
     int retval = CC_ERROR;
     char buf[2048], savechr;
     int res;

     LineInfo *lf = (LineInfo *)el_line(editline);

     savechr = *(char *)lf->cursor;
     *(char *)lf->cursor = '\0';
     ptr = (char *)lf->cursor;
     if (ptr) {
	 while (ptr > lf->buffer) {
	     if (isspace(*ptr)) {
		 ptr++;
		 break;
	     }
	     ptr--;
	 }
     }

     len = lf->cursor - ptr;

     if (ast_opt_remote) {
	 snprintf(buf, sizeof(buf), "_COMMAND NUMMATCHES \"%s\" \"%s\"", lf->buffer, ptr);
	 fdsend(ast_consock, buf);
	 if ((res = read(ast_consock, buf, sizeof(buf) - 1)) < 0) {
	     return (char*)(CC_ERROR);
	 }
	 buf[res] = '\0';
	 nummatches = atoi(buf);

	 if (nummatches > 0) {
	     char *mbuf;
	     char *new_mbuf;
	     int mlen = 0, maxmbuf = 2048;

	     /* Start with a 2048 byte buffer */
	     if (!(mbuf = camelgw_malloc(maxmbuf))) {
		 *((char *) lf->cursor) = savechr;
		 return (char *)(CC_ERROR);
	     }
	     snprintf(buf, sizeof(buf), "_COMMAND MATCHESARRAY \"%s\" \"%s\"", lf->buffer, ptr);
	     fdsend(ast_consock, buf);
	     res = 0;
	     mbuf[0] = '\0';
	     while (!strstr(mbuf, CAMELGW_CLI_COMPLETE_EOF) && res != -1) {
		 if (mlen + 1024 > maxmbuf) {
		     /* Every step increment buffer 1024 bytes */
		     maxmbuf += 1024;
		     new_mbuf = camelgw_realloc(mbuf, maxmbuf);
		     if (!new_mbuf) {
			 camelgw_free(mbuf);
			 *((char *) lf->cursor) = savechr;
			 return (char *)(CC_ERROR);
		     }
		     mbuf = new_mbuf;
		 }
		 /* Only read 1024 bytes at a time */
		 res = read(ast_consock, mbuf + mlen, 1024);
		 if (res > 0)
		     mlen += res;
	     }
	     mbuf[mlen] = '\0';

	     matches = ast_el_strtoarr(mbuf);
	     camelgw_free(mbuf);
	 } else
	     matches = (char **) NULL;
     } else {
	 char **p, *oldbuf=NULL;
	 nummatches = 0;
	 matches = ast_cli_completion_matches((char *)lf->buffer,ptr);
	 for (p = matches; p && *p; p++) {
	     if (!oldbuf || strcmp(*p,oldbuf))
		 nummatches++;
	     oldbuf = *p;
	 }
     }

     if (matches) {
	 int i;
	 int matches_num, maxlen, match_len;

	 if (matches[0][0] != '\0') {
	     el_deletestr(editline, (int) len);
	     el_insertstr(editline, matches[0]);
	     retval = CC_REFRESH;
	 }

	 if (nummatches == 1) {
	     /* Found an exact match */
	     el_insertstr(editline, " ");
	     retval = CC_REFRESH;
	 } else {
	     /* Must be more than one match */
	     for (i = 1, maxlen = 0; matches[i]; i++) {
		 match_len = strlen(matches[i]);
		 if (match_len > maxlen)
		     maxlen = match_len;
	     }
	     matches_num = i - 1;
	     if (matches_num >1) {
		 fprintf(stdout, "\n");
		 ast_cli_display_match_list(matches, nummatches, maxlen);
		 retval = CC_REDISPLAY;
	     } else {
		 el_insertstr(editline," ");
		 retval = CC_REFRESH;
	     }
	 }
	 for (i = 0; matches[i]; i++)
	     camelgw_free(matches[i]);
	 camelgw_free(matches);
     }

     *((char *) lf->cursor) = savechr;

     return (char *)(long)retval;
 }



/*  static int ast_el_write_history(const char *filename) */
/*  { */
/*      HistEvent ev; */

/*      if (el_hist == NULL || el == NULL) */
/* 	 ast_el_initialize(); */

/*      return (history(el_hist, &ev, H_SAVE, filename)); */
/*  } */

/*  static int ast_el_read_history(const char *filename) */
/*  { */
/*      HistEvent ev; */

/*      if (el_hist == NULL || el == NULL) { */
/* 	 ast_el_initialize(); */
/*      } */

/*      return history(el_hist, &ev, H_LOAD, filename); */
/*  } */

/*  static void ast_el_read_default_histfile(void) */
/*  { */
/*      char histfile[80] = ""; */
/*      const char *home = getenv("HOME"); */

/*      if (!camelgw_strlen_zero(home)) { */
/* 	 snprintf(histfile, sizeof(histfile), "%s/.asterisk_history", home); */
/* 	 ast_el_read_history(histfile); */
/*      } */
/*  } */

/*  static void ast_el_write_default_histfile(void) */
/*  { */
/*      char histfile[80] = ""; */
/*      const char *home = getenv("HOME"); */

/*      if (!camelgw_strlen_zero(home)) { */
/* 	 snprintf(histfile, sizeof(histfile), "%s/.asterisk_history", home); */
/* 	 ast_el_write_history(histfile); */
/* } */
/* } */


/*  static void ast_remotecontrol(char *data) */
/*  { */
/*     char buf[256] = ""; */
/*     int res; */
/*     char *hostname; */
/*     char *cpid; */
/*     char *version; */
/*     int pid; */
/*     char *stringp = NULL; */

/*     char *ebuf; */
/*     int num = 0; */

/*     ast_term_init(); */
/*     printf("%s", term_end()); */
/*     fflush(stdout); */

/*     memset(&sig_flags, 0, sizeof(sig_flags)); */
/*     signal(SIGINT, __remote_quit_handler); */
/*     signal(SIGTERM, __remote_quit_handler); */
/*     signal(SIGHUP, __remote_quit_handler); */

/*     if (read(ast_consock, buf, sizeof(buf) - 1) < 0) { */
/*     ast_log(LOG_ERROR, "read() failed: %s\n", strerror(errno)); */
/*     return; */
/* } */
/*     if (data) { */
/*     char prefix[] = "cli quit after "; */
/*     char *tmp = ast_alloca(strlen(data) + strlen(prefix) + 1); */
/*     sprintf(tmp, "%s%s", prefix, data); */
/*     if (write(ast_consock, tmp, strlen(tmp) + 1) < 0) { */
/*     ast_log(LOG_ERROR, "write() failed: %s\n", strerror(errno)); */
/*     if (sig_flags.need_quit || sig_flags.need_quit_handler) { */
/*     return; */
/* } */
/* } */
/* } */
/*     stringp = buf; */
/*     hostname = strsep(&stringp, "/"); */
/*     cpid = strsep(&stringp, "/"); */
/*     version = strsep(&stringp, "\n"); */
/*     if (!version) */
/* 	version = "<Version Unknown>"; */
/*     stringp = hostname; */
/*     strsep(&stringp, "."); */
/*     if (cpid) */
/* 	pid = atoi(cpid); */
/*     else */
/* 	pid = -1; */
/*     if (!data) { */
/*     send_rasterisk_connect_commands(); */
/* } */

/*     if (ast_opt_exec && data) {  /\* hack to print output then exit if asterisk -rx is used *\/ */
/*     int linefull = 1, prev_linefull = 1, prev_line_verbose = 0; */
/*     struct pollfd fds; */
/*     fds.fd = ast_consock; */
/*     fds.events = POLLIN; */
/*     fds.revents = 0; */

/*     while (ast_poll(&fds, 1, 60000) > 0) { */
/*     char buffer[512] = "", *curline = buffer, *nextline; */
/*     int not_written = 1; */

/*     if (sig_flags.need_quit || sig_flags.need_quit_handler) { */
/*     break; */
/* } */

/*     if (read(ast_consock, buffer, sizeof(buffer) - 1) <= 0) { */
/*     break; */
/* } */

/*     do { */
/*     prev_linefull = linefull; */
/*     if ((nextline = strchr(curline, '\n'))) { */
/*     linefull = 1; */
/*     nextline++; */
/* } else { */
/*     linefull = 0; */
/*     nextline = strchr(curline, '\0'); */
/* } */

/*     /\* Skip verbose lines *\/ */
/*     /\* Prev line full? | Line is verbose | Last line verbose? | Print */
/*      * TRUE            | TRUE*           | TRUE               | FALSE */
/*      * TRUE            | TRUE*           | FALSE              | FALSE */
/*      * TRUE            | FALSE*          | TRUE               | TRUE */
/*      * TRUE            | FALSE*          | FALSE              | TRUE */
/*      * FALSE           | TRUE            | TRUE*              | FALSE */
/*      * FALSE           | TRUE            | FALSE*             | TRUE */
/*      * FALSE           | FALSE           | TRUE*              | FALSE */
/*      * FALSE           | FALSE           | FALSE*             | TRUE */
/*      *\/ */
/*     if ((!prev_linefull && !prev_line_verbose) || (prev_linefull && *curline > 0)) { */
/*     prev_line_verbose = 0; */
/*     not_written = 0; */
/*     if (write(STDOUT_FILENO, curline, nextline - curline) < 0) { */
/*     ast_log(LOG_WARNING, "write() failed: %s\n", strerror(errno)); */
/* } */
/* } else { */
/*     prev_line_verbose = 1; */
/* } */
/*     curline = nextline; */
/* } while (!ast_strlen_zero(curline)); */

/*     /\* No non-verbose output in 60 seconds. *\/ */
/*     if (not_written) { */
/*     break; */
/* } */
/* } */
/*     return; */
/* } */

/*     ast_verbose("Connected to Asterisk %s currently running on %s (pid = %d)\n", version, hostname, pid); */
/*     remotehostname = hostname; */
/*     if (el_hist == NULL || el == NULL) */
/* 	ast_el_initialize(); */
/*     ast_el_read_default_histfile(); */

/*     el_set(el, EL_GETCFN, ast_el_read_char); */

/*     for (;;) { */
/*     ebuf = (char *)el_gets(el, &num); */

/*     if (sig_flags.need_quit || sig_flags.need_quit_handler) { */
/* 	break; */
/*     } */

/*     if (!ebuf && write(1, "", 1) < 0) */
/* 	break; */

/*     if (!ast_strlen_zero(ebuf)) { */
/* 	if (ebuf[strlen(ebuf)-1] == '\n') */
/* 	    ebuf[strlen(ebuf)-1] = '\0'; */
/* 	if (!remoteconsolehandler(ebuf)) { */
/* 	    res = write(ast_consock, ebuf, strlen(ebuf) + 1); */
/* 	    if (res < 1) { */
/* 		ast_log(LOG_WARNING, "Unable to write: %s\n", strerror(errno)); */
/* 		break; */
/* 	    } */
/* 	} */
/*     } */
/*     } */
/*     printf("\nDisconnected from Asterisk server\n"); */
/*  } */


 static int show_version(void)
 {
     printf("Asterisk %s\n", camelgw_get_version());
     return 0;
 }


 static int show_cli_help(void)
 {
     printf("Asterisk %s, Copyright (C) 1999 - 2016, Digium, Inc. and others.\n", camelgw_get_version());
     printf("Usage: asterisk [OPTIONS]\n");
     printf("Valid Options:\n");
     printf("   -V              Display version number and exit\n");
     printf("   -C <configfile> Use an alternate configuration file\n");
     printf("   -G <group>      Run as a group other than the caller\n");
     printf("   -U <user>       Run as a user other than the caller\n");
     printf("   -c              Provide console CLI\n");
     printf("   -d              Increase debugging (multiple d's = more debugging)\n");
#if HAVE_WORKING_FORK
     printf("   -f              Do not fork\n");
     printf("   -F              Always fork\n");
#endif
     printf("   -g              Dump core in case of a crash\n");
     printf("   -h              This help screen\n");
     printf("   -i              Initialize crypto keys at startup\n");
     printf("   -L <load>       Limit the maximum load average before rejecting new calls\n");
     printf("   -M <value>      Limit the maximum number of calls to the specified value\n");
     printf("   -m              Mute debugging and console output on the console\n");
     printf("   -n              Disable console colorization\n");
     printf("   -p              Run as pseudo-realtime thread\n");
     printf("   -q              Quiet mode (suppress output)\n");
     printf("   -r              Connect to Asterisk on this machine\n");
     printf("   -R              Same as -r, except attempt to reconnect if disconnected\n");
     printf("   -s <socket>     Connect to Asterisk via socket <socket> (only valid with -r)\n");
     printf("   -t              Record soundfiles in /var/tmp and move them where they\n");
     printf("                   belong after they are done\n");
     printf("   -T              Display the time in [Mmm dd hh:mm:ss] format for each line\n");
     printf("                   of output to the CLI\n");
     printf("   -v              Increase verbosity (multiple v's = more verbose)\n");
     printf("   -x <cmd>        Execute command <cmd> (implies -r)\n");
     printf("   -X              Enable use of #exec in asterisk.conf\n");
     printf("   -W              Adjust terminal colors to compensate for a light background\n");
     printf("\n");
     return 0;
 }


/* static int remoteconsolehandler(const char *s) */
/* { */
/*     int ret = 0; */

/*     /\* Called when readline data is available *\/ */
/*     if (!camelgw_all_zeros(s)) */
/* 	ast_el_add_history(s); */

/*     while (isspace(*s)) { */
/* 	s++; */
/*     } */

/*     /\* The real handler for bang *\/ */
/*     if (s[0] == '!') { */
/* 	if (s[1]) */
/* 	    ast_safe_system(s+1); */
/* 	else */
/* 	    ast_safe_system(getenv("SHELL") ? getenv("SHELL") : "/bin/sh"); */
/* 	ret = 1; */
/*     } else if ((strncasecmp(s, "quit", 4) == 0 || strncasecmp(s, "exit", 4) == 0) && */
/* 	       (s[4] == '\0' || isspace(s[4]))) { */
/* 	quit_handler(0, SHUTDOWN_FAST, 0); */
/* 	ret = 1; */
/*     } else if (s[0]) { */
/* 	char *shrunk = camelgw_strdupa(s); */
/* 	char *cur; */
/* 	char *prev; */

/* 	/\* */
/* 	 * Remove duplicate spaces from shrunk for matching purposes. */
/* 	 * */
/* 	 * shrunk has at least one character in it to start with or we */
/* 	 * couldn't get here. */
/* 	 *\/ */
/* 	for (prev = shrunk, cur = shrunk + 1; *cur; ++cur) { */
/* 	    if (*prev == ' ' && *cur == ' ') { */
/* 		/\* Skip repeated space delimiter. *\/ */
/* 		continue; */
/* 	    } */
/* 	    *++prev = *cur; */
/* 	} */
/* 	*++prev = '\0'; */

/* 	if (strncasecmp(shrunk, "core set verbose ", 17) == 0) { */
/* 	    /\* */
/* 	     * We need to still set the rasterisk option_verbose in case we are */
/* 	     * talking to an earlier version which doesn't prefilter verbose */
/* 	     * levels.  This is really a compromise as we should always take */
/* 	     * whatever the server sends. */
/* 	     *\/ */

/* 	    if (!strncasecmp(shrunk + 17, "off", 3)) { */
/* 		ast_verb_console_set(0); */
/* 	    } else { */
/* 		int verbose_new; */
/* 		int atleast; */

/* 		atleast = 8; */
/* 		if (strncasecmp(shrunk + 17, "atleast ", atleast)) { */
/* 		    atleast = 0; */
/* 		} */

/* 		if (sscanf(shrunk + 17 + atleast, "%30d", &verbose_new) == 1) { */
/* 		    if (!atleast || ast_verb_console_get() < verbose_new) { */
/* 			ast_verb_console_set(verbose_new); */
/* 		    } */
/* 		} */
/* 	    } */
/* 	} */
/*     } */

/*     return ret; */
/* } */

#define CAMELGW_PROMPT "*CLI> "

/*!
 * \brief Shutdown Asterisk CLI commands.
 *
 * \note These CLI commands cannot be unregistered at shutdown
 * because one of them is likely the reason for the shutdown.
 * The CLI generates a warning if a command is in-use when it is
 * unregistered.
 */
/*  static struct ast_cli_entry cli_asterisk_shutdown[] = { */
/*      AST_CLI_DEFINE(handle_stop_now, "Shut down Asterisk immediately"), */
/*      AST_CLI_DEFINE(handle_stop_gracefully, "Gracefully shut down Asterisk"), */
/*      AST_CLI_DEFINE(handle_stop_when_convenient, "Shut down Asterisk at empty call volume"), */
/*      AST_CLI_DEFINE(handle_restart_now, "Restart Asterisk immediately"), */
/*      AST_CLI_DEFINE(handle_restart_gracefully, "Restart Asterisk gracefully"), */
/*      AST_CLI_DEFINE(handle_restart_when_convenient, "Restart Asterisk at empty call volume"), */
/*  }; */

/*  static struct ast_cli_entry cli_asterisk[] = { */
/*      AST_CLI_DEFINE(handle_abort_shutdown, "Cancel a running shutdown"), */
/*      AST_CLI_DEFINE(show_warranty, "Show the warranty (if any) for this copy of Asterisk"), */
/*      AST_CLI_DEFINE(show_license, "Show the license(s) for this copy of Asterisk"), */
/*      AST_CLI_DEFINE(handle_version, "Display version info"), */
/*      AST_CLI_DEFINE(handle_bang, "Execute a shell command"), */
/* #if !defined(LOW_MEMORY) */
/*      AST_CLI_DEFINE(handle_show_threads, "Show running threads"), */
/* #if defined(HAVE_SYSINFO) || defined(HAVE_SYSCTL) */
/*      AST_CLI_DEFINE(handle_show_sysinfo, "Show System Information"), */
/* #endif */
/*      AST_CLI_DEFINE(handle_show_profile, "Display profiling info"), */
/*      AST_CLI_DEFINE(handle_show_settings, "Show some core settings"), */
/*      AST_CLI_DEFINE(handle_clear_profile, "Clear profiling info"), */
/* #endif /\* ! LOW_MEMORY *\/ */
/*  }; */

static void send_rasterisk_connect_commands(void)
     {
     char buf[80];

     /*
      * Tell the server asterisk instance about the verbose level
      * initially desired.
      */
     if (option_verbose) {
	 snprintf(buf, sizeof(buf), "core set verbose atleast %d silent", option_verbose);
	 fdsend(ast_consock, buf);
     }

     if (option_debug) {
	 snprintf(buf, sizeof(buf), "core set debug atleast %d", option_debug);
	 fdsend(ast_consock, buf);
     }

/*      if (!ast_opt_mute) { */
/* 	 fdsend(ast_consock, "logger mute silent"); */
/*      } else { */
/* 	 printf("log and verbose output currently muted ('logger mute' to unmute)\n"); */
/*      } */
  } 

/* #ifdef HAVE_LIBEDIT_IS_UNICODE */
/*  static int ast_el_read_char(EditLine *editline, wchar_t *cp) */
/* #else */
/*      static int ast_el_read_char(EditLine *editline, char *cp) */
/* #endif */
/*  { */
/*      int num_read = 0; */
/*      int lastpos = 0; */
/*      struct pollfd fds[2]; */
/*      int res; */
/*      int max; */
/* #define EL_BUF_SIZE 512 */
/*      char buf[EL_BUF_SIZE]; */

/*      for (;;) { */
/*      max = 1; */
/*      fds[0].fd = ast_consock; */
/*      fds[0].events = POLLIN; */
/*      if (!ast_opt_exec) { */
/*      fds[1].fd = STDIN_FILENO; */
/*      fds[1].events = POLLIN; */
/*      max++; */
/*      } */
/*      res = ast_poll(fds, max, -1); */
/*      if (res < 0) { */
/*      if (sig_flags.need_quit || sig_flags.need_quit_handler) */
/* 	 break; */
/*      if (errno == EINTR) */
/* 	 continue; */
/*      fprintf(stderr, "poll failed: %s\n", strerror(errno)); */
/*      break; */
/*      } */

/*      if (!ast_opt_exec && fds[1].revents) { */
/*      char c = '\0'; */
/*      num_read = read(STDIN_FILENO, &c, 1); */
/*      if (num_read < 1) { */
/*      break; */
/*      } else { */
/* #ifdef HAVE_LIBEDIT_IS_UNICODE */
/*      *cp = btowc(c); */
/* #else */
/*      *cp = c; */
/* #endif */
/*      return (num_read); */
/*      } */
/*      } */
/*      if (fds[0].revents) { */
/*      res = read(ast_consock, buf, sizeof(buf) - 1); */
/*      /\* if the remote side disappears exit *\/ */
/*      if (res < 1) { */
/*      fprintf(stderr, "\nDisconnected from Asterisk server\n"); */
/*      if (!ast_opt_reconnect) { */
/*      quit_handler(0, SHUTDOWN_FAST, 0); */
/*      } else { */
/*      int tries; */
/*      int reconnects_per_second = 20; */
/*      fprintf(stderr, "Attempting to reconnect for 30 seconds\n"); */
/*      for (tries = 0; tries < 30 * reconnects_per_second; tries++) { */
/* 	 if (camelgw_tryconnect()) { */
/* 	     fprintf(stderr, "Reconnect succeeded after %.3f seconds\n", 1.0 / reconnects_per_second * tries); */
/* 	     printf("%s", term_quit()); */
/* 	     WELCOME_MESSAGE; */
/* 	     send_rasterisk_connect_commands(); */
/* 	     break; */
/* 	 } else */
/* 	     usleep(1000000 / reconnects_per_second); */
/*      } */
/*      if (tries >= 30 * reconnects_per_second) { */
/* 	 fprintf(stderr, "Failed to reconnect for 30 seconds.  Quitting.\n"); */
/* 	 quit_handler(0, SHUTDOWN_FAST, 0); */
/*      } */
/*      } */
/*      continue; */
/*      } */

/*      buf[res] = '\0'; */

/*      /\* Write over the CLI prompt *\/ */
/*      if (!ast_opt_exec && !lastpos) { */
/* 	 if (write(STDOUT_FILENO, "\rK", 5) < 0) { */
/* 	 } */
/*      } */

/*      console_print(buf); */

/*      if ((res < EL_BUF_SIZE - 1) && ((buf[res-1] == '\n') || (res >= 2 && buf[res-2] == '\n'))) { */
/* #ifdef HAVE_LIBEDIT_IS_UNICODE */
/* 	 *cp = btowc(CC_REFRESH); */
/* #else */
/* 	 *cp = CC_REFRESH; */
/* #endif */
/* 	 return(1); */
/*      } else { */
/* 	 lastpos = 1; */
/*      } */
/*      } */
/* } */

/* #ifdef HAVE_LIBEDIT_IS_UNICODE */
/* *cp = btowc('\0'); */
/* #else */
/* *cp = '\0'; */
/* #endif */

//return (0);
//}


#ifdef HAVE_LIBEDIT_IS_UNICODE
 static int ast_el_read_char(EditLine *editline, wchar_t *cp)
#else
     static int ast_el_read_char(EditLine *editline, char *cp)
#endif
 {
     int num_read = 0;
     int lastpos = 0;
     struct pollfd fds[2];
     int res;
     int max;
#define EL_BUF_SIZE 512
     char buf[EL_BUF_SIZE];

     for (;;) {
	 max = 1;
	 fds[0].fd = ast_consock;
	 fds[0].events = POLLIN;
	 if (!ast_opt_exec) {
	     fds[1].fd = STDIN_FILENO;
	     fds[1].events = POLLIN;
	     max++;
	 }
	 res = poll(fds, max, -1);
	 if (res < 0) {
	     //    if (sig_flags.need_quit || sig_flags.need_quit_handler)
	     //break;
    if (errno == EINTR)
	continue;
    fprintf(stderr, "poll failed: %s\n", strerror(errno));
    break;
	 }

	 if (!ast_opt_exec && fds[1].revents) {
	     char c = '\0';
	     num_read = read(STDIN_FILENO, &c, 1);
	     if (num_read < 1) {
		 break;
	     } else {
#ifdef HAVE_LIBEDIT_IS_UNICODE
		 *cp = btowc(c);
#else
		 *cp = c;
#endif
		 return (num_read);
	     }
	 }
	 if (fds[0].revents) {
	     res = read(ast_consock, buf, sizeof(buf) - 1);
	     /* if the remote side disappears exit */
	     if (res < 1) {
		 fprintf(stderr, "\nDisconnected from Asterisk server\n");
		 if (!ast_opt_reconnect) {
		     quit_handler(0, SHUTDOWN_FAST, 0);
		 } else {
		     int tries;
		     int reconnects_per_second = 20;
		     fprintf(stderr, "Attempting to reconnect for 30 seconds\n");
		     for (tries = 0; tries < 30 * reconnects_per_second; tries++) {
			 if (camelgw_tryconnect()) {
			     fprintf(stderr, "Reconnect succeeded after %.3f seconds\n", 1.0 / reconnects_per_second * tries);
			     printf("%s", term_quit());
			     WELCOME_MESSAGE;
			     send_rasterisk_connect_commands();
			     break;
			 } else
			     usleep(1000000 / reconnects_per_second);
		     }
		     if (tries >= 30 * reconnects_per_second) {
			 fprintf(stderr, "Failed to reconnect for 30 seconds.  Quitting.\n");
			 quit_handler(0, SHUTDOWN_FAST, 0);
		     }
		 }
		 continue;
	     }

	     buf[res] = '\0';

	     /* Write over the CLI prompt */
	     if (!ast_opt_exec && !lastpos) {
		 if (write(STDOUT_FILENO, "\rK", 5) < 0) {
		 }
	     }

	     console_print(buf);

	     if ((res < EL_BUF_SIZE - 1) && ((buf[res-1] == '\n') || (res >= 2 && buf[res-2] == '\n'))) {
#ifdef HAVE_LIBEDIT_IS_UNICODE
		 *cp = btowc(CC_REFRESH);
#else
		 *cp = CC_REFRESH;
#endif
		 return(1);
	     } else {
		 lastpos = 1;
	     }
	 }
     }

#ifdef HAVE_LIBEDIT_IS_UNICODE
     *cp = btowc('\0');
#else
     *cp = '\0';
#endif

     return (0);
 }




 static struct ast_str *prompt = NULL;

 static char *cli_prompt(EditLine *editline)
 {
     char tmp[100];
     char *pfmt;
     int color_used = 0;
     static int cli_prompt_changes = 0;
     struct passwd *pw;
     struct group *gr;

     if (prompt == NULL) {
	 prompt = camelgw_str_create(100);
     } else if (!cli_prompt_changes) {
	 return camelgw_str_buffer(prompt);
     } else {
	 camelgw_str_reset(prompt);
     }

     if ((pfmt = getenv("ASTERISK_PROMPT"))) {
	 char *t = pfmt;
	 struct timeval ts = camelgw_tvnow();
	 while (*t != '\0') {
	     if (*t == '%') {
		 char hostname[MAXHOSTNAMELEN] = "";
		 int i, which;
		 struct ast_tm tm = { 0, };
		 int fgcolor = COLOR_WHITE, bgcolor = COLOR_BLACK;

		 t++;
		 switch (*t) {
		 case 'C': /* color */
		     t++;
		     if (sscanf(t, "%30d;%30d%n", &fgcolor, &bgcolor, &i) == 2) {
			 ast_term_color_code(&prompt, fgcolor, bgcolor);
			 t += i - 1;
		     } else if (sscanf(t, "%30d%n", &fgcolor, &i) == 1) {
			 ast_term_color_code(&prompt, fgcolor, 0);
			 t += i - 1;
		     }

		     /* If the color has been reset correctly, then there's no need to reset it later */
		     color_used = ((fgcolor == COLOR_WHITE) && (bgcolor == COLOR_BLACK)) ? 0 : 1;
		     break;
		 case 'd': /* date */
		     if (ast_localtime(&ts, &tm, NULL)) {
			 ast_strftime(tmp, sizeof(tmp), "%Y-%m-%d", &tm);
			 ast_str_append(&prompt, 0, "%s", tmp);
			 cli_prompt_changes++;
		     }
		     break;
		 case 'g': /* group */
		     if ((gr = getgrgid(getgid()))) {
			 ast_str_append(&prompt, 0, "%s", gr->gr_name);
		     }
		     break;
		 case 'h': /* hostname */
		     if (!gethostname(hostname, sizeof(hostname) - 1)) {
			 ast_str_append(&prompt, 0, "%s", hostname);
		     } else {
			 ast_str_append(&prompt, 0, "%s", "localhost");
		     }
		     break;
		 case 'H': /* short hostname */
		     if (!gethostname(hostname, sizeof(hostname) - 1)) {
			 char *dotptr;
			 if ((dotptr = strchr(hostname, '.'))) {
			     *dotptr = '\0';
			 }
			 ast_str_append(&prompt, 0, "%s", hostname);
		     } else {
			 ast_str_append(&prompt, 0, "%s", "localhost");
		     }
		     break;
#ifdef HAVE_GETLOADAVG
		 case 'l': /* load avg */
		     t++;
		     if (sscanf(t, "%30d", &which) == 1 && which > 0 && which <= 3) {
			 double list[3];
			 getloadavg(list, 3);
			 ast_str_append(&prompt, 0, "%.2f", list[which - 1]);
			 cli_prompt_changes++;
		     }
		     break;
#endif
		 case 's': /* Asterisk system name (from asterisk.conf) */
		     ast_str_append(&prompt, 0, "%s", camelgw_config_CAMELGW_SYSTEM_NAME);
		     break;
		 case 't': /* time */
		     if (ast_localtime(&ts, &tm, NULL)) {
			 ast_strftime(tmp, sizeof(tmp), "%H:%M:%S", &tm);
			 ast_str_append(&prompt, 0, "%s", tmp);
			 cli_prompt_changes++;
		     }
		     break;
		 case 'u': /* username */
		     if ((pw = getpwuid(getuid()))) {
			 ast_str_append(&prompt, 0, "%s", pw->pw_name);
		     }
		     break;
		 case '#': /* process console or remote? */
		     ast_str_append(&prompt, 0, "%c", ast_opt_remote ? '>' : '#');
		     break;
		 case '%': /* literal % */
		     ast_str_append(&prompt, 0, "%c", '%');
		     break;
		 case '\0': /* % is last character - prevent bug */
		     t--;
		     break;
		 }
	     } else {
		 ast_str_append(&prompt, 0, "%c", *t);
	     }
	     t++;
	 }
	 if (color_used) {
	     /* Force colors back to normal at end */
	     ast_term_color_code(&prompt, 0, 0);
	 }
     } else {
	 ast_str_set(&prompt, 0, "%s%s",
		     remotehostname ? remotehostname : "",
		     CAMELGW_PROMPT);
     }

     return camelgw_str_buffer(prompt);
 }

 static int ast_el_initialize(void)
 {
     HistEvent ev;
     char *editor, *editrc = getenv("EDITRC");

     if (!(editor = getenv("AST_EDITMODE"))) {
	 if (!(editor = getenv("AST_EDITOR"))) {
	     editor = "emacs";
	 }
     }

     if (el != NULL)
	 el_end(el);
     if (el_hist != NULL)
	 history_end(el_hist);

     el = el_init("asterisk", stdin, stdout, stderr);
     el_set(el, EL_PROMPT, cli_prompt);

     el_set(el, EL_EDITMODE, 1);
     el_set(el, EL_EDITOR, editor);
     el_hist = history_init();
     if (!el || !el_hist)
	 return -1;

     /* setup history with 100 entries */
     history(el_hist, &ev, H_SETSIZE, 100);

     el_set(el, EL_HIST, history, el_hist);

     el_set(el, EL_ADDFN, "ed-complete", "Complete argument", cli_complete);
     /* Bind <tab> to command completion */
     el_set(el, EL_BIND, "^I", "ed-complete", NULL);
     /* Bind ? to command completion */
     el_set(el, EL_BIND, "?", "ed-complete", NULL);
     /* Bind ^D to redisplay */
     el_set(el, EL_BIND, "^D", "ed-redisplay", NULL);
     /* Bind Delete to delete char left */
     el_set(el, EL_BIND, "\\e[3~", "ed-delete-next-char", NULL);
     /* Bind Home and End to move to line start and end */
     el_set(el, EL_BIND, "\\e[1~", "ed-move-to-beg", NULL);
     el_set(el, EL_BIND, "\\e[4~", "ed-move-to-end", NULL);
     /* Bind C-left and C-right to move by word (not all terminals) */
     el_set(el, EL_BIND, "\\eOC", "vi-next-word", NULL);
     el_set(el, EL_BIND, "\\eOD", "vi-prev-word", NULL);

     if (editrc) {
	 el_source(el, editrc);
     }

     return 0;
 }



#define MAX_HISTORY_COMMAND_LENGTH 256

 static int ast_el_add_history(const char *buf)
 {
     HistEvent ev;
     char *stripped_buf;

     if (el_hist == NULL || el == NULL) {
	 ast_el_initialize();
     }
     if (strlen(buf) > (MAX_HISTORY_COMMAND_LENGTH - 1)) {
	 return 0;
     }

     stripped_buf = camelgw_strip(camelgw_strdupa(buf));

     /* HISTCONTROL=ignoredups */
     if (!history(el_hist, &ev, H_FIRST) && strcmp(ev.str, stripped_buf) == 0) {
	 return 0;
     }

     return history(el_hist, &ev, H_ENTER, stripped_buf);
 }



 static int ast_el_write_history(const char *filename)
 {
     HistEvent ev;

     if (el_hist == NULL || el == NULL)
	 ast_el_initialize();

     return (history(el_hist, &ev, H_SAVE, filename));
 }

 static int ast_el_read_history(const char *filename)
 {
     HistEvent ev;

     if (el_hist == NULL || el == NULL) {
	 ast_el_initialize();
     }

     return history(el_hist, &ev, H_LOAD, filename);
 }

 static void ast_el_read_default_histfile(void)
 {
     char histfile[80] = "";
     const char *home = getenv("HOME");

     if (!camelgw_strlen_zero(home)) {
	 snprintf(histfile, sizeof(histfile), "%s/.asterisk_history", home);
	 ast_el_read_history(histfile);
     }
 }

 static void ast_el_write_default_histfile(void)
 {
     char histfile[80] = "";
     const char *home = getenv("HOME");

     if (!camelgw_strlen_zero(home)) {
	 snprintf(histfile, sizeof(histfile), "%s/.asterisk_history", home);
	 ast_el_write_history(histfile);
}
}


/* This is the main console CLI command handler.  Run by the main() thread. */
static void consolehandler(const char *s)
{
    printf("%s", term_end());
    fflush(stdout);

    /* Called when readline data is available */
    if (!camelgw_all_zeros(s))
	ast_el_add_history(s);
    /* The real handler for bang */
    if (s[0] == '!') {
	if (s[1])
	    ast_safe_system(s+1);
	else
	    ast_safe_system(getenv("SHELL") ? getenv("SHELL") : "/bin/sh");
    } else
	camelgw_cli_command(STDOUT_FILENO, s);
}


static int remoteconsolehandler(const char *s)
{
    int ret = 0;

    /* Called when readline data is available */
    if (!camelgw_all_zeros(s))
	ast_el_add_history(s);

    while (isspace(*s)) {
	s++;
    }

    /* The real handler for bang */
    if (s[0] == '!') {
	if (s[1])
	    ast_safe_system(s+1);
	else
	    ast_safe_system(getenv("SHELL") ? getenv("SHELL") : "/bin/sh");
	ret = 1;
    } else if ((strncasecmp(s, "quit", 4) == 0 || strncasecmp(s, "exit", 4) == 0) &&
	       (s[4] == '\0' || isspace(s[4]))) {
	quit_handler(0, SHUTDOWN_FAST, 0);
	ret = 1;
    } else if (s[0]) {
	char *shrunk = camelgw_strdupa(s);
	char *cur;
	char *prev;

	/*
	 * Remove duplicate spaces from shrunk for matching purposes.
	 *
	 * shrunk has at least one character in it to start with or we
	 * couldn't get here.
	 */
	for (prev = shrunk, cur = shrunk + 1; *cur; ++cur) {
	    if (*prev == ' ' && *cur == ' ') {
		/* Skip repeated space delimiter. */
		continue;
	    }
	    *++prev = *cur;
	}
	*++prev = '\0';

	if (strncasecmp(shrunk, "core set verbose ", 17) == 0) {
	    /*
	     * We need to still set the rasterisk option_verbose in case we are
	     * talking to an earlier version which doesn't prefilter verbose
	     * levels.  This is really a compromise as we should always take
	     * whatever the server sends.
	     */

	    if (!strncasecmp(shrunk + 17, "off", 3)) {
		ast_verb_console_set(0);
	    } else {
		int verbose_new;
		int atleast;

		atleast = 8;
		if (strncasecmp(shrunk + 17, "atleast ", atleast)) {
		    atleast = 0;
		}

		if (sscanf(shrunk + 17 + atleast, "%30d", &verbose_new) == 1) {
		    if (!atleast || ast_verb_console_get() < verbose_new) {
			ast_verb_console_set(verbose_new);
		    }
		}
	    }
	}
    }

    return ret;
}


 static void ast_remotecontrol(char *data)
 {
     char buf[256] = "";
     int res;
     char *hostname;
     char *cpid;
     char *version;
     int pid;
     char *stringp = NULL;

     char *ebuf;
     int num = 0;

     ast_term_init();
     printf("%s", term_end());
     fflush(stdout);

     //    memset(&sig_flags, 0, sizeof(sig_flags));
     //signal(SIGINT, __remote_quit_handler);
     //signal(SIGTERM, __remote_quit_handler);
     //signal(SIGHUP, __remote_quit_handler);

     if (read(ast_consock, buf, sizeof(buf) - 1) < 0) {
	 //camelgw_log(LOG_ERROR, "read() failed: %s\n", strerror(errno));
	 printf("read error!");
	 return;
     }
     if (data) {
	 char prefix[] = "cli quit after ";
	 char *tmp = camelgw_alloca(strlen(data) + strlen(prefix) + 1);
	 sprintf(tmp, "%s%s", prefix, data);
	 if (write(ast_consock, tmp, strlen(tmp) + 1) < 0) {
	     camelgw_log(LOG_ERROR, "write() failed: %s\n", strerror(errno));
	     //	     if (sig_flags.need_quit || sig_flags.need_quit_handler) {
	     //	 return;
	     //}
	 }
     }
     stringp = buf;
     hostname = strsep(&stringp, "/");
     cpid = strsep(&stringp, "/");
     version = strsep(&stringp, "\n");
     if (!version)
	 version = "<Version Unknown>";
     stringp = hostname;
     strsep(&stringp, ".");
     if (cpid)
	 pid = atoi(cpid);
     else
	 pid = -1;
     if (!data) {
	 send_rasterisk_connect_commands();
     }

     if (ast_opt_exec && data) {  /* hack to print output then exit if asterisk -rx is used */
	 int linefull = 1, prev_linefull = 1, prev_line_verbose = 0;
	 struct pollfd fds;
	 fds.fd = ast_consock;
	 fds.events = POLLIN;
	 fds.revents = 0;

	 while (poll(&fds, 1, 60000) > 0) {
	     char buffer[512] = "", *curline = buffer, *nextline;
	     int not_written = 1;

	     //	     if (sig_flags.need_quit || sig_flags.need_quit_handler) {
	     //	 break;
	     //}

	     if (read(ast_consock, buffer, sizeof(buffer) - 1) <= 0) {
		 break;
	     }

	     do {
		 prev_linefull = linefull;
		 if ((nextline = strchr(curline, '\n'))) {
		     linefull = 1;
		     nextline++;
		 } else {
		     linefull = 0;
		     nextline = strchr(curline, '\0');
		 }

		 /* Skip verbose lines */
		 /* Prev line full? | Line is verbose | Last line verbose? | Print
		  * TRUE            | TRUE*           | TRUE               | FALSE
		  * TRUE            | TRUE*           | FALSE              | FALSE
		  * TRUE            | FALSE*          | TRUE               | TRUE
		  * TRUE            | FALSE*          | FALSE              | TRUE
		  * FALSE           | TRUE            | TRUE*              | FALSE
		  * FALSE           | TRUE            | FALSE*             | TRUE
		  * FALSE           | FALSE           | TRUE*              | FALSE
		  * FALSE           | FALSE           | FALSE*             | TRUE
		  */
		 if ((!prev_linefull && !prev_line_verbose) || (prev_linefull && *curline > 0)) {
		     prev_line_verbose = 0;
		     not_written = 0;
		     if (write(STDOUT_FILENO, curline, nextline - curline) < 0) {
			 camelgw_log(LOG_WARNING, "write() failed: %s\n", strerror(errno));
		     }
		 } else {
		     prev_line_verbose = 1;
		 }
		 curline = nextline;
	     } while (!camelgw_strlen_zero(curline));

	     /* No non-verbose output in 60 seconds. */
	     if (not_written) {
		 break;
	     }
	 }
	 return;
     }

     ast_verbose("Connected to Asterisk %s currently running on %s (pid = %d)\n", version, hostname, pid);
     remotehostname = hostname;
     if (el_hist == NULL || el == NULL)
	 ast_el_initialize();
     ast_el_read_default_histfile();

     el_set(el, EL_GETCFN, ast_el_read_char);

     for (;;) {
	 ebuf = (char *)el_gets(el, &num);

	 //	 if (sig_flags.need_quit || sig_flags.need_quit_handler) {
	 //  break;
	 //}

	 if (!ebuf && write(1, "", 1) < 0)
	     break;

	 if (!camelgw_strlen_zero(ebuf)) {
	     if (ebuf[strlen(ebuf)-1] == '\n')
		 ebuf[strlen(ebuf)-1] = '\0';
	     if (!remoteconsolehandler(ebuf)) {
		 res = write(ast_consock, ebuf, strlen(ebuf) + 1);
		 if (res < 1) {
		     camelgw_log(LOG_WARNING, "Unable to write: %s\n", strerror(errno));
		     break;
		 }
	     }
	 }
     }
     printf("\nDisconnected from Camelgw server\n");
 }


/*
 * Main function for INAP Test Utility (INTU):
 *
 * Returns 0
 */
int main(int argc, char *argv[])
{
    //  struct alarm alrm;	

  int failed_arg;
  int cli_error;
  
  //pid_t pid;
  //int pfd1[2], pfd2[2]; //pipes
  //char *arg[] = { "/opt/sandbox/ora_cli",  0 };

 
  //	    pthread_t thread;
  //        sigset_t set;
  //        int s;
  //        int sig = SIGRTMIN;
  //        struct sigaction sa;
 
  //        union sigval val;
            //val.sival_int = 1;
            /* Block SIGUSR1; other threads created by main()
               will inherit a copy of the signal mask. */
 
  //          sigemptyset(&set);
  //        sigaddset(&set, SIGRTMIN);
  //        s = pthread_sigmask(SIG_BLOCK, &set, NULL);
  //        printf("Create thread\n");
  //        s = pthread_create(&thread, NULL, &thread_func, NULL);

 
  intu_mod_id = DEFAULT_MODULE_ID;
  inap_mod_id = DEFAULT_INAP_MODULE_ID;
  num_dialogues = MAX_NUM_DIALOGUES;
  base_dialogue_id = DEFAULT_BASE_DIALOGUE_ID;
  intu_options = DEFAULT_OPTIONS;

  if ((cli_error = read_command_line_params(argc, argv, &failed_arg)) != 0)
  {
    switch (cli_error)
    {
      case COMMAND_LINE_UNRECON_OPTION :
        fprintf(stderr, "intu() : Unrecognised option : %s\n", argv[failed_arg]);
        show_syntax();
        break;

      case COMMAND_LINE_RANGE_ERR :
	fprintf(stderr, "intu() : Parameter range error : %s\n", argv[failed_arg]);
        show_syntax();
        break;

    case COMMAND_LINE_REMOTE_CONNECTION :
	fprintf(stderr, "intu() : Going to connect to camelgw process...... : \n");
	camelgw_tryconnect();

	ast_term_init();
	printf("%s", term_end());
	fflush(stdout);

	//print_intro_message(runuser, rungroup);
	printf("%s", term_quit());
	ast_remotecontrol(NULL);
	//	quit_handler(0, SHUTDOWN_FAST, 0);
	//exit(0);





	break;

      default :
        break;
    }
    exit(0);
  }

  //pipe(pfd1);
  //pipe(pfd2);



  /*need to fork to run ora_cli as standalone process */

  // switch( pid=fork() ) {
  //case -1:
  //  perror("fork"); /* произошла ошибка */
  //  exit(1); /*выход из родительского процесса*/
  //  break;
  //case 0:
      // printf(" CHILD: Это процесс-потомок!\n");
      //printf(" CHILD: Мой PID -- %d\n", getpid());
      //printf(" CHILD: PID моего родителя -- %d\n", getppid());
  // execv(arg[0],  arg);
  //	printf("I will never be called\n");
	//}
     
	//scanf(" %d");
	//printf(" CHILD: Выход!\n");
	//exit(rv);
  //	break;
  //default:
  //  break;
  // }

  // wait_child();


  //switched off - testing, moved to libprepaid.so 
 
  // pthread_t pthread;
  
//prepaid_sessions_list_init();
  // pthread_create(&pthread, NULL, (void *)prepaid_cdr_logger, &prepaid_cdrs);

  read_lua_config();
  camelgw_curl_init();
  
  readConfigFile();
  bootstrap_service_libs();

  //camelgw_makesocket();
  //camelgw_builtins_init(); 

printf("i am here!!! begore intu_ent!\n");

  intu_ent();

  return 0;
}


/*
 * show_syntax()
 */
static void show_syntax()
{
  fprintf(stderr,
	"Syntax: intu [-m -i -n -b -o]\n");
  fprintf(stderr,
	"  -m  : intu module_id (default=0x%02x)\n", DEFAULT_MODULE_ID);
  fprintf(stderr,
	"  -i  : inap module_id (default=0x%02x)\n", DEFAULT_INAP_MODULE_ID);
  fprintf(stderr,
	"  -n  : number of dialogues (default=0x%04x)\n", MAX_NUM_DIALOGUES);
  fprintf(stderr,
	"  -b  : base dialogue ID (default=0x%04x)\n", DEFAULT_BASE_DIALOGUE_ID);
  fprintf(stderr,
	"  -o  : run-time options (default 0x%04x)\n", DEFAULT_OPTIONS);
  fprintf(stderr,
	"Example: intu -m0x3d -i0x35 -n0x200 -b0x0 -o0x013f\n");
}

/*
 * Read in command line options a set the system variables accordingly.
 *
 * Returns 0 on success; on error returns non-zero and
 * writes the parameter index which caused the failure
 * to the variable arg_index.
 */
static int read_command_line_params(argc, argv, arg_index)
  int argc;             /* Number of arguments */
  char *argv[];         /* Array of argument pointers */
  int *arg_index;       /* Used to return parameter index on error */
{
  int error;
  int i;

  for (i=1; i < argc; i++)
  {
    if ((error = read_option(argv[i])) != 0)
    {
      *arg_index = i;
      return(error);
    }
  }
  return 0;
}

/*
 * Read a command line parameter and check syntax.
 *
 * Returns 0 on success or error code on failure.
 */
static int read_option(arg)
  char *arg;            /* Pointer to the parameter */
{
  u32 temp_u32;

  if (arg[0] != '-')
    return(COMMAND_LINE_UNRECON_OPTION);

  switch (arg[1])
  {
    case 'h' :
    case 'H' :
    case '?' :
    case 'v' :
      show_syntax();
      return(COMMAND_LINE_EXIT_REQ);

    case 'm' :
      if (strtou32(&temp_u32, &arg[2]) != 0)
        return(COMMAND_LINE_RANGE_ERR);
      intu_mod_id = (u8)temp_u32;
      break;

    case 'i' :
      if (strtou32(&temp_u32, &arg[2]) != 0)
        return(COMMAND_LINE_RANGE_ERR);
      inap_mod_id = (u8)temp_u32;
      break;

    case 'n' :
      if (strtou32(&temp_u32, &arg[2]) != 0)
        return(COMMAND_LINE_RANGE_ERR);
      num_dialogues = (u16) temp_u32;
      break;

    case 'b' :
      if (strtou32(&temp_u32, &arg[2]) != 0)
        return(COMMAND_LINE_RANGE_ERR);
      base_dialogue_id = (u16) temp_u32;
      break;

    case 'o' :
      if (strtou32(&temp_u32, &arg[2]) != 0)
        return(COMMAND_LINE_RANGE_ERR);
      intu_options = (u16)temp_u32;
      break;

  case 'r':
      return COMMAND_LINE_REMOTE_CONNECTION;

  }
  return 0;
}
    
