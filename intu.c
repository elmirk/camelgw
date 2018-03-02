/*
camel gateway engine application
version 1.1
02022017
working!

MODIFIED   (MM/DD/YY)  
Elvina     21/03/2017 - В get запросы при режиме SIP передаются вх и исх номер
Elvina     29/03/2017 - Добывлен сценарий на коды возврата при обращение к БД
*/

#include "intu_def.h"
#include "sys/ipc.h"
#include "time.h"
#include "helper.h"
#include "globals.h"

#include "camelgw_handle_idp.h"
#include "camelgw_handle_erb.h"
#include "camelgw_conf.h"
//#include "camelgw_handle_acr.c"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

//#define NATIONAL_NUMBER 3
//#define INTERNATIONAL_NUMBER 4

//#define REGIME  1 
#define IP      2
#define IDP_RES 3


#define DEBUG	//working in DEBUG mode, with a lot of printfs and other tricks


struct alarm {
                int severity;
                char *descriptor;
         };

//struct alarm alrm; 
/* modes for main processing logic by INTU module */

//#define MODE_CONTINUE_ONLY 1 
//#define MODE_PREPAID 2
//#define MODE_SIP_TERMINATION 3

/*
* INTU
* ====
*
* INTU operates as a state machine, with each state representing a point durinmg
* an INAP dialogue. A separate Dialogue Control Block (DLG_CB) exists for each
* dialogue identifier in the supported range.
*
* The following Message Sequence Chart shows the messages that occur and the
* expected flow through the state machine for a single dialogue. The messages
* between INTU and the local INAP task.
*
*                INTU                             INAP
*                =====                           =====
*     (Note 1)     |                               |
*                IDLE                              |
*                  |        DLG-IND (OPEN)         |
*     (Note 2)     | <---------------------------- |
*                  |                               |
*                OPEN                              |
*                  |   SRV-IND (INVOKE:InitialDP)  |
*     (Note 3)     | <---------------------------- |

*                  |                               |

*               PENDING                            |
*               DELIMIT                            |
*                  |       DLG-IND (DELIMIT)       |
*                  | <---------------------------- |
*     (Note 4)     |                               |
*                  |      DLG-REQ (OPEN-RSP)       |
*                  | ----------------------------> |
*                  |                               |
*                  |    SRV-REQ (INVOKE:Connect)   |
*                  | ----------------------------> |
*                  |                               |
*                  |        DLG-REQ (DELIMIT)      |
*                  | ----------------------------> |
*                  |                               |
*                  |        DLG-REQ (CLOSE)        |
*                  | ----------------------------> |
*                  |      (pre-arranged end)       |
*                  |                               |
*               CLOSING                            |

*                  |        DLG-IND (CLOSE)        |
*     (Note 5)     | <---------------------------- |
*                  |                               |
*                IDLE                              |
*                  |                               |
*
* Note 1: Dialogue is idle
*
* Note 2: Dialogue OPEN indication causes the dialogue to be opened
*
* Note 3: The service key is checked if it matches the information needed
*         to process the service is stored and prepared in the dialogue control
*         block (DLG_CB) waiting for the delimit.
*
* Note 4: The delimit triggers the processing of the prepared service logic,
*         In this case a simple number translation from one known number to
*         another. If the called party address isn't recognised a ReleaseCall
*         would be sent. INTU replies with an OPEN-RSP, INVOKE-Connect,
*         DELIMIT, CLOSE(pre-arranged end).
*
* Note 5: The dialogue is waiting for INAP to tell it to close the dialogue.
*         When the CLOSE comes the dialogue control block is reset.
*/

/*
* 
* unsigned char   u8        Exactly one byte (8 bits);
* unsigned int short  u16;  16bit
* unsigned int long   u32;  Exactly 4 bytes (32 bits) 
* 
*/

//typedef struct msgbuf {
//long msg_type;
//long int msg_time; // время в сыром линукс формате
//int msg_dlg_id;  //current dialog id
//int msg_serv_mode;//indicator of service mode     
//u16 msg_operation;
//u16 msg_databuf_offset;
//u8 msg_databuf[0];
//} LOG_MSG;


//void sig_handler (int signum, siginfo_t *siginfo, void *ucontext);// {
static void* thread_func(void *ptr);// {
static void* thread_func2(void *p);// {

int INTU_handle_cpt_list(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, IN_CPT *buffer_cpt_ptr);
int INTU_process_invokes(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h);
/* 
* global enum for consttrcting SERVICE flag parameter -от этого параметра 
* зависит вся логика шлюза, в зависимости от настроек lua, 
* вход или исходящий вызов, домашний или гостевой VLR
*/
//enum OPTIONS_FLAGS {
//	DP2 =     0x01, //DP2 is an int type - MO call
//	DP12 =    0x00, //MT call
//	BYPASS =  0x00, //bypass mode, all services switched off, just continue to MSC
//	SIP_ON =  0x04, //SIP service ON
//	SIP_OFF = 0x08, //SIP service OFF
//	HOME_VLR = 0x00, // user located in Home VLR
//	GUEST_VLR =0x40,// user located in GUEST VLR
//};

//global array for max size of LOG_MSG structure
//4112 = 4096(max number of databuf elements + length of another elements of mLOG_MSG
unsigned char memory_buffer[256];

u8 example_freephone_num[]    = {8,9,0,2,7,1,1,1,2,8,7}; 
u8 example_dest_routing_num[] = {8,9,1,7,8,8,5,0,3,0,7}; 
/*
* Cause normal_unspecified
*/
//u8 example_release_cause[]= {0,0x80+INTU_cause_value_normal_unspecified};

/*
* Example service key. This is the only service key recognised by this example
* program. If the InitialDP does not use this service key INTU will respond
* with U_ERROR.
*/
static u8 example_srv_key[] = {0x1};
static u8 example_srv_key_2[] = {0x2};

int socket_d, socket_d_vpn, socket_d_m2m;
/*
* Module variables, updated by command line options in intu_main
*/
u8  intu_mod_id;      // The task id of this module 
u8  inap_mod_id;      // The task id of the INAP binary module
u16 num_dialogues;    // The number of dialogues to support
u32 base_dialogue_id; // The base number of the supported dialogue IDs 
u16 intu_options;     // Defines which tracing options are configured 

/*
 * Module variables
 */
u32 last_dlg_id;      /* Last Dialog ID of configured range */


/*
* Array of dialogue control blocks for dialogue ids
* from (base_dialogue_id) to (base_dialogue_id + NUM_DIALOGUES - 1).
*/
DLG_CB dlg_data[MAX_NUM_DIALOGUES];


//u8 example_release_cause[2]= {0,0x80+INTU_cause_value_normal_unspecified};

//u8 example_release_cause2[2] = {0x80, 0x80 + INTU_cause_call_bearer_capability_not_authorized}; 

/*
* Variables to log dialogue statistics
*/
static u16 active_dialogues = 0;
static u32 completed_dialogues = 0;
static u32 successful_dialogues = 0;
static u32 timeout_dialogues;

struct My_Data {
	    u16 Dead_Dialogue;
pthread_spinlock_t spin;
	};


char *socket_path = "/var/run/camelgw/socket";
const char *socket_path_vpn = "/var/run/camelgw/vpn_driver";
char *socket_path_m2m = "/var/run/camelgw/m2m_driver";

pthread_spinlock_t spin;
  u16 Global;


volatile sig_atomic_t services_conf_reread_flag;


void
sighup(int signo)
{

    services_conf_reread_flag = 1; /*need to reread services configuration file */

}

  /*
*******************************************************************************
*                                                                             *
* intu_ent: Main program loop                                                 *
*                            
* intu_ent() called from the main function in intumain.c                      *
*******************************************************************************
*/
int intu_ent()
{
	HDR	*h;          /* Received message */
	DLG_CB	*dlg;        /* The control block for the dlg id of the rec msg*/

	u16	i;           /* loop counter */
	u16	min_rev;     /* INAP API major revision number */
	u16	maj_rev;     /* INAP API major revision number */

	char	*ident_text; /* INAP API name in text */

	//	int socket_d;
	MSG *msg_buff_ptr;
	     u8 *pptr;
	int ret;
	int status;
	int fd; //descriptor for connecting with Erlang node

	   pthread_t thread;
	   //       timer_t timer;
	 struct sigaction sa;
	struct sockaddr_un addr;
	//struct sigevent se = {0};
	//		     struct itimerspec ts = {0};
		     sigset_t new_set, pending_set, old_set;

		     //	struct alarm alrm; 
	//example_release_cause[2]= {0,0x80+INTU_cause_value_normal_unspecified};

		     
/*
		      * Calculate last valid dialog ID
		      */
		     last_dlg_id = base_dialogue_id + num_dialogues - 1;





	if (num_dialogues > MAX_NUM_DIALOGUES)
	{
		fprintf(stderr, "intu() : Not enough dialogue resources available\n");
		fprintf(stderr, "0x%04x available, 0x%04x requested\n", MAX_NUM_DIALOGUES, num_dialogues);
		return 0;
	}

	/*
	 * If not using Extended Dialog IDs, check that all dialog DIDs required only 16 bits
	 */
	if ((intu_options & INTU_OPT_EXT_DLG) == 0)
	    {
		if ((last_dlg_id > MAX_NON_EXT_DID) && (num_dialogues > 0))
		    {
			fprintf(stderr, "intu() : Dialogue range requires use of Extended Dialog IDs option (0x%04x)\n", INTU_OPT_EXT_DLG);
			fprintf(stderr, "         last dialog in configured range is 0x%08lx\n", last_dlg_id);
			return 0;
		    }
	    }


	/*
	* Print banner so we know what's running:
	*/
	printf("CAMEL Gateway application (C) based on Dialogic intu.c Version = 1.2 \n");
	printf("=======================================================================================\n\n");
	if (IN_version(&maj_rev, &min_rev, &ident_text) == IN_SUCCESS);
	printf("%s Version %i.%i\n",ident_text, maj_rev, min_rev);
	printf("INTU module ID - 0x%02x \nINAP module ID - 0x%02x\n", intu_mod_id, inap_mod_id);
	printf("Number of dialogues - 0x%04x (%i)\nBase dialogue ID - 0x%04x\n", num_dialogues, num_dialogues, base_dialogue_id);
	printf("Options set - 0x%04x \n\n", intu_options);

	/*
	* Initialise the per-dialogue resources:
	*/
	for (i=0; i<MAX_NUM_DIALOGUES; i++)
	{
		dlg = &dlg_data[i];
		dlg->state = INTU_STATE_IDLE;
		dlg->reply_prepared = INTU_REPLY_NONE;
		dlg->cpt_list_head = NULL; // для каждого диалога список компонент изначально состоит как бы из одного узла, указатель на голову = null 
		dlg->cursor = 0;
		//may be need this initialising by zero memset(&(dlg->call_details), 0, sizeof(struct calldetails));
	}

	/* timer environment settings */
	/*timer used when no any message received for open dialog during TIMEOUT value */
	//           se.sigev_notify = SIGEV_SIGNAL;
	// se.sigev_signo = SIGRTMIN;
	   //	   se.sigev_value.sival_int = 15;//&timer;
		     //

		     /* timer create */

    /*		     ret = timer_create(CLOCK_REALTIME, &se, &timer);

		     if (ret)
		     perror("timer create"); */

		     

		     //timer_settime(timer, 0, &ts, NULL);

	   //struct sigaction sa;
	     //sigset_t set, pending_set, old_set;
	     //		     timer_t timer;
	     //	     int ret,i;
	   //sa.sa_handler = sig_handler;
		     // sigemptyset (&new_action.sa_mask);
	   //	     sa.sa_flags = SA_SIGINFO;
	   //	     sigaction (SIGRTMIN, &sa, NULL);
		     //			 sigaction (SIGTERM, &new_action, NULL);
  

	//socket_d for ora_cli moved to prepaid lib
	
    /* 	/\*prepare UNIX DOMAIN SOCKET DESCRIPTOR *\/ */
    /* 	/\*used to connect to oracle_cli process*\/ */

    /* 	    if ( (socket_d = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) { */
    /* 	perror("socket error"); */
    /* 	exit(-1); */
    /* } */
    /* memset(&addr, 0, sizeof(addr)); */
    /* addr.sun_family = AF_UNIX; */
    /* if (*socket_path == '\0') { */
    /* 	*addr.sun_path = '\0'; */
    /* 	strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2); */
    /* } else { */
    /* 	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1); */
    /* } */

    /* if (connect(socket_d, (struct sockaddr*)&addr, sizeof(addr)) == -1) { */
    /* 	perror("unix domain socket connect error"); */
    /* 	//	exit(-1); */
    /* 	//TODO - set global flag */

    /* } */

    /***** connect to vpn_driver ********/

	    if ( (socket_d_vpn = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	perror("socket for vpn driver  error");
	exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (*socket_path_vpn == '\0') {
	*addr.sun_path = '\0';
	strncpy(addr.sun_path+1, socket_path_vpn+1, sizeof(addr.sun_path)-2);
    } else {
	strncpy(addr.sun_path, socket_path_vpn, sizeof(addr.sun_path)-1);
    }

    if (connect(socket_d_vpn, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
	perror("unix domain socket connect with vpn driver error");
	//	exit(-1);


    }

    /***** connect to m2m_driver ********/
    //work in progress 

     	    if ( (socket_d_m2m = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
     	perror("socket for m2m driver  error");
     	exit(-1);
     } 
     memset(&addr, 0, sizeof(addr));
     addr.sun_family = AF_UNIX;
     if (*socket_path_m2m == '\0')
	 {
	     *addr.sun_path = '\0';
	     strncpy(addr.sun_path+1, socket_path_m2m+1, sizeof(addr.sun_path)-2);
	 }
     else
	 { 
     	strncpy(addr.sun_path, socket_path_m2m, sizeof(addr.sun_path)-1);
     } 

     if (connect(socket_d_m2m, (struct sockaddr*)&addr, sizeof(addr)) == -1) { 
     	perror("unix domain socket connect with m2m driver error"); 
	//    	exit(-1); 
     

     } 




/*******************************************************************************************
	 * MAIN PROGRAMM ENDLESS LOOP
****************************************************************************************************/

/* all things about signals sould be moved inti intumain.c !!!!!!!

   sigemptyset(&new_set);                                                             
     //sigaddset(&set, SIGUSR1); 
     //sigaddset(&set, SIGUSR2);
     sigaddset(&new_set, SIGRTMIN);
     //     sigprocmask(SIG_SETMASK, &new_set, &old_set);
     //	finished = 0;

	//sigprocmask(SIG_SETMASK, &new_set, &old_set);

     pthread_sigmask(SIG_BLOCK, &new_set, &old_set); //block SIGRTMIN signal for main thread

     //  u8 *pptr;
     unsigned char value = 0x35;

     //uc_ptr = (unsigned char*) calloc(80, 1);

/**************************** configure SCCP module parameters *******************************************************/

msg_buff_ptr = getm(SCP_MSG_CONFIG, 0, 0, SCPML_CONFIG_MAX );

     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = SCP_TASK_ID;
     msg_buff_ptr->hdr.id = 0;
     msg_buff_ptr->hdr.rsp_req = 0x2000;  //need confirmation send to user module
     
     pptr = get_param(msg_buff_ptr);

     memset(pptr, 0, msg_buff_ptr->len);

    rpackbytes(pptr, SCPMO_CONFIG_cnf_ver ,   1,    SCPMS_CONFIG_cnf_ver );
    rpackbytes(pptr, SCPMO_CONFIG_sio,  0xc3  , SCPMS_CONFIG_sio   ); //sio = c3
    rpackbytes(pptr, SCPMO_CONFIG_options, 0x8006, SCPMS_CONFIG_options); //TODO! - should check this value
    rpackbytes(pptr, SCPMO_CONFIG_module_id, 0x33,   SCPMS_CONFIG_module_id); //SCCP module ID
    rpackbytes(pptr, SCPMO_CONFIG_mtp_id,   0x22,  SCPMS_CONFIG_mtp_id);
    rpackbytes(pptr, SCPMO_CONFIG_mngt_id,   0xef, SCPMS_CONFIG_mngt_id);
    rpackbytes(pptr, SCPMO_CONFIG_maint_id,   0xef, SCPMS_CONFIG_maint_id);
    rpackbytes(pptr, SCPMO_CONFIG_point_code,  14100, SCPMS_CONFIG_point_code); //SCP Point Code
    rpackbytes(pptr, SCPMO_CONFIG_max_sif, 272, SCPMS_CONFIG_max_sif);
    rpackbytes(pptr, SCPMO_CONFIG_sccp_instance, 0  , SCPMS_CONFIG_sccp_instance);
    rpackbytes(pptr, SCPMO_CONFIG_smb_id, 0xef, SCPMS_CONFIG_smb_id);
    rpackbytes(pptr, SCPMO_CONFIG_smb_flags, 0x1c,    SCPMS_CONFIG_smb_flags);
    rpackbytes(pptr, SCPMO_CONFIG_num_uc, 0, SCPMS_CONFIG_num_uc);
    rpackbytes(pptr, SCPMO_CONFIG_uc_onset  ,0 , SCPMS_CONFIG_uc_onset);
    rpackbytes(pptr, SCPMO_CONFIG_uc_abmt  ,0 , SCPMS_CONFIG_uc_abmt);
    rpackbytes(pptr, SCPMO_CONFIG_num_ic ,0 , SCPMS_CONFIG_num_ic);
    rpackbytes(pptr, SCPMO_CONFIG_ic_onset ,0 ,SCPMS_CONFIG_ic_onset);
    rpackbytes(pptr, SCPMO_CONFIG_ic_abmt, 0 ,  SCPMS_CONFIG_ic_abmt);
    rpackbytes(pptr, SCPMO_CONFIG_num_data,0 , SCPMS_CONFIG_num_data);
    rpackbytes(pptr, SCPMO_CONFIG_data_onset,0 , SCPMS_CONFIG_data_onset);
    rpackbytes(pptr, SCPMO_CONFIG_data_abmt,0 , SCPMS_CONFIG_data_abmt);
    rpackbytes(pptr, SCPMO_CONFIG_num_edata,0 , SCPMS_CONFIG_num_edata);
    rpackbytes(pptr, SCPMO_CONFIG_edata_onset,0 , SCPMS_CONFIG_edata_onset);
    rpackbytes(pptr, SCPMO_CONFIG_edata_abmt,0 , SCPMS_CONFIG_edata_abmt);
    rpackbytes(pptr, SCPMO_CONFIG_ext_options, SCPXF_DEL_CGPC, SCPMS_CONFIG_ext_options); //remove PointCode from SCCP Calling GT from INTU messages
    rpackbytes(pptr, SCPMO_CONFIG_base_id  , 0, SCPMS_CONFIG_base_id);
    rpackbytes(pptr, SCPMO_CONFIG_top_id   , 0, SCPMS_CONFIG_top_id);
    rpackbytes(pptr, SCPMO_CONFIG_min_id  , 0, SCPMS_CONFIG_min_id);
    rpackbytes(pptr, SCPMO_CONFIG_max_id  , 0, SCPMS_CONFIG_max_id);
    rpackbytes(pptr, SCPMO_CONFIG_isup_id , 0 , SCPMS_CONFIG_isup_id);
    rpackbytes(pptr, SCPMO_CONFIG_nc      , 0 , SCPMS_CONFIG_nc);
    rpackbytes(pptr, SCPMO_CONFIG_hop_counter, 0, SCPMS_CONFIG_hop_counter );
    rpackbytes(pptr, SCPMO_CONFIG_error_offset, 0, SCPMS_CONFIG_error_offset);
    rpackbytes(pptr, SCPMO_CONFIG_trace_id, 0xef, SCPMS_CONFIG_trace_id);
    rpackbytes(pptr, SCPMO_CONFIG_gtt_separator, 0, SCPMS_CONFIG_gtt_separator);
    rpackbytes(pptr, SCPMO_CONFIG_ext2_options, 0x1, SCPMS_CONFIG_ext2_options);

     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
}

/*******************************end of SCCP module parameters configuration ******************************************/

/**************************** configure TCAP module parameters *******************************************************/
     msg_buff_ptr = getm(TCP_MSG_CONFIG, 0, 0, TCPML_CONFIG_V1);

     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = 0x14;
     msg_buff_ptr->hdr.rsp_req = 0x2000;
     
     pptr = get_param(msg_buff_ptr);

     memset(pptr, 0, msg_buff_ptr->len);

     rpackbytes(pptr, TCPMO_CONFIG_V1_cnf_ver,   1,    TCPMS_CONFIG_V1_cnf_ver);
     rpackbytes(pptr, TCPMO_CONFIG_V1_module_id, 0,    TCPMS_CONFIG_V1_module_id );
     rpackbytes(pptr, TCPMO_CONFIG_V1_user_id,   0x35, TCPMS_CONFIG_V1_user_id);
     rpackbytes(pptr, TCPMO_CONFIG_V1_nsap_id,   0x33, TCPMS_CONFIG_V1_nsap_id);
     //old commerc     rpackbytes(pptr, TCPMO_CONFIG_V1_mngt_id,   0xef, TCPMS_CONFIG_V1_mngt_id); //try to use new mngt id module
     //rpackbytes(pptr, TCPMO_CONFIG_V1_mngt_id,   0xed, TCPMS_CONFIG_V1_mngt_id);
     rpackbytes(pptr, TCPMO_CONFIG_V1_mngt_id,   0x3d, TCPMS_CONFIG_V1_mngt_id); //try to use new mngt id module
 //rpackbytes(pptr, TCPMO_CONFIG_V1_maint_id , 0xef, TCPMS_CONFIG_V1_maint_id);
     rpackbytes(pptr, TCPMO_CONFIG_V1_maint_id , 0x3d, TCPMS_CONFIG_V1_maint_id);
     //old commerc     rpackbytes(pptr, TCPMO_CONFIG_V1_trace_id , 0xed, TCPMS_CONFIG_V1_trace_id); //module to catch trace messages
 rpackbytes(pptr, TCPMO_CONFIG_V1_trace_id , 0x3d, TCPMS_CONFIG_V1_trace_id); //module to catch trace messages

     rpackbytes(pptr, TCPMO_CONFIG_V1_tcap_instance, 0x0, TCPMS_CONFIG_V1_tcap_instance );
     rpackbytes(pptr, TCPMO_CONFIG_V1_tid_ninst , 0x0, TCPMS_CONFIG_V1_tid_ninst);
     rpackbytes(pptr, TCPMO_CONFIG_V1_tid_ndref, 0x10, TCPMS_CONFIG_V1_tid_ndref);
     rpackbytes(pptr, TCPMO_CONFIG_V1_tid_nseq  , 0x0c, TCPMS_CONFIG_V1_tid_nseq);
     rpackbytes(pptr, TCPMO_CONFIG_V1_addr_format , 0x0, TCPMS_CONFIG_V1_addr_format );
     rpackbytes(pptr, TCPMO_CONFIG_V1_reserved_1 , 0, TCPMS_CONFIG_V1_reserved_1);
     //     rpackbytes(pptr, TCPMO_CONFIG_V1_flags, TCPF_DLG_TIM_ABORT, TCPMS_CONFIG_V1_flags);
     //old commerc rpackbytes(pptr, TCPMO_CONFIG_V1_flags, 0, TCPMS_CONFIG_V1_flags);
     rpackbytes(pptr, TCPMO_CONFIG_V1_flags, TCPF_TR_DISC, TCPMS_CONFIG_V1_flags); //trace discarded primitives to mngmn module
     rpackbytes(pptr, TCPMO_CONFIG_V1_ext_flags, 0x0, TCPMS_CONFIG_V1_ext_flags);
     rpackbytes(pptr, TCPMO_CONFIG_V1_max_data , 0xff, TCPMS_CONFIG_V1_max_data);
     rpackbytes(pptr, TCPMO_CONFIG_V1_dlg_hunt, 0x0, TCPMS_CONFIG_V1_dlg_hunt);
     rpackbytes(pptr, TCPMO_CONFIG_V1_def_dlg_idle_tmo , 0x012c, TCPMS_CONFIG_V1_def_dlg_idle_tmo); //TCAP dialog timeout timer = 5 minutes
     rpackbytes(pptr, TCPMO_CONFIG_V1_reserved_2, 0x0000, TCPMS_CONFIG_V1_reserved_2);
     //rpackbytes(pptr, TCPMO_CONFIG_V1_num_components , 0x100, TCPMS_CONFIG_V1_num_components );
rpackbytes(pptr, TCPMO_CONFIG_V1_num_components , 0x0014, TCPMS_CONFIG_V1_num_components );
//rpackbytes(pptr, TCPMO_CONFIG_V1_num_invokes , 0x0100, TCPMS_CONFIG_V1_num_invokes);
     rpackbytes(pptr, TCPMO_CONFIG_V1_num_invokes , 0x0014, TCPMS_CONFIG_V1_num_invokes);
     //    rpackbytes(pptr, TCPMO_CONFIG_V1_nog_dialogues, 0x800, TCPMS_CONFIG_V1_nog_dialogues);
     rpackbytes(pptr, TCPMO_CONFIG_V1_nog_dialogues , 0x0014, TCPMS_CONFIG_V1_nog_dialogues );
//rpackbytes(pptr, TCPMO_CONFIG_V1_nic_dialogues, 0x800, TCPMS_CONFIG_V1_nic_dialogues);
  rpackbytes(pptr, TCPMO_CONFIG_V1_nic_dialogues, 0x000a, TCPMS_CONFIG_V1_nic_dialogues);
   rpackbytes(pptr, TCPMO_CONFIG_V1_base_ogdlg_id , 0x0000, TCPMS_CONFIG_V1_base_ogdlg_id );
     rpackbytes(pptr, TCPMO_CONFIG_V1_base_icdlg_id , 0x8000, TCPMS_CONFIG_V1_base_icdlg_id );


     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
}
     /*********************TCAP trace mask config******************************************************************************/
	       msg_buff_ptr = getm(TCP_MSG_TRACE_MASK, 0, 0, TCPML_TRACE_MASK);

	       msg_buff_ptr->hdr.src = 0x3d;
	       msg_buff_ptr->hdr.dst = TCP_TASK_ID;
	       msg_buff_ptr->hdr.id = 0;
	       //msg_buff_ptr->hdr.rsp_req = 0x2000; //rsp_req for confirmation receiving
	       pptr = get_param(msg_buff_ptr);

	      memset(pptr, 0, msg_buff_ptr->len);
     //TCPMO_CONFIG_V1_cnf_ver
	      rpackbytes(pptr, TCPMO_TRACE_MASK_op_evt_mask,   0, TCPMS_TRACE_MASK_op_evt_mask);
	     rpackbytes(pptr, TCPMO_TRACE_MASK_ip_evt_mask, TCPIEM_UDT_IND, TCPMS_TRACE_MASK_ip_evt_mask);
	     rpackbytes(pptr, TCPMO_TRACE_MASK_mng_evt_mask, 0, TCPMO_TRACE_MASK_mng_evt_mask);
     
	   if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	   {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	   printf("message send error!");
	   relm(&msg_buff_ptr->hdr);
	   }


     /***************************************************************************************************/
	       msg_buff_ptr = getm(TCP_MSG_S_TCU_ID, 0, 0, 3);

	       msg_buff_ptr->hdr.src = 0x3d;
	       msg_buff_ptr->hdr.dst = TCP_TASK_ID;
	       msg_buff_ptr->hdr.id = 0x92;
	       msg_buff_ptr->hdr.rsp_req = 0x2000; //rsp_req for confirmation receiving
	       pptr = get_param(msg_buff_ptr);

	      memset(pptr, 0, msg_buff_ptr->len);
     //TCPMO_CONFIG_V1_cnf_ver
	      rpackbytes(pptr, 0,   0x35,    1);
	     rpackbytes(pptr, 1, 0,    1 );
     
	   if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	   {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	   printf("message send error!");
	   relm(&msg_buff_ptr->hdr);
	   }

/**************************** configure INAP module parameters *******************************************************/

	   msg_buff_ptr = getm(INAP_MSG_CONFIG, 0, 0, INAPML_CONFIG_V1);

     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = 0x35;
    msg_buff_ptr->hdr.rsp_req = 0x2000;
     //msg_tcap->hdr.id = 3;
     pptr = get_param(msg_buff_ptr);

     memset(pptr, 0, msg_buff_ptr->len);
  
     rpackbytes(pptr, INAPMO_CONFIG_V1_user_id,   0x3d,    INAPMS_CONFIG_V1_user_id  );
     rpackbytes(pptr, INAPMO_CONFIG_V1_TCAP_id,  0x14  ,   INAPMS_CONFIG_V1_TCAP_id   );
     rpackbytes(pptr, INAPMO_CONFIG_V1_mngt_id,   0xef,   INAPMS_CONFIG_V1_mngt_id  );
     rpackbytes(pptr, INAPMO_CONFIG_V1_maint_id,   0xef,  INAPMS_CONFIG_V1_maint_id   );
     rpackbytes(pptr, INAPMO_CONFIG_V1_trace_id,  0xef ,   INAPMS_CONFIG_V1_trace_id  );
     rpackbytes(pptr, INAPMO_CONFIG_V1_version,   1,    INAPMS_CONFIG_V1_version   );
     rpackbytes(pptr, INAPMO_CONFIG_V1_base_usr_ogdlg_id,   0x0000,   INAPMS_CONFIG_V1_base_usr_ogdlg_id );
     rpackbytes(pptr, INAPMO_CONFIG_V1_base_usr_icdlg_id,   0x8000,   INAPMS_CONFIG_V1_base_usr_icdlg_id  );
     rpackbytes(pptr, INAPMO_CONFIG_V1_base_tc_ogdlg_id,   0x0000,    INAPMS_CONFIG_V1_base_tc_ogdlg_id );
     rpackbytes(pptr, INAPMO_CONFIG_V1_base_tc_icdlg_id,   0x8000,   INAPMS_CONFIG_V1_base_tc_icdlg_id  );

     //     rpackbytes(pptr, INAPMO_CONFIG_V1_nog_dialogues,  0x0800,    INAPMS_CONFIG_V1_nog_dialogues );
     //rpackbytes(pptr, INAPMO_CONFIG_V1_nic_dialogues,   0x0800,    INAPMS_CONFIG_V1_nic_dialogues ); //reserved octets set to 0 all
     //rpackbytes(pptr, INAPMO_CONFIG_V1_num_invokes,   0x0100,    INAPMS_CONFIG_V1_num_invokes  ); //reserved octets set to 0 all


     rpackbytes(pptr, INAPMO_CONFIG_V1_nog_dialogues,  0x000a,    INAPMS_CONFIG_V1_nog_dialogues );
     rpackbytes(pptr, INAPMO_CONFIG_V1_nic_dialogues,   0x000a,    INAPMS_CONFIG_V1_nic_dialogues ); //reserved octets set to 0 all
     rpackbytes(pptr, INAPMO_CONFIG_V1_num_invokes,   0x0100,    INAPMS_CONFIG_V1_num_invokes  ); //reserved octets set to 0 all



     rpackbytes(pptr, INAPMO_CONFIG_V1_options,   INAPF_TRNS_AC,    INAPMS_CONFIG_V1_options ); //transparent AC
     rpackbytes(pptr, INAPMO_CONFIG_V1_error_offset,   0,   INAPMS_CONFIG_V1_error_offset ); //reserved octets set to 0 all

     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
}
     /*********************************************************************************************/


/**************************** configure SCCP GT translation ******************************************************/

	  //remote sccp gt
     //msg_tcap = getm(SCP_MSG_GTT_ADD, 0, 0, 30+1 );


     // unsigned char SCP_SCCP_GT_address_MSC[] = {0x08, 0x0b, 0x12, 0x92, 0x00, 0x11, 0x04, 0x97, 0x05, 0x66, 0x15, 0x20, 0x01};
     // unsigned char SCP_SCCP_destination_MSC[] = {0x09, 0x0c, 0x11, 0x3c, 0x37, 0x00, 0x11, 0x04, 0x97, 0x05, 0x66, 0x15, 0x20, 0x01};
     //msg_tcap->hdr.src = 0x3d;
     // msg_tcap->hdr.dst = SCP_TASK_ID;
     //msg_tcap->hdr.id = 0;
     //msg_tcap->hdr.rsp_req = 0x2000;
          
     //   pptr = get_param(msg_tcap);
     
     // memset(pptr, 0, msg_tcap->len);
     //memcpy(pptr, SCP_SCCP_GT_address_MSC, 13);
     //memcpy(pptr+13, SCP_SCCP_destination_MSC, 14);
     //memcpy(pptr+13+14, SCP_GT_MASK, 3);
 
     //   if (GCT_send(msg_tcap->hdr.dst, &msg_tcap->hdr) != 0)
     //	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
     //	     printf("message send error!");
     //	     relm(&msg_tcap->hdr);
     // }


     /*******************************end of SCCP GT translations configuration ******************************************/

/**************************** configure SCCP subsystem resource RSP parameters *********************************************/

     msg_buff_ptr = getm(SCP_MSG_CNF_SSR, 0, 0, SCPML_CNF_SSR);

     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = 0x33;
     msg_buff_ptr->hdr.rsp_req = 0x2000;
     msg_buff_ptr->hdr.id = 1;
     pptr = get_param(msg_buff_ptr);

     memset(pptr, 0, msg_buff_ptr->len);

     rpackbytes(pptr, SCPMO_CNF_SSR_cnf_ver ,   0,    SCPMS_CNF_SSR_cnf_ver );
     rpackbytes(pptr, SCPMO_CNF_SSR_ssr_type  ,   SCP_SSRT_RSP ,    SCPMS_CNF_SSR_ssr_type  ); //remote point code
     rpackbytes(pptr, SCPMO_CNF_SSR_module_id  ,   0,    SCPMS_CNF_SSR_module_id  );
     rpackbytes(pptr, SCPMO_CNF_SSR_mult_ind  ,   0,    SCPMS_CNF_SSR_mult_ind  );
     rpackbytes(pptr, SCPMO_CNF_SSR_spc ,   14140,   SCPMS_CNF_SSR_spc );
     rpackbytes(pptr, SCPMO_CNF_SSR_ssn  ,   0,    SCPMS_CNF_SSR_ssn  );
     rpackbytes(pptr, SCPMO_CNF_SSR_mgmt_id  ,   0xef,   SCPMS_CNF_SSR_mgmt_id );
     rpackbytes(pptr, SCPMO_CNF_SSR_ssr_flags  ,   0,   SCPMS_CNF_SSR_ssr_flags );
     rpackbytes(pptr, SCPMO_CNF_SSR_pc_mask ,   0,    SCPMS_CNF_SSR_pc_mask );
     rpackbytes(pptr, SCPMO_CNF_SSR_sio  ,   0,    SCPMS_CNF_SSR_sio  );
     rpackbytes(pptr, SCPMO_CNF_SSR_riid ,   0,    SCPMS_CNF_SSR_riid );
     rpackbytes(pptr, SCPMO_CNF_SSR_label_fmt ,   0,    21 ); //reserved octets set to 0 all

     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
}

/**************************** configure SCCP subsystem resource LSS parameters *********************************************/

     msg_buff_ptr = getm(SCP_MSG_CNF_SSR, 0, 0, SCPML_CNF_SSR);

     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = 0x33;
     msg_buff_ptr->hdr.rsp_req = 0x2000;
     msg_buff_ptr->hdr.id = 2;
     pptr = get_param(msg_buff_ptr);

     memset(pptr, 0, msg_buff_ptr->len);

     rpackbytes(pptr, SCPMO_CNF_SSR_cnf_ver,   0,    SCPMS_CNF_SSR_cnf_ver );
     rpackbytes(pptr, SCPMO_CNF_SSR_ssr_type,   SCP_SSRT_LSS ,    SCPMS_CNF_SSR_ssr_type  );
     rpackbytes(pptr, SCPMO_CNF_SSR_module_id,   0x14,    SCPMS_CNF_SSR_module_id  );
     rpackbytes(pptr, SCPMO_CNF_SSR_mult_ind,   0,    SCPMS_CNF_SSR_mult_ind  );
     rpackbytes(pptr, SCPMO_CNF_SSR_spc, 0,   SCPMS_CNF_SSR_spc );
     rpackbytes(pptr, SCPMO_CNF_SSR_ssn,   0x92,    SCPMS_CNF_SSR_ssn  );
     rpackbytes(pptr, SCPMO_CNF_SSR_mgmt_id,   0,   SCPMS_CNF_SSR_mgmt_id );
     // rpackbytes(pptr, SCPMO_CNF_SSR_ssr_flags,   SSRF_HBT,   SCPMS_CNF_SSR_ssr_flags ); //LSS heartbeat flag
     rpackbytes(pptr, SCPMO_CNF_SSR_ssr_flags,   0,   SCPMS_CNF_SSR_ssr_flags ); //without heartbeat flag
     rpackbytes(pptr, SCPMO_CNF_SSR_pc_mask,   0,    SCPMS_CNF_SSR_pc_mask );
     rpackbytes(pptr, SCPMO_CNF_SSR_sio,   0,    SCPMS_CNF_SSR_sio  );
     rpackbytes(pptr, SCPMO_CNF_SSR_riid,   0,    SCPMS_CNF_SSR_riid );
     rpackbytes(pptr, SCPMO_CNF_SSR_label_fmt,   0,    21 ); //reserved octets set to 0 all

     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
}

/**************************** configure SCCP subsystem resource RSS parameters *********************************************/

     msg_buff_ptr = getm(SCP_MSG_CNF_SSR, 0, 0, SCPML_CNF_SSR);

     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = 0x33;
     msg_buff_ptr->hdr.rsp_req = 0x2000;
     msg_buff_ptr->hdr.id = 3;
     pptr = get_param(msg_buff_ptr);

     memset(pptr, 0, msg_buff_ptr->len);

     rpackbytes(pptr, SCPMO_CNF_SSR_cnf_ver ,   0,    SCPMS_CNF_SSR_cnf_ver );
     rpackbytes(pptr, SCPMO_CNF_SSR_ssr_type,   SCP_SSRT_RSS ,    SCPMS_CNF_SSR_ssr_type  );
     rpackbytes(pptr, SCPMO_CNF_SSR_module_id,   0,    SCPMS_CNF_SSR_module_id  );
     rpackbytes(pptr, SCPMO_CNF_SSR_mult_ind,   0,    SCPMS_CNF_SSR_mult_ind  );
     rpackbytes(pptr, SCPMO_CNF_SSR_spc,   14140,   SCPMS_CNF_SSR_spc ); /*SSF GT*/
     rpackbytes(pptr, SCPMO_CNF_SSR_ssn,   0x92,    SCPMS_CNF_SSR_ssn  );
     rpackbytes(pptr, SCPMO_CNF_SSR_mgmt_id,   0xef,   SCPMS_CNF_SSR_mgmt_id );
     rpackbytes(pptr, SCPMO_CNF_SSR_ssr_flags,   0,   SCPMS_CNF_SSR_ssr_flags );
     rpackbytes(pptr, SCPMO_CNF_SSR_pc_mask,   0,    SCPMS_CNF_SSR_pc_mask );
     rpackbytes(pptr, SCPMO_CNF_SSR_sio,   0,    SCPMS_CNF_SSR_sio  );
     rpackbytes(pptr, SCPMO_CNF_SSR_riid,   0,    SCPMS_CNF_SSR_riid );
     rpackbytes(pptr, SCPMO_CNF_SSR_label_fmt,   0,    21 ); //reserved octets set to 0 all

     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
}

/**************************** configure SCCP subsystem concerned resource parameters *********************************************/

     msg_buff_ptr = getm(SCP_MSG_ADD_CONC, 0, 0, SCPML_ADD_CONC);

     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = SCP_TASK_ID;
     msg_buff_ptr->hdr.rsp_req = 0x2000;
     msg_buff_ptr->hdr.id = 0;
     pptr = get_param(msg_buff_ptr);

     memset(pptr, 0, msg_buff_ptr->len);

  rpackbytes(pptr, SCPMO_ADD_CONC_cnf_ver, 0 ,SCPMS_ADD_CONC_cnf_ver);
  rpackbytes(pptr,  SCPMO_ADD_CONC_ssr_type, SCP_SSRT_LSS  ,SCPMS_ADD_CONC_ssr_type);
  rpackbytes(pptr,  SCPMO_ADD_CONC_ssr_spc, 14100 ,SCPMS_ADD_CONC_ssr_spc); /*SCP*/
 rpackbytes(pptr,  SCPMO_ADD_CONC_ssr_ssn, 0x92 ,SCPMS_ADD_CONC_ssr_ssn);
 rpackbytes(pptr,  SCPMO_ADD_CONC_conc_type, SCP_SSRT_RSP ,SCPMS_ADD_CONC_conc_type);
 rpackbytes(pptr,  SCPMO_ADD_CONC_conc_spc, 14140 ,SCPMS_ADD_CONC_conc_spc); /*SSF*/
 rpackbytes(pptr,  SCPMO_ADD_CONC_conc_ssn, 0 ,SCPMS_ADD_CONC_conc_ssn);
 rpackbytes(pptr,  SCPMO_ADD_CONC_reserved, 0 ,SCPMS_ADD_CONC_reserved);

     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
}

     /**************SSN allowed**************************/
     //	       msg_tcap = getm(SCP_MSG_SCMG_REQ, 0, 0, 8);
     //
     //	       msg_tcap->hdr.src = 0x3d;
     //	       msg_tcap->hdr.dst = SCP_TASK_ID;
     //	       msg_tcap->hdr.id = 0x92;
     //	       msg_tcap->hdr.rsp_req = 0x2000;
     //	       pptr = get_param(msg_tcap);

     //	      memset(pptr, 0, msg_tcap->len);
     //TCPMO_CONFIG_V1_cnf_ver
     //	      rpackbytes(pptr, 0,   1,    1);
     //	     rpackbytes(pptr, 1, 1,    1 );
     
     //	   if (GCT_send(msg_tcap->hdr.dst, &msg_tcap->hdr) != 0)
     //	   {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
     //	   printf("message send error!");
     //	   relm(&msg_tcap->hdr);
     //	   }

     /**************SSN allowed**************************/
     //	       msg_tcap = getm(SCP_MSG_SCMG_REQ, 0, 0, 8);

     //	       msg_tcap->hdr.src = 0x3d;
     //	       msg_tcap->hdr.dst = SCP_TASK_ID;
     //	       msg_tcap->hdr.id = 0x01;
     //	       msg_tcap->hdr.rsp_req = 0x2000;
     //	       pptr = get_param(msg_tcap);

     //	      memset(pptr, 0, msg_tcap->len);
     //TCPMO_CONFIG_V1_cnf_ver
     //	      rpackbytes(pptr, 0,   1,    1);
     //	     rpackbytes(pptr, 1, 1,    1 );
     
     //	   if (GCT_send(msg_tcap->hdr.dst, &msg_tcap->hdr) != 0)
     //	   {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
     //	   printf("message send error!");
     //	   relm(&msg_tcap->hdr);
     //	   }


/************************************/

     /**************SSN allowed**************************/
     //     msg_tcap = getm(SCP_MSG_SCMG_REQ, 0, 0, 8);
     //
     // msg_tcap->hdr.src = 0x3d;
     //msg_tcap->hdr.dst = SCP_TASK_ID;
     //msg_tcap->hdr.id = 0x92;
     
     //pptr = get_param(msg_tcap);

     //memset(pptr, 0, msg_tcap->len);
     //TCPMO_CONFIG_V1_cnf_ver
     //rpackbytes(pptr, 0,   1,    1);
     //rpackbytes(pptr, 1, 1,    1 );
     
     //if (GCT_send(msg_tcap->hdr.dst, &msg_tcap->hdr) != 0)
     //	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
     //	     printf("message send error!");
     //	     relm(&msg_tcap->hdr);
     //	 }
     /*******************************************/
     /**********************INAP TRACE ********/
     /*msg_tcap = getm(INAP_MSG_TRACE_MASK , 0, 0, INAPML_TRACE_MASK );

     msg_tcap->hdr.src = 0x3d;
     msg_tcap->hdr.dst = INAP_TASK_ID;
     //msg_tcap->hdr.id = 0x21;
     pptr = get_param(msg_tcap);

memset(pptr, 0, msg_tcap->len);

     rpackbytes(pptr, INAPMO_TRACE_MASK_op_evt_mask  ,   15,    INAPMS_TRACE_MASK_op_evt_mask  );
     rpackbytes(pptr, INAPMO_TRACE_MASK_ip_evt_mask   ,  15  , INAPMS_TRACE_MASK_ip_evt_mask   );
     rpackbytes(pptr, INAPMO_TRACE_MASK_non_prim_mask    , 127  , INAPMS_TRACE_MASK_non_prim_mask  );


     if (GCT_send(msg_tcap->hdr.dst, &msg_tcap->hdr) != 0)
	 {
	     printf("message send error!");
	     relm(&msg_tcap->hdr);
     
	     }*/
     /**********************************************************************************************/

/**************************** configure SCCP GT translation ******************************************************/
//local SCCP gt translation --  this SCCP GT translation need to transfer IDP to INTU application
//when remove this translation -- IDP component doesn't transferred to INTU binary

msg_buff_ptr = getm(SCP_MSG_GTT_ADD, 0, 0, 30+1 );

//commerc
// unsigned char SCP_SCCP_GT_address[] = {0x08, 0x0b, 0x12, 0x92, 0x00, 0x11, 0x04, 0x97, 0x05, 0x66, 0x15, 0x20, 0x05};
 
//testIN GT 79506651002
//target address
unsigned char SCP_SCCP_GT_address[] = {0x08, 0x0b, 0x12, 0x92, 0x00, 0x11, 0x04, 0x97, 0x05, 0x66, 0x15, 0x00, 0x02};

//commerc SPc where GT should be routed  27 37 = 14119
// unsigned char SCP_SCCP_destination_scp[] = {0x09, 0x0c, 0x11, 0x27, 0x37, 0x00, 0x11, 0x04, 0x97, 0x05, 0x66, 0x15, 0x20, 0x05};

//test IN SPc where GT should be routed  14 37 = 14100 and also shoul use SCP GT
 unsigned char SCP_SCCP_destination_scp[] = {0x09, 0x0c, 0x11, 0x14, 0x37, 0x00, 0x11, 0x04, 0x97, 0x05, 0x66, 0x15, 0x00, 0x02};


 unsigned char SCP_GT_MASK[] = {0x1c, 0x01, 0x00};
 msg_buff_ptr->hdr.src = 0x3d;
 msg_buff_ptr->hdr.dst = SCP_TASK_ID;
 msg_buff_ptr->hdr.id = 0;
 msg_buff_ptr->hdr.rsp_req = 0x2000;
          
        pptr = get_param(msg_buff_ptr);
     
      memset(pptr, 0, msg_buff_ptr->len);
      memcpy(pptr, SCP_SCCP_GT_address, 13);
     memcpy(pptr+13, SCP_SCCP_destination_scp, 14);
     memcpy(pptr+13+14, SCP_GT_MASK, 3);
 
        if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
     	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
     	     printf("message send error!");
     	     relm(&msg_buff_ptr->hdr);
      }
	  //remote sccp gt
     //msg_tcap = getm(SCP_MSG_GTT_ADD, 0, 0, 30+1 );


     //unsigned char SCP_SCCP_GT_address_MSC[] = {0x08, 0x0b, 0x12, 0x92, 0x00, 0x11, 0x04, 0x97, 0x05, 0x66, 0x15, 0x20, 0x01};
     //unsigned char SCP_SCCP_destination_MSC[] = {0x09, 0x0c, 0x11, 0x3c, 0x37, 0x00, 0x11, 0x04, 0x97, 0x05, 0x66, 0x15, 0x20, 0x01};
     // msg_tcap->hdr.src = 0x3d;
     //msg_tcap->hdr.dst = SCP_TASK_ID;
     //msg_tcap->hdr.id = 0;
     //msg_tcap->hdr.rsp_req = 0x2000;
          
     //   pptr = get_param(msg_tcap);
     
     // memset(pptr, 0, msg_tcap->len);
     //memcpy(pptr, SCP_SCCP_GT_address_MSC, 13);
     //memcpy(pptr+13, SCP_SCCP_destination_MSC, 14);
     //memcpy(pptr+13+14, SCP_GT_MASK, 3);
 
     //   if (GCT_send(msg_tcap->hdr.dst, &msg_tcap->hdr) != 0)
     // 	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
     //	     printf("message send error!");
     //	     relm(&msg_tcap->hdr);
     // }


     /*******************************end of SCCP GT translations configuration ******************************************/

     /**************SSN allowed**************************/
     //	       msg_tcap = getm(SCP_MSG_SCMG_REQ, 0, 0, 8);

     //	       msg_tcap->hdr.src = 0x3d;
     //	       msg_tcap->hdr.dst = SCP_TASK_ID;
     //	       msg_tcap->hdr.id = 0x92;
     //	       msg_tcap->hdr.rsp_req = 0x2000;
     //	       pptr = get_param(msg_tcap);

     //	      memset(pptr, 0, msg_tcap->len);
     //TCPMO_CONFIG_V1_cnf_ver
     //	      rpackbytes(pptr, 0,   1,    1);
     //	     rpackbytes(pptr, 1, 1,    1 );
     
     //	   if (GCT_send(msg_tcap->hdr.dst, &msg_tcap->hdr) != 0)
     //	   {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
     //	   printf("message send error!");
     //	   relm(&msg_tcap->hdr);
     //	   }
     /*******************************************/

/****************INAP Application Context configuration*********************************/

	msg_buff_ptr = getm(INAP_MSG_CNF_AC, 0, 0, 20);

     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = 0x35;
     msg_buff_ptr->hdr.id = 0x21;
     pptr = get_param(msg_buff_ptr);
     //INAP_AC 0x21 0xa109060704000001003201
     unsigned char ACtx[] = {0xa1, 0x09, 0x06, 0x07, 0x04, 0x0, 0x0, 0x01, 0x0, 0x32, 0x01};
     //example for two AC references
     //S7_MGT Tx: M-I0000-t77f6-i0021-fcf-d35-s00-r8000-p00000000000000000ba109060704000001003201
     //S7_MGT Tx: M-I0000-t77f6-i0050-fcf-d35-s00-r8000-p00000000000000000ba109060704000001150304     


     memset(pptr, 0, msg_buff_ptr->len);

     rpackbytes(pptr, INAPMO_CNF_AC_filler  ,   0,    INAPMS_CNF_AC_filler  );
     rpackbytes(pptr, INAPMO_CNF_AC_len   ,  11  ,   INAPMS_CNF_AC_len   );
     //rpackbytes(pptr, INAPMO_CNF_AC_context    ,   0x4001003201,   5  ); //zaebalca eto otlazhivat
     memcpy(pptr+INAPMO_CNF_AC_context, ACtx, 11);     

     
     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
     
	 }
     /***********************************************************/
     /**********************INAP TRACE ********/
     //msg_tcap = getm(INAP_MSG_TRACE_MASK , 0, 0, INAPML_TRACE_MASK );

     //msg_tcap->hdr.src = 0x3d;
     //msg_tcap->hdr.dst = INAP_TASK_ID;
     //msg_tcap->hdr.id = 0x21;
     //pptr = get_param(msg_tcap);

     //memset(pptr, 0, msg_tcap->len);

     ///rpackbytes(pptr, INAPMO_TRACE_MASK_op_evt_mask  ,   15,    INAPMS_TRACE_MASK_op_evt_mask  );
     //rpackbytes(pptr, INAPMO_TRACE_MASK_ip_evt_mask   ,  15  , INAPMS_TRACE_MASK_ip_evt_mask   );
     //rpackbytes(pptr, INAPMO_TRACE_MASK_non_prim_mask    , 127  , INAPMS_TRACE_MASK_non_prim_mask  );


     //if (GCT_send(msg_tcap->hdr.dst, &msg_tcap->hdr) != 0)
     //	 {
     //	     printf("message send error!");
							    //	     relm(&msg_tcap->hdr);
     
		      // }

/*******************************/
 //		     sa.sa_handler = sig_handler;
		     // sigemptyset (&new_action.sa_mask);
 //		     sa.sa_flags = SA_SIGINFO;
		     //		     sigaction (SIGRTMIN, &sa, NULL);
     //timer_settime(timer, 0, &ts, NULL);

	    /* use spin locks */
     //elm    Global = 0;
	    //pthread_spinlock_t spin;
     //elm     pthread_spin_init(&spin, 0);


    /**************SSN allowed**************************/
	       msg_buff_ptr = getm(SCP_MSG_SCMG_REQ, 0, 0, 8);

	       msg_buff_ptr->hdr.src = 0x3d;
	       msg_buff_ptr->hdr.dst = SCP_TASK_ID;
	       msg_buff_ptr->hdr.id = 0x92; //ssn = CAP
	       msg_buff_ptr->hdr.rsp_req = 0x2000;
	       pptr = get_param(msg_buff_ptr);

	      memset(pptr, 0, msg_buff_ptr->len);
     //TCPMO_CONFIG_V1_cnf_ver
	      rpackbytes(pptr, 0,   1,    1);
	     rpackbytes(pptr, 1, 1,    1 );
     
	   if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	   {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	   printf("message send error!");
	   relm(&msg_buff_ptr->hdr);
	   }

/**********************SCCP module  TRACE set code template ********/
/* just check to wich module traced messages will transfer and receive it in this module! */
     /* msg_buff_ptr = getm(SCP_MSG_TRACE_MASK , 0, 0, SCPML_TRACE_MASK ); */

     /* msg_buff_ptr->hdr.src = 0x3d; */
     /* msg_buff_ptr->hdr.dst = SCP_TASK_ID; */
     /* //msg_tcap->hdr.id = 0x21; */
     /* pptr = get_param(msg_buff_ptr); */

     /* memset(pptr, 0, msg_buff_ptr->len); */

     /* rpackbytes(pptr, SCPMO_TRACE_MASK_op_evt_mask  ,   0x3,    SCPMS_TRACE_MASK_op_evt_mask  ); */
     /* rpackbytes(pptr, SCPMO_TRACE_MASK_ip_evt_mask   ,  0x3   , SCPMS_TRACE_MASK_ip_evt_mask   ); */
     /* rpackbytes(pptr, SCPMO_TRACE_MASK_non_prim_mask    , 0x6  , SCPMS_TRACE_MASK_non_prim_mask  ); */


     /* if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0) */
     /* 	 { */
     /* 	     printf("message send error!"); */
     /* 	     relm(&msg_buff_ptr->hdr); */
     
     /* 	 } */

/***************************** sccp gt translation test code template ************************************************/
/*use this template for GT translation test */
/*in this case we test some international GT whish is not in GT translation table */
     msg_buff_ptr = getm(SCP_MSG_GTT_TEST, 0, 0, SCPML_CNF_SSR); //TODO - here should check length!
     msg_buff_ptr->hdr.src = 0x3d;
     msg_buff_ptr->hdr.dst = 0x33;
     msg_buff_ptr->hdr.rsp_req = 0x2000;
     msg_buff_ptr->hdr.id = 0;
     msg_buff_ptr->len = 14; //1 byte is 9 byte in param area for indicating end of params
     pptr = get_param(msg_buff_ptr);

     //SCP GT for test coded as TYPE - LENGTV VALUE
     //SCCP calling and called number coded as in Q.713
     //b - length
     //12 - address indicator
     //92 - ssn
     //and then goes GTI and GT digits, as in wireshark trace
     unsigned char GT_test[] = {0x05, 0x0b, 0x12, 0x92, 0x00, 0x11, 0x04, 0x79, 0x51, 0x40, 0x10, 0x24, 0x90};

     memset(pptr, 0, msg_buff_ptr->len);

     memcpy(pptr, GT_test, 13);
     /* rpackbytes(pptr, SCPMO_CNF_SSR_ssr_type,   SCP_SSRT_LSS ,    SCPMS_CNF_SSR_ssr_type  ); */

     if (GCT_send(msg_buff_ptr->hdr.dst, &msg_buff_ptr->hdr) != 0)
	 {
	     /*
	      * GCT_send() has failed.
	      * Sending process retains ownerships of the MSG.
	      * Release the MSG.
	      */
	     printf("message send error!");
	     relm(&msg_buff_ptr->hdr);
	 }

/***************************************************************************/



	   /*SIGHUP handler installation */
	   //struct sigaction sa;

//bzero(&act, sizeof(struct sigaction));
sa.sa_handler = sighup;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
 sa.sa_flags |= SA_RESTART; /*interrupted syscalls restarted!*/
 if ( sigaction(SIGHUP, &sa, NULL) < 0 )
     {
         perror("sigaction:");
         exit(1);
    }
	   
/************************************** main loop started here ******************************************/
	while (1)
	{

	    if (services_conf_reread_flag == 1)
		{
		printf("we need to reread services configuration file and initially bootstrap data in tnt!\n");

		update_services_config();
		services_conf_reread_flag = 0;
		}

	    if ((h = GCT_receive(intu_mod_id)) != 0)
		{
		    switch (h->type)
			{
			case INAP_MSG_SRV_IND :
			    printf("srv ind received!\n");
				INTU_srv_ind(h);
				break;
			case INAP_MSG_DLG_IND :
			    printf("dlg ind received!\n");
				INTU_dlg_ind(h);
				break;
			case TCP_MSG_MAINT_IND:
			    if (h->status == TCPEV_DLG_TIM_TIMEOUT)
				{
			    printf("CAMELGW: dialog timeout occured received from TCAP module\n");
    INTU_send_close(h->id, INAPRM_prearranged_end);
    //TODO!!!    INTU_change_state(h->id, INTU_get_dlg_cb(&ic_dlg_id, h), INTU_STATE_CLOSING);
				}
			    else
				INTU_disp_other_msg(h);
			    break;
			default : INTU_disp_other_msg(h);
				break;
			}
			/*
			* Once we have finished processing the message
			* it must be released to the pool of messages.
			*/
			relm(h);
		}
	    else
		{
		    printf("h pointer = %p\n",h);
		    perror("intuc:");
			}
	}
	return 0; //never arrive here
}
/*
*******************************************************************************
*                                                                             *
* INTU_dlg_ind: handles dialogue indications                                  *
*                                                                             *
*******************************************************************************
*/
/*
* Returns 0 on success
*         INTUE_INVALID_DLG_ID if the dialogue id was invalid
*         INTUE_MSG_DECODE_ERROR if the message could not be decoded
*/
int INTU_dlg_ind(HDR *h)
{
	DLG_CB	*dlg_ptr;	// Pointer to the per-dialogue id control block  
	u32	ic_dlg_id;		// Dialogue id of the incoming dialogue id 
	u8	dialogue_type;	// Type of the incoming dialogue indication  

	int	decode_status;	// Return status of the dialogue type recovery  

		printf("INTU_dlg_ind function call\n");
	
	dlg_ptr = INTU_get_dlg_cb(&ic_dlg_id, h);

	if (dlg_ptr != 0)
		ic_dlg_id = h->id;
	else
		return(INTUE_INVALID_DLG_ID);

	/*
	* We know the message is a Dialogue Indication but what type is it...
	* OPEN, CLOSE, DELIMIT, P_ABORT, U_ABORT, NOTICE or OPEN_RSP
	*/
	decode_status = IN_get_dialogue_type(h, &dialogue_type);

	printf("file: %s   function:  %s     line: %d :     dialogue type = %d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__, dialogue_type);

	if (decode_status != IN_SUCCESS)
	{
		INTU_disp_other_msg(h);
		return INTUE_MSG_DECODE_ERROR;
	}

	/*
	* If dialogue indication tracing is enabled display the message
	*/
	if (intu_options & INTU_OPT_TR_DLG_IND)
	    INTU_disp_dlg_msg(ic_dlg_id, h);
	/*
	* Switch on the current state of the dialogue control block for the
	* incoming dialogue indication.
	*/
	switch (dlg_ptr->state)
	{
		/*
		* INTU_STATE_IDLE:
		* In this state we expect to receive only OPEN indications
		*/
	case INTU_STATE_IDLE:
		if (dialogue_type == INDT_OPEN)
		{
			INTU_open_dialogue(ic_dlg_id, dlg_ptr, h);
		}
		else
		{
		    INTU_disp_unexpected_dlg_ind(ic_dlg_id, dlg_ptr, h);
			INTU_send_u_abort(ic_dlg_id);
		}
		break;

		/*
		* INTU_STATE_OPEN:
		* In this state we wouldn't expect to receive a dialogue 
		* indication if the dialogue is progressing normally but some 
		* indications are still valid.
		* For example a failure in the network could result in a P_ABORT
		*/
	case INTU_STATE_OPEN:
		switch (dialogue_type)
		{
		case INDT_OPEN: 
		    INTU_disp_dlg_reopened(ic_dlg_id, dlg_ptr, h);
			INTU_open_dialogue(ic_dlg_id, dlg_ptr, h);
			break;
		case INDT_CLOSE:
		    INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED); //TODO - when basic end with TC - END receiving Megafon case
		    //    			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
			break;

		case INDT_DELIMIT:
		    printf("CAMELGW: receive IND-DELIMIT dlg ind when in STATE OPEN!\n");
		    INTU_disp_unexpected_dlg_ind(ic_dlg_id, dlg_ptr, h);
			break;

		case INDT_U_ABORT:
		case INDT_P_ABORT:
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			break;

		case INDT_NOTICE:
			INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
			break;

		case INDT_OPEN_RSP:
		    INTU_disp_unexpected_dlg_ind(ic_dlg_id, dlg_ptr, h);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			break;

		default:
		    INTU_disp_invalid_dlg_ind(ic_dlg_id, dlg_ptr, h);
			break;
		}
		break;

		/*
		* INTU_STATE_PENDING_DELIMIT:
		* In this state we are waiting for the DELIMIT to trigger any 
		* service logic waiting to be processed.
		*/
	case INTU_STATE_PENDING_DELIMIT:
		switch (dialogue_type)
		{
		case INDT_OPEN:
		    INTU_disp_dlg_reopened(ic_dlg_id, dlg_ptr, h);
			INTU_open_dialogue(ic_dlg_id, dlg_ptr, h);
			break;

		case INDT_CLOSE:
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_TIMEOUT);
			break;

		case INDT_DELIMIT:
		   	fprintf(stderr,"CAMELGW: *** На вход получил сообщение INDT_DELIMIT от модуля INAP ***\n");
			INTU_process_pending_req(ic_dlg_id, dlg_ptr, h);
			break;

		case INDT_U_ABORT:
		case INDT_P_ABORT:
		case INDT_NOTICE:
		case INDT_OPEN_RSP:
		    INTU_disp_unexpected_dlg_ind(ic_dlg_id, dlg_ptr, h);
			INTU_send_u_abort(ic_dlg_id);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			break;
		default:
		    INTU_disp_invalid_dlg_ind(ic_dlg_id, dlg_ptr, h);
			break;
		}
		break;

		/*
		* INTU_STATE_CLOSING:
		* In this state we have already sent a CLOSE DLG-REQ with 
		* pre-arranged end and are just waiting to receive the CLOSE 
		* DLG-IND from INAP.
		*/
	case INTU_STATE_CLOSING:
		switch(dialogue_type)
		{
		case INDT_OPEN:
		    INTU_disp_dlg_reopened(ic_dlg_id, dlg_ptr, h);
			INTU_open_dialogue(ic_dlg_id, dlg_ptr, h);
			break;

		case INDT_CLOSE:
		        printf("receive INDT_CLOSE in state INTU STATE CLOSING\n");
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
			break;

		case INDT_DELIMIT: /*megafon case - we could receive srv ind and delimit when intu in CLOSING_STATE */
		    //		    	fprintf(stderr,"INTU: *** На вход получил сообщение INDT_DELIMIT от модуля INAP находясь в сотоянии CLOSING_STATE ***\n");
			INTU_process_pending_req(ic_dlg_id, dlg_ptr, h);
			break;

		case INDT_U_ABORT:
		case INDT_P_ABORT:
		case INDT_NOTICE:
		case INDT_OPEN_RSP:
		    INTU_disp_unexpected_dlg_ind(ic_dlg_id, dlg_ptr, h);
			INTU_send_u_abort(ic_dlg_id);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			break;

		default:
		    INTU_disp_invalid_dlg_ind(ic_dlg_id, dlg_ptr, h);
			break;
		}
		break;

	default:
		break;
	}
	return 0;
}

/*
*******************************************************************************
*                                                                             *
*   INTU_srv_ind: handles service indications for on a dialogue 	      *
*                                                                             *
********************************************************************************/
/*
* Returns 0 on success,
*         INTUE_INVALID_DLG_ID if the dialogue id was invalid
*         INTUE_MSG_DECODE_ERROR if the component type could not be decoded
* 
*/
int INTU_srv_ind(HDR *h) {
	DLG_CB	*dlg_ptr;	// Pointer to the per-dialogue id control block 	
	u32	ic_dlg_id;		// Dialogue id of the incoming dialogue indication 
	u8	cpt_type;		// Type of the incoming service indication 
	int	decode_status;	// Return status of the component type recovery 

	//int invokeID_len, invokeID; //test

	int status; //test

	printf("INTU_srv_ind function call\n");

	dlg_ptr = INTU_get_dlg_cb(&ic_dlg_id, h);

	if (dlg_ptr != 0)
		ic_dlg_id = h->id;
	else
		return INTUE_INVALID_DLG_ID;

	decode_status = IN_get_component_type(h, &cpt_type);

	//status = IN_get_component_param(INAPPN_invoke_id, &invokeID_len, &invokeID, 1, buffer_cpt_ptr);/
	//status = IN_get_component_param(INAPPN_invoke_id, &invokeID_len, &invokeID, 1, &(dlg_ptr->cpt));//dlg_cpt_ptr);
	printf("cpt type in SRV IND function = %d\n", cpt_type);

	if (decode_status != IN_SUCCESS)
	{
		INTU_disp_other_msg(h);
		return INTUE_MSG_DECODE_ERROR;
	}

	if (intu_options & INTU_OPT_TR_SRV_IND)
	    INTU_disp_srv_ind(ic_dlg_id, dlg_ptr, h);

	switch (dlg_ptr->state)
	{
		/*
		* INTU_STATE_IDLE:
		* The dialogue is not yet open, print the unexpected message.
		*/
	case INTU_STATE_IDLE:
		fprintf(stderr, "intu() : INTU in IDLE state, couldn't receive SRV_IND\n");
		INTU_disp_unexpected_srv_ind(ic_dlg_id, dlg_ptr, h);
		INTU_send_u_abort(ic_dlg_id);
		break;

		/*
		* INTU_STATE_OPEN:
		* In this state we would expect to receive a service indication 
		* with the INVOKE (InitialDP). Handle this INVOKE to prepare the 
		* service logic.
		*/

	case INTU_STATE_OPEN:
		switch (cpt_type)
		{
		case INCPT_INVOKE:
			INTU_handle_invoke(ic_dlg_id, dlg_ptr, h);
			break;
		case INCPT_REJECT:
		    fprintf(stderr, "intu() : INTU in OPEN state, receive SRV_IND with REJECT component\n");
		    INTU_disp_unexpected_srv_ind(ic_dlg_id, dlg_ptr, h);
		    INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
		    break;
		case INCPT_U_ERROR:
		    fprintf(stderr, "intu() : INTU in OPEN state, receive SRV_IND with U_ERROR component\n");
		    INTU_disp_unexpected_srv_ind(ic_dlg_id, dlg_ptr, h);
		    INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
		    break;

		case INCPT_RESULT_L:
		    //	ResultLast received in case when we send ActivityTest to SSF
			fprintf(stderr, "CAMELGW : INTU in OPEN state, and receive SRV_IND with ReturnResultLast.........\n");
			CAMELGW_handle_returnresultlast(ic_dlg_id, dlg_ptr, h); /*process return result last */
			//			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT); /*and change state to PENDING DELIMIT */
			break;
		case INCPT_NULL:
			fprintf(stderr, "intu() : INTU in OPEN state, couldn't receive SRV_IND\n");
			INTU_disp_unexpected_srv_ind(ic_dlg_id, dlg_ptr, h);
			INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
			break;
		default:
		    INTU_disp_invalid_srv_ind(ic_dlg_id, dlg_ptr, h);
			break;
		}
		break;

		/*
		* INTU_STATE_PENDING_DELIMIT:
		* We're waiting for a DELIMIT, we shouldn't expect any more 
		* SRV-IND in this state.
		*
		* INTU_STATE_CLOSING:
		* We're waiting for a CLOSE, we shouldn't expect any more 
		* SRV-IND in this state.
		*/
	case INTU_STATE_PENDING_DELIMIT: /* in this state intu could receive also another srv_ind with another component */
		switch (cpt_type)
		{
		case INCPT_INVOKE: /* intercept another INVOKES from MSC - ACR, RRBEvent and other in PENDING_DELIMIT_STATE and call handle_invoke function  */
			INTU_handle_invoke(ic_dlg_id, dlg_ptr, h);
			break;
		}
		break; 
	case INTU_STATE_CLOSING:
	    if (cpt_type == INCPT_INVOKE)
		{
		    INTU_handle_invoke(ic_dlg_id, dlg_ptr, h);
		}
	    else
		{
		fprintf(stderr, "intu() : INTU in CLOSING state, couldn't receive SRV_IND\n");
		//		INTU_process_ApplyChargingReport(ID, ic_dlg_id, dlg_ptr, h);

		//oldAPI version		INTU_disp_unexpected_srv_ind(dlg_ptr, h);
		INTU_disp_unexpected_srv_ind(ic_dlg_id, dlg_ptr, h);


		INTU_send_u_abort(ic_dlg_id);
		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
		}
		break;

	default:
		break;
	}
	return 0;
}
/*
*******************************************************************************
*                                                                             *
* INTU_change_state: handles the changing of state for dialogue control blocks*
*                                                                             *
*******************************************************************************
*/
int INTU_change_state(u32 ic_dlg_id, DLG_CB *dlg_ptr, u8 new_state)
//u16	ic_dlg_id;      // Dialogue id of the incoming dialogue indication
//DLG_CB	*dlg_ptr;	// Pointer to the per-dialogue id control block
//u8	new_state;		// New state to change the dlg pointed to by dlg_ptr
{
	if (dlg_ptr != 0)
	{
		INTU_disp_state_change(ic_dlg_id, dlg_ptr, new_state);
		dlg_ptr->state = new_state;
	}
	if (new_state == INTU_STATE_IDLE)
	{
		dlg_ptr->cpt_list_head = NULL; /* для каждого диалога список компонент изначально состоит как бы из одного узла, указатель на голову = null */
		dlg_ptr->cursor = 0;
		//may be need this one memset(&(dlg_ptr->call_details), 0, sizeof(struct calldetails));
	}

	return 0;
}

/*
*******************************************************************************
*                                                                             *
* 			 INTU_open_dialogue       				      *
*                                                                             *
*******************************************************************************
*/
/*
* Checks the application context in index form against the supported contexts
* and opens the dialogue control block for the dialogue id of the incoming
* message. Dialogue statistics are updated and the dialogue state is set
* to OPEN.
*
* Returns: 0 on success
*          INTUE_INVALID_APPLIC_CONTEXT - if the application context was
*                                         invalid or unsupported.
*          INTUE_INIT_CPT_FAILED - if the component structure in the DLG_CB
*                                  could not be initialised.
*/
 int INTU_open_dialogue(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h)
{

	void 	*prot_ptr;
	int	status;
	u16	dlg_ac_idx;
	u8	ac_index_data[2];
	PLEN	ac_index_data_len;

	//	u8 dlg_timeout = 20;
	status = IN_get_dialogue_param(INDP_applic_context_index, &ac_index_data_len, ac_index_data, sizeof(ac_index_data),h);
	// status = IN_set_dialogue_param(INDP_dlg_idle_timeout, 1, &dlg_timeout, h);
	//printf("ac_index_data_len %i \n",ac_index_data_len);
	//printf("ac index data %d\n", ac_index_data);

	if (status != IN_SUCCESS)
	    {
		fprintf(stderr,"INTU: *** No application context index  ***\n");
		return(INTUE_INVALID_APPLIC_CONTEXT);
	    }


	if ((ac_index_data_len > 2) || (ac_index_data_len == 0))
	{
		fprintf(stderr, "INTU: *** Invalid application context index  ***\n");
		return(INTUE_INVALID_APPLIC_CONTEXT);
	}
	else
	{
		if (ac_index_data_len == 2)
			dlg_ac_idx = (ac_index_data[1] << 8) & 0xff00 + ac_index_data[0] & 0xff;
		else
			dlg_ac_idx = ac_index_data[0];

		fprintf(stderr, ANSI_COLOR_YELLOW  "OPEN_DIALOGIE: dlg_ac_index(dec_value) %i" ANSI_COLOR_RESET "\n",dlg_ac_idx);

		prot_ptr = INTU_get_protocol_definition(dlg_ac_idx);

		/*we should init cpt structure for incoming components */
		
		if (prot_ptr != 0)
		    {
			//#ifdef IN_LMSGS
			status = IN_init_component(prot_ptr, &dlg_ptr->cpt, 0);
			//#else
			//status = IN_init_component(prot_ptr, &dlg_ptr->cpt);
			//#endif
			    if (status != IN_SUCCESS)
				return(INTUE_INIT_CPT_FAILED);
		    }
		else
		    {
			fprintf(stderr, "INTU: *** Unsupported application context index ***\n");
			return(INTUE_INVALID_APPLIC_CONTEXT);
		    }
		
		dlg_ptr->reply_prepared = INTU_REPLY_NONE;

		/*
		* If the dialogue was not idle then it is being re-opened and 
		* the old dialogue is complete
		*/
		if (dlg_ptr->state != INTU_STATE_IDLE)
			completed_dialogues++;

		/*
		* New active dialogue opening
		*/
		active_dialogues++;
		INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN);

		if (intu_options & INTU_OPT_TR_ACTV_DLG)
		    {
			printf("INTU: Dialogues: Active [%i], Completed [%i], Successful [%i], Failed [%i]\n", active_dialogues, completed_dialogues, successful_dialogues, completed_dialogues - successful_dialogues);
		    }
		return 0;
	}
}
/*
*******************************************************************************
*                                                                             *
*    		 INTU_close_dialogue        				      *
*                                                                             *
*******************************************************************************
*/
/*
* Check for a pending reply, if one exists print a warning. Return the
* dialogue state to IDLE and update the dialogue statistics.
*
* Returns 0
*/
int INTU_close_dialogue(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 dlg_close_rsn)
{
//u16    ic_dlg_id;      // Dialogue id of the incoming dialogue indication 
//DLG_CB *dlg_ptr;       // Pointer to the per-dialogue id control block 
//HDR    *h;             // Received message
//u8     dlg_close_rsn;  // Set to INTU_DLG_SUCCESS if the dialogue completed 
//as expected

	 union sigval val;	    
	struct alarm alrm; 	    

    
	switch (dlg_ptr->reply_prepared)
	{
	case INTU_REPLY_NONE:
		break;

	case INTU_REPLY_INVOKE:
	case INTU_REPLY_ERROR:
	case INTU_REPLY_REJECT:
	case INTU_REPLY_U_ABORT:
		if (intu_options & INTU_OPT_TR_ACTV_DLG)
		{
			printf("INTU: *** Dialogue 0x%04x closing with pending reply [%i]***\n", ic_dlg_id, dlg_ptr->reply_prepared);
		}
		break;

	default:
		break;
	}

	/*
	* Active dialogue closing, Another dialogue completed
	*/
	active_dialogues--;
	completed_dialogues++;
	if (dlg_close_rsn == INTU_DLG_SUCCESS)
		successful_dialogues++;

	if (dlg_close_rsn == INTU_DLG_TIMEOUT)
	    {
		successful_dialogues++;
		timeout_dialogues++;
	    }

	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_IDLE);
	if (intu_options & INTU_OPT_TR_ACTV_DLG)
	{

	    printf("INTU: Dialogues: Active [%i], Completed [%i], Successful [%i], Timeout [%i], Failed [%i]\n", active_dialogues, completed_dialogues, successful_dialogues, timeout_dialogues, completed_dialogues - successful_dialogues);
	   
	    //union sigval val;	    
	    //struct alarm alrm; 	    
	    //	    char string_buff[512];	    

	    // sprintf(string_buff,"INTU: Dialogues: Active [%i], Completed [%i], Successful [%i], Failed [%i]\n", active_dialogues, completed_dialogues, successful_dialogues, completed_dialogues - successful_dialogues);
	      //alrm.descriptor = 8;
	      //alrm.severity = 55;
          
	      //const struct alarm *Alarm1;
	      //Alarm1 = &alrm;
	   
	      //val.sival_ptr = Alarm1;
	    //val.sival_ptr = &string_buff[0];
	   //val.sival_int = 13;
	   //TODO - need check result of sigqueue
	    //printf("val.sival ptr in intu = %p\n",val.sival_ptr); 
	    //  printf("descriptor ptr 1 in intu ptr = %p\n",alrm.descriptor);
	   /*   printf("severity in intu  = %i\n",alrm.severity); */
	    //  printf("descriptor ptr 2  in intu ptr = %p\n",Alarm1->descriptor); 
	   /*   printf("descriptor as srint = %s in intu \n", ((struct alarm *) val.sival_ptr) ->descriptor); */
	  
	    //     sigqueue(getpid(), SIGRTMIN,  val );
	}

	return 0;
}

/*
*******************************************************************************
*                                                                             *
* INTU_handle_invoke: логика на принятый приложением шлюза SRV_IND = INVOKE   *
*                                                                             *
*******************************************************************************
*/
int INTU_handle_invoke(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h)
{
    //u16    ic_dlg_id;      // Dialogue id of the incoming dialogue indication 
    //DLG_CB *dlg_ptr;       // Pointer to the per-dialogue id control block 
    //HDR    *h;             // Received message 

	struct calldetails	*dlg_calldetails_ptr;
	SERVICE_KEY_TYPE	srv_key;	// Recovered serv_key of the IDP 
	LOG_MSG	message;

	IN_CPT	*dlg_cpt_ptr;			// Recovered INAP Component 
	IN_CPT	*dlg_cpt_ptr_new;		// Recovered INAP Component 
	int invokeID_len, invokeID;
	int	status;						// Return status fron INAP API func 
	int	length;
	int	i;

	u16	op_code;         			// Opcode of the recovered invoke 

	//	printf("INTU_handle_invoke function call\n");
	//	printf("cursor = %d\n", dlg_ptr->cursor);

	dlg_cpt_ptr = &(dlg_ptr->cpt);

	//fprintf(stderr, ANSI_COLOR_YELLOW "string CallReferenceNumber as it goes in handle invoke function =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallReferenceNumber);
	
	status = IN_decode_operation(dlg_cpt_ptr,h);

	if (status != IN_SUCCESS)
	    {
		if (status == IN_REJECT_REQUIRED)
		    {
			fprintf(stderr,"INTU: *** Operation Invoke rejected ***\n");
			INTU_prepare_reject(ic_dlg_id, dlg_ptr);
			return(INTUE_MSG_REJECTED);
		    }
		else
		    {
			fprintf(stderr,"INTU: *** Operation Invoke decoding failed [%i] ***\n", status);
			INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
			return(INTUE_MSG_DECODE_ERROR);
		    }
	    }



	dlg_calldetails_ptr = &(dlg_ptr->call_details);

	//fprintf(stderr, ANSI_COLOR_YELLOW "string CallReferenceNumber as it goes in handle invoke function =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallReferenceNumber);
//	printf("dlg ptr call details ptr in handle invoke function = %p\n", dlg_calldetails_ptr);
	

	//	}
	/*	void push(DLG_CPT_NODE **head, int data) {

	DLG_CPT_NODE *tmp = (DLG_CPT_NODE *) malloc(sizeof(DLG_CPT_NODE));
	tmp->operation = data;
	tmp->next = (*head);
	(*head) = tmp;
	}*/
	// dlg_ptr->cursor = dlg_cpt_ptr->databuf_offset; // TODO - need to move into handle invoke function
	IN_get_operation(dlg_cpt_ptr, &op_code);

	//	fprintf(stderr, ANSI_COLOR_YELLOW "Databuf_Offset(dex) in Handle_Invoke function === %i " ANSI_COLOR_RESET "\n", dlg_cpt_ptr->databuf_offset);
	//fprintf(stderr, ANSI_COLOR_YELLOW "Operation_Code(dec) in Handle_Invoke function === %i " ANSI_COLOR_RESET "\n", op_code);

	//status = IN_get_component_param(INAPPN_invoke_id, &invokeID_len, &invokeID, 1, dlg_cpt_ptr);//dlg_cpt_ptr);
	//printf("invoke id INAPPN in hanlde invoke = %d\n", invokeID);

 //for debug 	for (i=0; i< dlg_cpt_ptr->databuf_offset; i++) {
 //
 //	    printf(" %x ", dlg_cpt_ptr->databuf[i]);
 //	}
	//		CAMELGW_push(&(dlg_ptr->cpt_list_head), 5);
	//		printf("initial head = %p\n", (dlg_ptr->cpt_list_head)->next);
	/*push invoke component data into dialog component list */
	CAMELGW_push(&(dlg_ptr->cpt_list_head), op_code, (dlg_cpt_ptr->databuf_offset) - (dlg_ptr->cursor), &(dlg_cpt_ptr->databuf[0]), dlg_ptr->cursor);
	//TODO! should check component list params as belof but not use printf	
//	printf("operation = %d\n", (dlg_ptr->cpt_list_head)->operation);
	//printf("cpt length = %d\n", (dlg_ptr->cpt_list_head)->databuf_offset);
	//printf("tmp->next = %p\n", (dlg_ptr->cpt_list_head)->next);

	dlg_ptr->cursor = dlg_cpt_ptr->databuf_offset; // сохраняем смещение после получения Invoke в элемент структуры текущего диалога

	/*
	* TODO - should move following code to process_IDP function
	* we should send IDP to logger module 
	* keep in mind that we should pack structure correctly 
	*/

	if (op_code == INOP_InitialDP)
	    {
		if (dlg_cpt_ptr->databuf_offset%4 == 0)
			length = dlg_cpt_ptr->databuf_offset + sizeof(LOG_MSG)-sizeof(long);
		else if (dlg_cpt_ptr->databuf_offset%4 != 0)
			length = 4 + 4*(dlg_cpt_ptr->databuf_offset/4) + 
			sizeof(LOG_MSG)-sizeof(long);

		LOG_MSG *ptr = (LOG_MSG *) memory_buffer;	
		//LOG_MSG log_msg;	
		ptr->msg_type = 1;
		ptr->msg_time = time(NULL);	//get current time
		ptr->msg_dlg_id = ic_dlg_id;
		ptr->msg_databuf_offset = dlg_cpt_ptr->databuf_offset;

		memcpy(&ptr->msg_databuf[0], &dlg_cpt_ptr->databuf[0], dlg_cpt_ptr->databuf_offset);
		sendMessage(ptr, length);	//send full data from structure
		//print full databuf for IDP
		/*
		fprintf(stderr,"Databuf InitialDP === \n");
		for(i=0;i<dlg_cpt_ptr->databuf_offset;i++) {
		fprintf(stderr," %x",dlg_cpt_ptr->databuf[i]);
		}
		fprintf(stderr,"\n");
		*/
	    }

	if (op_code == INOP_InitialDP)
	{
		status = IN_get_component_param (INPN_ServiceKey, &srv_key.len, srv_key.data, sizeof(srv_key.data), dlg_cpt_ptr);
		if (status != IN_SUCCESS)
		{
			INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
			fprintf(stderr, "INTU: *** Operation Invoke processing failed [%i] ***\n", status);
		}
		else
		{
			memset(dlg_calldetails_ptr->ServiceKey, 0, sizeof(dlg_calldetails_ptr->ServiceKey)); 	
			sprintf(dlg_calldetails_ptr->ServiceKey, "%i", srv_key.data[0]);
			dlg_calldetails_ptr->uc_ServiceKey = srv_key.data[0];
			printf("debug: Service Key in HandleInvoke function = %d\n", dlg_calldetails_ptr->uc_ServiceKey);
			if (intu_options & INTU_OPT_TR_SRV_PARAM)
				INTU_disp_param("ServiceKey", INPN_ServiceKey, srv_key.len, srv_key.data);

			/*
			* If service Key does not match,send error MissingCustomerRecord
			*/
		
			if (((srv_key.len == sizeof(example_srv_key)) && 
				(memcmp(example_srv_key, srv_key.data, 
				srv_key.len) == 0))||
				((srv_key.len == sizeof(example_srv_key_2)) && 
				(memcmp(example_srv_key_2, srv_key.data, 
				srv_key.len) == 0))) 
			{
			    INTU_prepare_service_logic(ic_dlg_id, dlg_ptr);
			}
			else
			{
				fprintf(stderr, "INTU: *** Unexpected ServiceKey, sending MissingCustomerRecord ***\n");
				INTU_prepare_error(ic_dlg_id, dlg_ptr, INER_MissingCustomerRecord);
			}
		}
	}

	else if (op_code == INOP_EventReportBCSM)
	{	
		INTU_prepare_service_logic(ic_dlg_id, dlg_ptr);
	}

	//Поймали сообщение ApplyChargingReport	
	else if (op_code == INOP_ApplyChargingReport)
	{
		INTU_prepare_service_logic(ic_dlg_id, dlg_ptr);
	}

	else
	{
		fprintf(stderr, "INTU: *** Unexpected operation [%i]***\n", op_code);
		INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
	}
	return 0;
}


/*
*******************************************************************************
*                                                                             *
* CAMELGW_handle_resultlast: логика на принятый приложением шлюза SRV_IND = RESULT LAST   *
*                                                                             *
*******************************************************************************
*/
int CAMELGW_handle_returnresultlast(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h)
{
    //u16    ic_dlg_id;      // Dialogue id of the incoming dialogue indication 
    //DLG_CB *dlg_ptr;       // Pointer to the per-dialogue id control block 
    //HDR    *h;             // Received message 

	struct calldetails	*dlg_calldetails_ptr;
	SERVICE_KEY_TYPE	srv_key;	// Recovered serv_key of the IDP 
	LOG_MSG	message;

	IN_CPT cpt_test;

	IN_CPT	*dlg_cpt_ptr;			// Recovered INAP Component 
	IN_CPT	*dlg_cpt_ptr_new;		// Recovered INAP Component 
	int invokeID_len;
 unsigned char invokeID;
	int	status;						// Return status fron INAP API func 
	int	length;
	int	i;

	u16	op_code;         			// Opcode of the recovered invoke 


	dlg_cpt_ptr = &(dlg_ptr->cpt);

	IN_init_component(dlg_cpt_ptr->prot_spec,&cpt_test,0);

	
	//status = IN_decode_operation(dlg_cpt_ptr,h);

	//status = IN_decode_result(dlg_cpt_ptr,h);

	status = IN_decode_result(&cpt_test,h);

	if (status != IN_SUCCESS)
	    {
		if (status == IN_REJECT_REQUIRED)
		    {
			fprintf(stderr,"INTU: *** Operation Invoke rejected ***\n");
			INTU_prepare_reject(ic_dlg_id, dlg_ptr);
			return(INTUE_MSG_REJECTED);
		    }
		else
		    {
			fprintf(stderr,"INTU: *** Operation Invoke decoding failed [%i] ***\n", status);
			INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
			return(INTUE_MSG_DECODE_ERROR);
		    }
	    }

	//	status = IN_get_component_param (INPN_InvokeID, &invokeID_len, &invokeID, 1, dlg_cpt_ptr);

	//printf(" result handling : status = %d\n", status);

	//printf("result handling invoke id = %d\n", invokeID);


    			/* printf("result handle first_error = %d\n", dlg_cpt_ptr->first_error);                   /\* API Error code of the first failure  *\/ */
  			/* printf("result handle first_error_reason = %d\n", dlg_cpt_ptr->first_error_reason) ;            /\* Identifier reason of the first error *\/ */
  			/* printf(" operation       = %d\n", dlg_cpt_ptr->operation);                     /\* Operation Code *\/ */
  			/* printf(" timeout         = %d\n", dlg_cpt_ptr->timeout);                      /\* Operation timeout value *\/ */
  			/* printf("  err            = %d\n", dlg_cpt_ptr->err);                          /\* Operation Error Code *\/ */
  			/* printf("  type           = %d\n", dlg_cpt_ptr->type);                         /\* Is it an invoke, result or error *\/ */
  			/* printf("  options        = %d\n", dlg_cpt_ptr->options);                      /\* Options mask *\/ */
  			/* printf("  databuf_offset = %d\n", dlg_cpt_ptr->databuf_offset);               /\* Size of completed databuf *\/ */


    			 printf("test result handle first_error = %d\n", cpt_test.first_error);
  			 printf("result handle first_error_reason = %d\n", cpt_test.first_error_reason);
  			 printf(" operation       = %d\n", cpt_test.operation);
  			 printf(" timeout         = %d\n", cpt_test.timeout);
  			 printf("  err            = %d\n", cpt_test.err);
  			 printf("  type           = %d\n", cpt_test.type);
  			 printf("  options        = %d\n", cpt_test.options);
  			 printf("  databuf_offset = %d\n", cpt_test.databuf_offset);


	/* status = IN_get_component_param (INPN_InvokeID, &invokeID_len, &invokeID, 1, dlg_cpt_ptr); */

	/* printf(" result handling : status = %d\n", status); */

	/* printf("result handling invoke id = %d\n", invokeID); */



	status = IN_get_component_param (INPN_InvokeID, &invokeID_len, &invokeID, 1, &cpt_test);

	printf(" test result handling : status = %d\n", status);

	printf("result handling invoke id = %d\n", invokeID);

CAMELGW_prepare_returnresultlast_logic(ic_dlg_id, dlg_ptr);


	return 0;

	dlg_calldetails_ptr = &(dlg_ptr->call_details);

	

	//	}
	/*	void push(DLG_CPT_NODE **head, int data) {

	DLG_CPT_NODE *tmp = (DLG_CPT_NODE *) malloc(sizeof(DLG_CPT_NODE));
	tmp->operation = data;
	tmp->next = (*head);
	(*head) = tmp;
	}*/
	// dlg_ptr->cursor = dlg_cpt_ptr->databuf_offset; // TODO - need to move into handle invoke function
	IN_get_operation(dlg_cpt_ptr, &op_code);

	//	fprintf(stderr, ANSI_COLOR_YELLOW "Databuf_Offset(dex) in Handle_Invoke function === %i " ANSI_COLOR_RESET "\n", dlg_cpt_ptr->databuf_offset);
	//fprintf(stderr, ANSI_COLOR_YELLOW "Operation_Code(dec) in Handle_Invoke function === %i " ANSI_COLOR_RESET "\n", op_code);

	//status = IN_get_component_param(INAPPN_invoke_id, &invokeID_len, &invokeID, 1, dlg_cpt_ptr);//dlg_cpt_ptr);
	//printf("invoke id INAPPN in hanlde invoke = %d\n", invokeID);

 //for debug 	for (i=0; i< dlg_cpt_ptr->databuf_offset; i++) {
 //
 //	    printf(" %x ", dlg_cpt_ptr->databuf[i]);
 //	}
	//		CAMELGW_push(&(dlg_ptr->cpt_list_head), 5);
	//		printf("initial head = %p\n", (dlg_ptr->cpt_list_head)->next);
	/*push invoke component data into dialog component list */
	CAMELGW_push(&(dlg_ptr->cpt_list_head), op_code, (dlg_cpt_ptr->databuf_offset) - (dlg_ptr->cursor), &(dlg_cpt_ptr->databuf[0]), dlg_ptr->cursor);
	//TODO! should check component list params as belof but not use printf	
//	printf("operation = %d\n", (dlg_ptr->cpt_list_head)->operation);
	//printf("cpt length = %d\n", (dlg_ptr->cpt_list_head)->databuf_offset);
	//printf("tmp->next = %p\n", (dlg_ptr->cpt_list_head)->next);

	dlg_ptr->cursor = dlg_cpt_ptr->databuf_offset; // сохраняем смещение после получения Invoke в элемент структуры текущего диалога

	/*
	* TODO - should move following code to process_IDP function
	* we should send IDP to logger module 
	* keep in mind that we should pack structure correctly 
	*/

	if (op_code == INOP_InitialDP)
	    {
		if (dlg_cpt_ptr->databuf_offset%4 == 0)
			length = dlg_cpt_ptr->databuf_offset + sizeof(LOG_MSG)-sizeof(long);
		else if (dlg_cpt_ptr->databuf_offset%4 != 0)
			length = 4 + 4*(dlg_cpt_ptr->databuf_offset/4) + 
			sizeof(LOG_MSG)-sizeof(long);

		LOG_MSG *ptr = (LOG_MSG *) memory_buffer;	
		//LOG_MSG log_msg;	
		ptr->msg_type = 1;
		ptr->msg_time = time(NULL);	//get current time
		ptr->msg_dlg_id = ic_dlg_id;
		ptr->msg_databuf_offset = dlg_cpt_ptr->databuf_offset;

		memcpy(&ptr->msg_databuf[0], &dlg_cpt_ptr->databuf[0], dlg_cpt_ptr->databuf_offset);
		sendMessage(ptr, length);	//send full data from structure
		//print full databuf for IDP
		/*
		fprintf(stderr,"Databuf InitialDP === \n");
		for(i=0;i<dlg_cpt_ptr->databuf_offset;i++) {
		fprintf(stderr," %x",dlg_cpt_ptr->databuf[i]);
		}
		fprintf(stderr,"\n");
		*/
	    }

	if (op_code == INOP_InitialDP)
	{
		status = IN_get_component_param (INPN_ServiceKey, &srv_key.len, srv_key.data, sizeof(srv_key.data), dlg_cpt_ptr);
		if (status != IN_SUCCESS)
		{
			INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
			fprintf(stderr, "INTU: *** Operation Invoke processing failed [%i] ***\n", status);
		}
		else
		{
			memset(dlg_calldetails_ptr->ServiceKey, 0, sizeof(dlg_calldetails_ptr->ServiceKey)); 	
			sprintf(dlg_calldetails_ptr->ServiceKey, "%i", srv_key.data[0]);
			dlg_calldetails_ptr->uc_ServiceKey = srv_key.data[0];
			printf("debug: Service Key in HandleInvoke function = %d\n", dlg_calldetails_ptr->uc_ServiceKey);
			if (intu_options & INTU_OPT_TR_SRV_PARAM)
				INTU_disp_param("ServiceKey", INPN_ServiceKey, srv_key.len, srv_key.data);

			/*
			* If service Key does not match,send error MissingCustomerRecord
			*/
		
			if (((srv_key.len == sizeof(example_srv_key)) && 
				(memcmp(example_srv_key, srv_key.data, 
				srv_key.len) == 0))||
				((srv_key.len == sizeof(example_srv_key_2)) && 
				(memcmp(example_srv_key_2, srv_key.data, 
				srv_key.len) == 0))) 
			{
			    INTU_prepare_service_logic(ic_dlg_id, dlg_ptr);
			}
			else
			{
				fprintf(stderr, "INTU: *** Unexpected ServiceKey, sending MissingCustomerRecord ***\n");
				INTU_prepare_error(ic_dlg_id, dlg_ptr, INER_MissingCustomerRecord);
			}
		}
	}

	else if (op_code == INOP_EventReportBCSM)
	{	
		INTU_prepare_service_logic(ic_dlg_id, dlg_ptr);
	}

	//Поймали сообщение ApplyChargingReport	
	else if (op_code == INOP_ApplyChargingReport)
	{
		INTU_prepare_service_logic(ic_dlg_id, dlg_ptr);
	}

	else
	{
		fprintf(stderr, "INTU: *** Unexpected operation [%i]***\n", op_code);
		INTU_prepare_u_abort(ic_dlg_id, dlg_ptr);
	}
	return 0;
}

/*
*******************************************************************************
*                                                                             *
*   INTU_prepare_service_logic: Mark the reply prepared as invoke	  	  *
*                                                                             *
*******************************************************************************
*/
/*
* All the info we need is already decoded into the dlg->cpt structure
* so just change state and wait for the delimit.
*
* Returns 0
*/
int CAMELGW_prepare_returnresultlast_logic(u32 ic_dlg_id, DLG_CB *dlg_ptr)
{
    //u16    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
    //DLG_CB *dlg_ptr;       /* Pointer to the per-dialogue id control block */
    //{
	u16 op_code;
	
	//	printf("INTU_prepare_service_logic function call\n");

	//	IN_get_operation(&(dlg_ptr->cpt), &op_code);

	//switch(op_code){
	//case 0 :

		 dlg_ptr->reply_prepared = CAMELGW_REPLY_RETURNRESULTLAST;
		 //INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
		 //	 break;
		 //case 24 :
	
		 //		 dlg_ptr->reply_prepared = INTU_REPLY_EVENTREPORT;
		 //dlg_ptr->reply_prepared = INTU_REPLY_INVOKE;
		 //INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
		 // break;
		 //case 36 :
	
		 //dlg_ptr->reply_prepared = INTU_REPLY_U_APPLYCHARGINGREPORT;
		 //dlg_ptr->reply_prepared = INTU_REPLY_INVOKE;
		 //INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
		 // break;
		 //	default: break; //TODO!
		 //	}
	if (dlg_ptr->state == INTU_STATE_OPEN )
	    INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
	if (dlg_ptr->state == INTU_STATE_CLOSING )
	    INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);

	//	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
	return 0;
}




/*
*******************************************************************************
*                                                                             *
*   INTU_prepare_service_logic: Mark the reply prepared as invoke	  	  *
*                                                                             *
*******************************************************************************
*/
/*
* All the info we need is already decoded into the dlg->cpt structure
* so just change state and wait for the delimit.
*
* Returns 0
*/
int INTU_prepare_service_logic(u32 ic_dlg_id, DLG_CB *dlg_ptr)
{
    //u16    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
    //DLG_CB *dlg_ptr;       /* Pointer to the per-dialogue id control block */
    //{
	u16 op_code;
	
	//	printf("INTU_prepare_service_logic function call\n");

	IN_get_operation(&(dlg_ptr->cpt), &op_code);

	switch(op_code){
	 case 0 :

		 dlg_ptr->reply_prepared = INTU_REPLY_INVOKE;
		 //INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
		 break;
	 case 24 :
	
		 //		 dlg_ptr->reply_prepared = INTU_REPLY_EVENTREPORT;
		 dlg_ptr->reply_prepared = INTU_REPLY_INVOKE;
		 //INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
		 break;
	 case 36 :
	
		 //dlg_ptr->reply_prepared = INTU_REPLY_U_APPLYCHARGINGREPORT;
		 dlg_ptr->reply_prepared = INTU_REPLY_INVOKE;
		 //INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
		 break;
	default: break; //TODO!
	}
	if (dlg_ptr->state == INTU_STATE_OPEN )
	    INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
	if (dlg_ptr->state == INTU_STATE_CLOSING )
	    INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);

	//	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
	return 0;
}

/*
*******************************************************************************
*                                                                             *
* 		INTU_prepare_error  				 	      *
*                              	                                          *
*******************************************************************************
*/
/*
* Mark the reply prepared as error
*
* Returns 0
*/
int INTU_prepare_error(ic_dlg_id, dlg_ptr, err_code)
u32    ic_dlg_id;      // Dialogue id of the incoming dialogue indication
DLG_CB *dlg_ptr;       // Pointer to the per-dialogue id control block 
u16    err_code;       // Error code being prepared 
{
	IN_set_error(dlg_ptr->cpt.operation, err_code, &(dlg_ptr->cpt));
	dlg_ptr->reply_prepared = INTU_REPLY_ERROR;
	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
	return 0;
}

/*
*******************************************************************************
*                                                                             *
* 		INTU_prepare_reject  				 	      *
*                               	                                          *
*******************************************************************************
*/
/*
* The decode operation_invoke stored the problem code into the cpt
* structure so we have all the info we need.
*
* Mark the reply prepared as reject
*
* Returns 0
*/
int INTU_prepare_reject(ic_dlg_id, dlg_ptr)
u32    ic_dlg_id;      // Dialogue id of the incoming dialogue indication 
DLG_CB *dlg_ptr;       // Pointer to the per-dialogue id control block 
{
	dlg_ptr->reply_prepared = INTU_REPLY_REJECT;
	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
	return 0;
}

/*
*******************************************************************************
*                                                                             *
* 		INTU_prepare_u_abort  				 	      *
*                              	                                          *
*******************************************************************************
*/
/*
*
* Mark the reply prepared as u_abort
*
* Returns 0
*/
int INTU_prepare_u_abort(ic_dlg_id, dlg_ptr)
u32    ic_dlg_id;      // Dialogue id of the incoming dialogue indication 
DLG_CB *dlg_ptr;       // Pointer to the per-dialogue id control block 
{
	dlg_ptr->reply_prepared = INTU_REPLY_U_ABORT;
	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_PENDING_DELIMIT);
	return 0;
}

/*
*******************************************************************************
*                                                                             *
* 	INTU_process_pending_req  				 	      *
*                              	                                          *
*******************************************************************************
*/
/*
* We should have a reply waiting to be sent or service logic waiting to be
* processed. So check for the reply stored in the DLG_CB and process it.
*
* Returns 0
*/
int INTU_process_pending_req(ic_dlg_id, dlg_ptr, h)
u32    ic_dlg_id;      // Dialogue id of the incoming dialogue indication 
DLG_CB *dlg_ptr;       // Pointer to the per-dialogue id control block 
HDR    *h;             // Received message 
{
	u8 reply;              // Reply prepared before processing 

	reply = dlg_ptr->reply_prepared;

	/*
	* Reset the reply_prepared so we know we've tried to process it once.
	*/
	dlg_ptr->reply_prepared = INTU_REPLY_NONE;
	switch (reply)
{
	case INTU_REPLY_INVOKE:
	    /* call function to process invokes list after received DELIMITER */
	    INTU_process_invokes(ic_dlg_id, dlg_ptr, h);
		return 0;

	case INTU_REPLY_EVENTREPORT:
		INTU_process_invokes(ic_dlg_id, dlg_ptr, h);

		return 0;

		//	case INTU_REPLY_U_APPLYCHARGINGREPORT:
		//		INTU_process_ApplyChargingReport(ID,ic_dlg_id, dlg_ptr, h);
		//return 0;

	case INTU_REPLY_ERROR:
		INTU_send_open_rsp(ic_dlg_id);
		INTU_send_error(ic_dlg_id, dlg_ptr);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		break;

	case INTU_REPLY_REJECT:
		INTU_send_open_rsp(ic_dlg_id);
		INTU_send_reject(ic_dlg_id, dlg_ptr);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		break;

	case INTU_REPLY_U_ABORT:
		INTU_send_u_abort(ic_dlg_id);
		break;

	case INTU_REPLY_NONE:
		break;

 case CAMELGW_REPLY_RETURNRESULTLAST:
     //do nothing when receive return result last!
     return 0;
     break;

	default:
		break;
	}

	/*
	* For all replies except invoke processing close the dialogue.
	*/
	INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
	return 0;
}
/*
*******************************************************************************
*                                                                             *
* INTU_process_invoke_IDP: подготовка ответа на IDP, принятный шлюзом от MSC  *
*                                                                             *
*******************************************************************************
*/

/*
* If the invokeID in the previously decoded service indication is invalid
* an UnexpectedDataValue error is sent back to the remote end. For valid
* invokeIDs either a Connect or a ReleaseCall operation is sent back depending
* on the status returned by the number translation (Connect on success).
* 
* если прилетает несколько invokes то они складываются в односвязный список 
* и функция process_invokes запускается только после получения DELIMIT от модуля INAP
* 
* Returns 0 on success
*         INTUE_INVALID_INVOKE_ID
*         INTUE_MSG_ALLOC_FAILED
*         INTUE_MSG_DECODE_ERROR
*         INTUE_CODING_FAILURE
*/ 
int INTU_process_invokes(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h)
{
    //u16    ic_dlg_id;	// Dialogue id of the incoming dialogue indication 
    //DLG_CB *dlg_ptr;	// Pointer to the per-dialogue id control block 
    //HDR    *h;			// Received message 

	IN_CPT	buffer_cpt; /* буфферная структура для компоненты из Invoke */
	PLEN invokeID_len;
	u8 invokeID;
IN_CPT	*dlg_cpt_ptr;
 int status;

 //	printf("INTU_process_invokes function call\n");
	
	//	INTU_send_open_rsp(ic_dlg_id);
	//	fprintf(stderr, "INTU: *** отправил OPEN_RESPONSE  в модуль INAP ***\n");
	dlg_cpt_ptr = &(dlg_ptr->cpt); 

	//#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&buffer_cpt,0);
	//#else
	//IN_init_component(dlg_cpt_ptr->prot_spec,&buffer_cpt);
	//#endif

	status = IN_get_component_param(INPN_InvokeID, &invokeID_len, &invokeID, 1, dlg_cpt_ptr);//buffer_cpt_ptr);//dlg_cpt_ptr);
	if (status == IN_SUCCESS)
	    printf("invoke id in process invokes function = %d\n", invokeID);
	else
	    ("error:status in process invokes function = %d\n", status);


	if (CAMELGW_list_length(&(dlg_ptr->cpt_list_head)) == 1) //если в списке только одна компонента, то обрабатываем только ее 
	{
		//init of buffer component in case when only one node in components list
		buffer_cpt.operation  = (dlg_ptr->cpt_list_head)->operation;
		buffer_cpt.databuf_offset  = (dlg_ptr->cpt_list_head)->databuf_offset;
		buffer_cpt.prot_spec = ((dlg_ptr->cpt).prot_spec);
		 int i;
				for(i=0;i<(buffer_cpt.databuf_offset); i++)
				    {
					buffer_cpt.databuf[i] = (dlg_ptr->cpt_list_head)->databuf[i];
				    }
		//	fprintf(stderr, ANSI_COLOR_YELLOW "Type of message in process IDP function  op(dec) %i" ANSI_COLOR_RESET "\n", h->type);
		//printf("!!!!!!!!!!!!!!!!!!!!!!  buffer cpt  operation  = %i\n", buffer_cpt.operation);
		//printf("!!!!!!!!!!!!!!!!!!!!!!  buffer cpt  databuf_offset  = %i\n", buffer_cpt.databuf_offset);

		//for(i=0;i<(buffer_cpt.databuf_offset); i++) {
		//printf("buffer cpt  databuf[%i]  =====  %x\n", i, buffer_cpt.databuf[i]); 
		//} 
		//select invoke handling function depending on operation code in single invoke

		if (buffer_cpt.operation == INOP_InitialDP) {

		    INTU_send_open_rsp(ic_dlg_id);
		    //fprintf(stderr, "INTU: *** отправил OPEN_RESPONSE  в модуль INAP после получения IDP ***\n");
			camelgw_handle_idp( ic_dlg_id, dlg_ptr, h, &buffer_cpt);
			//purge component list
			printf("debug: after handle idp\n");
			free(dlg_ptr->cpt_list_head);
			dlg_ptr->cpt_list_head=NULL;
			return 0;
		}
		else if (buffer_cpt.operation == INOP_EventReportBCSM)  {
			//printf("попадаю в ветку обработки single incoming invoke ====eventBCSMreport!!!\n");
			camelgw_handle_erb(ic_dlg_id, dlg_ptr, h, &buffer_cpt);
			free(dlg_ptr->cpt_list_head);
			dlg_ptr->cpt_list_head=NULL;
			return 0;
		}
		else if (buffer_cpt.operation == INOP_ApplyChargingReport)  {
			//printf("попадаю в ветку обработки single incoming invoke ==== applychargingreport!!!\n");
		    //debug	    for(i=0;i<(buffer_cpt.databuf_offset); i++) {
		    //debug printf("buffer cpt  databuf[%i]  =====  %x\n", i, buffer_cpt.databuf[i]); 
		    //debug } 
	
		    //old with modes		    camelgw_handle_acr(ic_dlg_id, dlg_ptr, h, &buffer_cpt, SINGLE);
		    camelgw_handle_acr(ic_dlg_id, dlg_ptr, h, &buffer_cpt);

			//INTU_handle_eventreportbcsm(ic_dlg_id, dlg_ptr, h, &buffer_cpt);
			free(dlg_ptr->cpt_list_head);
			dlg_ptr->cpt_list_head=NULL;
			return 0;
		}
	}
	else if (CAMELGW_list_length(&(dlg_ptr->cpt_list_head)) == 2 ) {
		//printf("!!!!!!!!Две компоненты в сообщении на входе!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		buffer_cpt.prot_spec = ((dlg_ptr->cpt).prot_spec); // buffer cpt init 
		INTU_handle_cpt_list(ic_dlg_id, dlg_ptr, h, &buffer_cpt);
		free(dlg_ptr->cpt_list_head);
		dlg_ptr->cpt_list_head=NULL;
		return 0;
	}
	else {
		printf("количество компонент ни 1 и ни 2!!!!\n");
		/* TODO - write to log file */		
		return 0;
	}
	//dlg_cpt_ptr = &(dlg_ptr->cpt); 
	// dlg_ptr->cursor = dlg_cpt_ptr->databuf_offset; // TODO - need to move into handle invoke function
	//	IN_get_component_param(INPN_InvokeID, &invokeID_len, &invokeID, 1, &buffer_cpt);
	return 0;
}
/*
*******************************************************************************
*                                                                             *
* INTU_handle_cpt_list                                                        *
*                                                                             *
*******************************************************************************
*/
/* backend process for handling several ivokes in one signalling message from MSC 
* в одном сигнальном сообщении от коммутатора может содержаться несколько Invoke
* в этом случаем складываем их всех в один список для одного диалога и обрабатываем списком в этой функции
* предполагается, что в списке могут быть только две компоненты! 
*/
 int INTU_handle_cpt_list(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, IN_CPT *buffer_cpt_ptr) {
     //u16	ic_dlg_id;	    // Dialogue id of the incoming dialogue indication
     //DLG_CB	*dlg_ptr;	// Pointer to the per-dialogue id control block
     //HDR *h;			    // Received message
     //IN_CPT *buffer_cpt_ptr;

	IN_CPT	buffer_cpt;
	IN_CPT	*ptr;

	//	printf("Длина списка компонент в списке invokes  = %d\n", CAMELGW_list_length(&(dlg_ptr->cpt_list_head)));
	//printf("size of buffer cpt = %i\n", buffer_cpt.databuf_offset); 
	// printf("buffer cpt pointer 1 = %p\n", &buffer_cpt);
	// buffer_cpt_ptr = &buffer_cpt;

	CAMELGW_pop( &(dlg_ptr->cpt_list_head), buffer_cpt_ptr ); // head of list put into buffer component - выбрали головную компоненту из списка для анализа

	//  printf("buffer cpt pointer 2 = %p\n", buffer_cpt_ptr);

	switch ( buffer_cpt_ptr->operation) 
	{
	case INOP_EventReportBCSM: /* intercept another INVOKES from MSC - ACR, RRBEvent and other in PENDING_DELIMIT_STATE  */
		//INTU_handle_invoke(ic_dlg_id, dlg_ptr, h);
		//	printf(" Голова списка  = event report bcsm \n");
	    camelgw_handle_erb(ic_dlg_id, dlg_ptr, h, buffer_cpt_ptr);
		break;
	case INOP_ApplyChargingReport: /* intercept another INVOKES from MSC - ACR, RRBEvent and other in PENDING_DELIMIT_STATE  */
		//INTU_handle_invoke(ic_dlg_id, dlg_ptr, h);
		//	printf(" Голова списка  = apply charging report\n");
	    camelgw_handle_acr(ic_dlg_id, dlg_ptr, h, buffer_cpt_ptr);
		break;
	default:
		printf(" Голова списка =неведомая сущность\n");
		break;
	}
	CAMELGW_pop( &(dlg_ptr->cpt_list_head), buffer_cpt_ptr); // выбрали вторую  компоненту из списка для анализа
	switch ( buffer_cpt_ptr->operation) 
	{
	case INOP_EventReportBCSM: // intercept another INVOKES from MSC - ACR, RRBEvent and other in PENDING_DELIMIT_STATE
		//	printf(" Вторая компонента в списке  = event report bcsm \n");
	    camelgw_handle_erb(ic_dlg_id, dlg_ptr, h, buffer_cpt_ptr);
		break;
	case INOP_ApplyChargingReport: /* intercept another INVOKES from MSC - ACR, RRBEvent and other in PENDING_DELIMIT_STATE  */
		//	printf(" Вторая компонента в списке  = apply charging report\n");
	    camelgw_handle_acr(ic_dlg_id, dlg_ptr, h, buffer_cpt_ptr);
		break;
	default:
		printf(" Вторая компонента в списке  = неведомая сущность\n");
		break;
	}
	//printf(" Значение  operation второго элемента  списка = %d\n", CAMELGW_pop( &(dlg_ptr->cpt_list_head)));
	//printf("Длина списка компонент в списке invokes after pop  = %d\n", CAMELGW_list_length(&(dlg_ptr->cpt_list_head)));
	return 0;
}


/* commented 28.08.17 (Vladimir)
// signal handler 
void sig_handler (int signum, siginfo_t *siginfo, void *ucontext) //{
		     //struct temp_file *p;
		     //for (p = temp_file_list; p; p = p->next)
		     //printf("signal received!\n");
    //		     printf("signal %d received. code = %d, payload = %d\n", siginfo->si_signo, siginfo->si_code, siginfo->si_value.sival_int);
  {
		     //struct temp_file *p;
		     //for (p = temp_file_list; p; p = p->next)
		     //printf("signal received!\n");
      //      struct My_Data *ptr =  (struct  My_Data *) siginfo->si_value.sival_ptr;
	    if (pthread_spin_lock(&spin) == 0)
		{
		printf("spin locked in signal handler!\n");
		//printf("dead dialog = %d\n", siginfo->Dead_Dialogue);
		Global = siginfo->si_value.sival_int;
		pthread_spin_unlock(&spin);
		}
	else
	perror("spinlock");

	    //printf("signal %d received. code = %d, payload = %d\n", siginfo->si_signo, siginfo->si_code, siginfo->si_value.sival_int);
      //printf("signal %d received. code = %d, spinlock pointer = %p\n", siginfo->si_signo, siginfo->si_code, siginfo->si_value.sival_ptr);
		     
                 }
               		     
  // }
                
static void* thread_func(void *ptr) {
    int sig;
    sigset_t sigset;
  struct sigaction sa;
    
    sigemptyset(&sa.sa_mask);
    //sigaddset(&sigset, SIGUSR1);

		     sa.sa_handler = sig_handler;
		     // sigemptyset (&new_action.sa_mask);
		     sa.sa_flags = SA_SIGINFO;

		     sigaction (SIGRTMIN, &sa, NULL);

		     //		     struct sigaction sa;

		     //sigemptyset(&sa.sa_mask);
		     //
		     //sa.sa_flags = 0;
		     //sa.sa_handler = catch;

		     //sigaction(SIGINT, &sa, NULL);
		     //sigaction(SIGUSR1, &sa, NULL);



		     while(1)
			 ;
		     //    while(!sigwait(&sigset, &sig)) {
		     // printf("signal thread: tick\n");
		     //	printf("sigvalue = %d\n", sig->sigval);
		     //   }
}

static void* thread_func2(void *args) {


    struct sigaction sig_action; // Structure describing the action to be taken when a signal arrives.  

    sigset_t oldmask;  // Signal mask before signal disposition change.      
    sigset_t newmask;  // Signal mask after signal disposition change.       
    sigset_t zeromask; // Signal mask to unblock all signal while suspended.

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGRTMIN);

 
    // Examine and change blocked signals. 
    pthread_sigmask(SIG_UNBLOCK, &newmask, &oldmask);
    // Define signal mask and install signal handlers 
    memset(&sig_action, 0, sizeof(struct sigaction));

    sig_action.sa_flags = SA_SIGINFO;
    sig_action.sa_sigaction = sig_handler;
    sig_action.sa_mask = newmask;
    // Examine and change a signal action. 
    sigaction(SIGRTMIN, &sig_action, NULL);
    //sigaction(SIGINT, &sig_action, NULL);

        while(1)
    	;
}
*/
