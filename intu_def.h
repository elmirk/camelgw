/*
Copyright (C) Dialogic Corporation 1998-2012. All Rights Reserved.
* 
Name:    intu_def.h

Description:  INAP Test Utility example application program header
using the Dialogic INAP binary module and API interface.

Functions:  in_ent

-----   ---------  -----     ------------------------------------
Issue    Date       By        Changes
-----   ---------  -----     ------------------------------------
A     22-Dec-98   JET      - Initial code.
B     02-Mar-99   JET      - Updated to use current defintions in INAP API
1     09-Sep-04   GNK      - Move static declaration to intumain.c.
- Update copyright owner & year
-     20-Jul-06   JTD      - Updates to support INAPAPI DLL
2     13-Dec-06   ML       - Change to use of Dialogic Corporation copyright.
3     01-Dec-09   JLP      - Updated description
4     29-Mar-10   JLP      - Change plen from u8 to PLEN
06-Sep-12   MH       - Change DEFAULT_BASE_DIALOGUE_ID to 0x8000 (0x0000).
*/

/*
* Standard include files
*/
#pragma once

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <error.h>
#include <errno.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>

#include "camelgw.h"


/*
* General System7 include files
*/
#include "system.h"
#include "msg.h"
#include "sysgct.h"
#include "digpack.h"
#include "bit2byte.h"
#include "asciibin.h"
#include "strtonum.h"
#include "ss7_inc.h"
#include "tcp_inc.h"
#include "scp_inc.h"

//included in exact source which works with erlang
//#include <erl_interface.h>
//#include <ei.h>


/*
* INAP specific include files
*/
#include "inap_inc.h"
#include "in_inc.h"

//for htonll function
#define TYP_INIT 0 
#define TYP_SMLE 1 
#define TYP_BIGE 2 


/*
* Command Line Options
*
* -m  : intu module_id - The task id of this module
*
* -i  : inap module_id - The task id of the INAP binary module
*
* -n  : number of dialogues - The number of dialogues to support
*       This may be any number less than DEFAULT_NUM_DIALOGUES but
*       by default all dialogues are available.
*
* -b  : base dialogue ID - The base number of the supported dialogue IDs
*       Supported range from (base) upto (base + num_dlgs)
*
* -o  : tracing options - Defines which tracing options are configured
*
*/

#define LOC_NUM_MAX_DIGITS_STR 33 //33 digits + 1 for null term
#define IMSI_MAX_DIGITS_STR 16

/* Redirection reasons */

#define UNCONDITIONAL 3


/* service drivers */

#define PREPAID_DRIVER 0
#define VPN_DRIVER 1
#define M2M_DRIVER 2


/* prepaid service */

#define CAMELGW_ORACLE_INITIAL_QUOTA_REQ 0
#define CAMELGW_ORACLE_CALL_END_IND 1
#define CAMELGW_ORACLE_NEXT_QUOTA_REQ 2

/*m2m service */
#define CAMELGW_M2M_QUOTA_REQ 0
#define CAMELGW_M2M_CALL_END_IND 1



/* VPN service return codes */

#define VPN_CONNECT 7
#define VPN_RELEASE 8
#define VPN_CONTINUE 9
#define VPN_CONTINUE_NOK 10
#define VPN_TIMEOUT 11


/*index of return stack from Lua function call */
#define IP 2   /*return second value from stack*/
#define REGIME 1 /*return first value from stack*/

//TEST mode of application, need data from irbis for local calls
//#define TEST

/*
* Default values for INTU command line options:
*/
#define DEFAULT_MODULE_ID        (APP3_TASK_ID)
#define DEFAULT_INAP_MODULE_ID   (INAP_TASK_ID)
#define DEFAULT_BASE_DIALOGUE_ID (0x8000)
#define DEFAULT_OPTIONS          (0x0300)
#define MONITOR_MODULE_ID        (0xed)
/*
* The following definitions may be changed by the user to
* allow the module to handle more resources:
* The number of dialogues intu can use is limited by the number of
* dialogues available in the INAP binary.
*/
//#define DEFAULT_NUM_DIALOGUES (2048)  /* Max number of dialogue resources */

/*
 * The following definitions may be changed by the user to
 * allow the module to handle more resources:
 * The number of dialogues intu can use is limited by the number of
 * dialogues available in the INAP binary.
 */
#define MAX_NUM_DIALOGUES (2048)  /* Max number of dialogue resources */


/*
* Command Line Options: Trace configuration codes:
*
* Add the codes together for combinations,
* e.g. -o0x020a   Traces the dialogue status count and all dialogue and
*                 service indication messages.
*/
#define INTU_OPT_TR_DLG_REQ   (0x0001) /* Trace transmitted dialogue msgs */
#define INTU_OPT_TR_DLG_IND   (0x0002) /* Trace received dialogue msgs */
#define INTU_OPT_TR_SRV_REQ   (0x0004) /* Trace transmitted service req msgs */
#define INTU_OPT_TR_SRV_IND   (0x0008) /* Trace received service ind msgs */
#define INTU_OPT_TR_DLG_PARAM (0x0010) /* Include dialogue params in trace */
#define INTU_OPT_TR_SRV_PARAM (0x0020) /* Include service req params in trace */
#define INTU_OPT_TR_STATE     (0x0100) /* Trace state changes */
#define INTU_OPT_TR_ACTV_DLG  (0x0200) /* Trace the dialogue status count */
#define INTU_OPT_TR_NUM_TRANS (0x0400) /* Show the numbers translated to/from */

/*
 * Command Line Options (non-trace)
 */
#define INTU_OPT_EXT_DLG      (0x8000) /* Use Extended Dialog ID encoding and decoding for INAP messages */



/*
* Used by intumain.c to return the status of command line option recovery
*/
#define COMMAND_LINE_EXIT_REQ          (-1) /* Option requires immediate exit */
#define COMMAND_LINE_UNRECON_OPTION    (-2) /* Unrecognised option */
#define COMMAND_LINE_RANGE_ERR         (-3) /* Option value is out of range */
#define COMMAND_LINE_REMOTE_CONNECTION (-4) /* Option for remote connection to intu process */

/*
* INTU state definitions
*/
#define INTU_STATE_IDLE              (0)  /* Dialogue waiting for OPEN */
#define INTU_STATE_OPEN              (1)  /* Dialogue opened*/
#define INTU_STATE_PENDING_DELIMIT   (2)  /* Waiting for DELIMIT before */
/* processing Serive Logic or reply */
#define INTU_STATE_CLOSING           (3)  /* Waiting for CLOSE indication */

/*
* Completed dialogue status - did the dialogue progress as expected and
* complete the number translation.
*/
#define INTU_DLG_SUCCESS             (0)
#define INTU_DLG_FAILED              (1)
#define INTU_DLG_TIMEOUT             (2)  //timeout indication received from TCAP module

/*
* INTU reply identifier
*/
#define INTU_REPLY_NONE              (0)
#define INTU_REPLY_INVOKE            (1)
#define INTU_REPLY_ERROR             (2)
#define INTU_REPLY_REJECT            (3)
#define INTU_REPLY_U_ABORT           (4)
#define INTU_REPLY_EVENTREPORT       (5)
#define INTU_REPLY_U_APPLYCHARGINGREPORT (6)
#define CAMELGW_REPLY_RETURNRESULTLAST (7)


/*
* INTU error return codes
*/
#define INTUE_INVALID_DLG_ID         (-1)
#define INTUE_MSG_DECODE_ERROR       (-2)
#define INTUE_MSG_REJECTED           (-3)
#define INTUE_INVALID_DLG_STATE      (-4)
#define INTUE_INVALID_DLG_PTR        (-5)
#define INTUE_INVALID_APPLIC_CONTEXT (-6)
#define INTUE_INIT_CPT_FAILED        (-7)
#define INTUE_INVALID_INVOKE_ID      (-8)
#define INTUE_MSG_ALLOC_FAILED       (-9)
#define INTUE_CODING_FAILURE         (-10)
#define INTUE_MSG_SEND_FAILED        (-11)
#define INTUE_NUM_TRANSLATE_FAILED   (-12)
#define INTUE_NUM_RECOVERY_FAILED    (-13)

/*
* Defined values to aid the building of operations
*/
#define INTU_SIZEOF_DIGIT_DATA              (10)
#define INTU_NUMBER_OF_DIGITS               (INTU_SIZEOF_DIGIT_DATA * 2)
#define INTU_min_called_party_num_len       (4)
#define INTU_max_called_party_num_len       (11)
#define INTU_min_calling_party_num_len       (4)
#define INTU_max_calling_party_num_len       (11)

#define INTU_Connect_timeout                (4)
#define INTU_Continue_timeout               (4)  //because Continue is class 4 this timeout is not used, i think
#define INTU_ReleaseCall_timeout            (10)
//#define INTU_ReleaseCall_timeout            (3)
#define INTU_RequestReportBCSMEvent_timeout (10)
#define INTU_ApplyCharging_timeout 3

//#define INTU_cause_value_normal_unspecified (31)
//#define INTU_cause_value_user_busy (17)
//#define INTU_cause_value_subscriber_absent (20)
//#define INTU_cause_call_rejected (21)
//#define INTU_cause_call_bearer_capability_not_authorized (57)


#define MAX_PARAM_LEN        (320)

#define MAX_NON_EXT_DID                     (0xffff)    /* Maximum dialog ID when not using extended DIDs */


#define SINGLE 1
#define MULTIPLE 2



/* eventTypeBCSM monitor modes */

#define interrupted         (0)
#define notifyAndContinue   (1)

/*event types */

#define eventTypeBCSM_RouteSelectFailure              (4)
#define eventTypeBCSM_OCalledPartyBusy                (5)
#define eventTypeBCSM_ONoAnswer                       (6)
#define eventTypeBCSM_OAnswer                         (7)
#define eventTypeBCSM_ODisconnect                     (9)
#define eventTypeBCSM_OAbandon                        (10)
#define eventTypeBCSM_TBusy                           (13)
#define eventTypeBCSM_TNoAnswer                       (14) 
#define eventTypeBCSM_TAnswer                         (15) 
#define eventTypeBCSM_TDisconnect                     (17)
#define eventTypeBCSM_TAbandon                        (18) 

/*sendingsideID */
#define Leg_01                 (1)
#define Leg_02                 (2)

/*for quering quotas from irbis */

#define QUERY_DURATION 3   //ACR wieh leg=false received
#define QUERY_REPETITION_QUOTA 2
#define QUERY_CALL_DETAILS 1

/* modes for main processing logic by INTU module */

#define MODE_CONTINUE_ONLY 1 
#define MODE_PREPAID 2
#define MODE_SIP_TERMINATION 3

#define AplyChar	35
#define ReleaseCall	41
#define Continue	31

#define NATIONAL_NUMBER 3
#define INTERNATIONAL_NUMBER 4
#define REGIME  1 


/*call control mechanism when duration exceeded */
#define CONTINUE 0
#define RELEASE 1



#define M2M_RESPONSE_REQNEXTQUOTA 0
#define M2M_RESPONSE_LASTQUOTA 1 /*camelgw shouldnt request next quota */
#define M2M_RESPONSE_RELEASECALL 2
#define M2M_RESPONSE_DEFAULTACTION 3
#define M2M_RESPONSE_OUTOFORDER_DUE_MAINT 4



#define PSP_ATTR_SUCCESS_CODE 0x0000
#define PSP_ATTR_ERROR_DESC 0x0001
#define PSP_ATTR_QUOTA_RESULT_FLAG 0x0002
#define PSP_ATTR_QUOTA_SECONDS 0x0003

#define PSP_QUOTA_SUCCESS 0
#define M2M_SUCCESS 0



//#define eventTypeBCSM_oNoAnswer 6
//#define eventTypeBCSM_oAbandon 10 /*TODO - need to process in eventreport function */



/*
* Parameter length storage. If IN_LMSGS are used then the length is a u16
* otherwise it is a u8.
*/
//#ifdef IN_LMSGS
typedef u16 PLEN;
//#else
//typedef u8  PLEN;
//#endif

/*
* Example User operation parameter types
*/
typedef struct service_key_type
{
	PLEN len;
	u8   data[4];
} SERVICE_KEY_TYPE;

/*
* Used by paramters:
*   Called party number
*   Original Called Party Number
*   Destination Routing Address
*/

typedef struct pty_addr
{
	u8    noai;           /* Nature of address indicator */
	u8    inni;           /* Internal network number indicator */
	u8    npi;            /* Numbering plan indicator */
	u8    ni;             /* Number incomplete indicator */
	u8    pri;            /* Presentation restricted indicator */
	u8    si;             /* Screening indicator */
	u8    naddr;          /* number of address digits */
	u8    addr[INTU_SIZEOF_DIGIT_DATA];
} PTY_ADDR;

/*structure used for CalledPartyBCDnumber, used when MO call with DP2 */

struct bcd_called_addr {
	u8    ext;           /* extension: 1=no_extension */
	u8    ton;           /* type of number, 0001b - international number */
	u8    npi;            /* Numbering plan indicator 0001b ISDN E164 */
	u8    naddr;          /* number of address digits , this is our element not exist in standart calledBCDnumber*/
	u8    addr[15];
};

/*called number types */
//BCD_CALLED - in MO calls
//CALLED - in MT calls or in MO leg when CF call

#define BCD_CALLED 2
#define CALLED 12



/* struct called_pty_addr { */
/*     unsigned char num_type; //called, called_bcd, or calling? or another? */
/* 	unsigned char    bcd_exti;           /\* extension: 1=no_extension *\/ */
/* 	unsigned char    bcd_toni;           /\* type of number, 0001b - international number *\/ */
/* 	unsigned char    both_npi;            /\* Numbering plan indicator 0001b ISDN E164 *\/ */
/*     unsigned char oddi; //odd-even indicator in calledpartynumber */
/*     unsigned char noai; //nature of address indicator, used in CalledParty, DP12 */
/*     unsigned char inni; */
/*     unsigned char    naddr;          /\* number of address digits , this is our element not exist in standart calledBCDnumber*\/ */
/*     unsigned char    addr[15]; //array of unpacked address digits */
/* }; */


/* struct calling_pty_addr { */
/*     u8 addr_type; //called, called_bcd, or calling? or another? */
/*     u8 oddi; */
/*     u8 noa;  */
/*     u8 ni;            /\* Number incomplete indicator, 0 = complete *\/ */
/*     u8 npi;  */
/*     u8 apri; //adress presentation restricted indicator */
/*     u8 si; //screening indicator */
/*     u8 naddr;          /\* number of address digits , this is our element not exist in standart calledBCDnumber*\/ */
/*     u8 addr[15]; //mixed pairs of digits, not unpacked yet */
/* }; */

//camelgw.h
/* struct calling_pty_addr { */
/*     unsigned char num_type; //called, called_bcd, or calling? or another? */
/*     unsigned char oddi; */
/*     unsigned char noai;//nature of address indicator  */
/*     unsigned char ni;            /\* Number incomplete indicator, 0 = complete *\/ */
/*     unsigned char npi;      //numbering plan indicator  */
/*     unsigned char apri;     //adress presentation restricted indicator */
/*     unsigned char si;       //screening indicator */
/*     unsigned char naddr;     /\* number of address digits , this is our element not exist in standart calledBCDnumber*\/ */
/*     unsigned char addr[15]; //mixed pairs of digits, not unpacked yet */
/* }; */


//camelgw.h
/* struct redirecting_pty_addr { */
/*         //need to fill */
/*     unsigned char oddi; */
/*     unsigned char noai;//nature of address indicator  */
/*     //    unsigned char ni;            /\* Number incomplete indicator, 0 = complete *\/ */
/*     unsigned char npi;      //numbering plan indicator  */
/*     unsigned char apri;     //adress presentation restricted indicator */
/*     //    unsigned char si;       //screening indicator */
/*     unsigned char naddr;     /\* number of address digits *\/ */
/*     unsigned char addr[15]; //mixed pairs of digits, not unpacked yet */
/* }; */


typedef struct pty_addr_orig{	/* ORIGINAL CALLED NUMBER */
	u8	oe;			/* O/E */
	u8 	noa;			/* Nature of address indicator */
	u8	spf;			/* spare 1 */
	u8	apri;	                /* Address presentation restricted indicatior */
	u8	np;			/* Numbering plan indicator */
	u8    sps;			/* spare 2 */
	u8    addr[INTU_SIZEOF_DIGIT_DATA];
} PTY_ADDR_ORIG;

//
//typedef struct msgbuf {
//	long msg_type;
//	long int msg_time; // время в сыром линукс формате
//	int msg_dlg_id;  //current dialog id
//	int msg_serv_mode;//indicator of service mode     
//	u16 msg_operation;
//	u16 msg_databuf_offset;
//	u8 msg_databuf[0];
//} LOG_MSG;


typedef struct msgbuf {
long msg_type;
long int msg_time;
int msg_dlg_id;  //current dialog id
int msg_databuf_offset;
u8 msg_databuf[0]; //maximum size of databuf array. it depends
} LOG_MSG;



/*
* Per dialogue resource store - Dialogue Control Block
*
* cpt - used to store decoded inap messages, error codes and reject problem
* for later processing.
* reply_prepared - Do we have a reply waiting for the delimit
* state - Holds the dialogue state
*/
/* структура узла в односвязном списке компонент диалога */


typedef struct Dlg_Cpt_Node {
	void *prot_spec;
	u16 operation;
	u8 type;
	u16 databuf_offset;
	u8 databuf[256];
	struct Dlg_Cpt_Node *next;
} DLG_CPT_NODE; 



/* struct calldetails { */
/*     char CallReferenceNumber[17]; //call reference is 8 octets total */
/*     unsigned long long ull_CallReferenceNumber; */
/*     char CallingPartyNumber[16]; // 15 max digits in calling and +1 for null term */
/*     char RedirectingNumber[16]; */
/*     char LocationNumber[LOC_NUM_MAX_DIGITS_STR]; */
/*     char IMSI[IMSI_MAX_DIGITS_STR]; */
/*     unsigned long long ull_IMSI; */
/*     char VLR[16]; */
/*     char MSC[16]; */
/*     char CalledPartyNumber[19]; // 15 - max for msisdn 1 for f filler and 1 for null-term 0 */
/*     char CallingPartysCategory[5]; */
/*     char TimeAndTimezone[17]; */
/*     char CountryCodeA[17]; */
/*     char ServiceKey[2]; */
/*     u8 uc_ServiceKey; */
/*     char EventTypeBCSM[3]; */
/*     u8 uc_EventTypeBCSM; */
/*     char CountryCodeB[19]; */
/*     char CELLID[25]; */
/*     int leg_type;          //mo, mf, mt or mtr leg */
/*     unsigned long long id; //identity for processing timeouted replys from billing */
/*     int op_code; //op_code for what we should ask from oracle */
/*     unsigned int quota; */
/*     int call_id; */
/*     int status_code; */
/*     unsigned char param1[16]; //experimetnal for vpn */
/*     unsigned char param2[16]; //experimental for vpn */
/*     int duration; */
/*     struct calling_pty_addr CallingAddr; */
/*     struct called_pty_addr CalledAddr; */
/*     struct redirecting_pty_addr RedirectingAddr; */
/* }; */



/* struct oracle_iface_data { */
/*     char CallReferenceNumber[17]; //call reference is 8 octets total */
/*     char CallingPartyNumber[16]; // 15 max digits in calling and +1 for null term */
/*     char RedirectingNumber[16]; */
/*     char LocationNumber[LOC_NUM_MAX_DIGITS_STR]; */
/*     char IMSI[IMSI_MAX_DIGITS_STR]; */
/*    char VLR[16]; */
/*     char MSC[16]; */
/*     char CalledPartyNumber[19]; // 15 - max for msisdn 1 for f filler and 1 for null-term 0 */
/*     char CallingPartysCategory[5]; */
/*     char TimeAndTimezone[17]; */
/*     char CountryCodeA[17]; */
/*     char ServiceKey[2]; */
/*     char EventTypeBCSM[3]; */
/*     char CountryCodeB[19]; */
/*     char CELLID[25]; */
/*     int leg_type; */
/*     unsigned long long id; //identity for processing timeouted replys from billing */
/*     int op_code; //op_code for what we should ask from oracle */
/*     unsigned int quota; */
/*     int call_id; */
/*     int status_code; */
/*     int duration; */
/* }; */


//struct vpn_iface_data {

 //   unsigned long long id;
    // int status_code;
    //unsigned char param1[16]; //experimetnal for vpn
    //unsigned char param2[16]; //experimental for vpn

 //   char CallingPartyNumber[16]; // 15 max digits in calling and +1 for null term
 //   char IMSI[25];
 //   char CalledPartyNumber[19]; // 15 - max for msisdn 1 for f filler and 1 for null-term 0
 //   char EventTypeBCSM[3];
//    int status_code;
//    unsigned char param1[20]; //experimetnal for vpn
//    unsigned char param2[20]; //experimental for vpn
//
//};


struct m2m_iface_data {
    unsigned long long id;
    //struct base so_Header;
    unsigned char op_code;
    unsigned long long IMSI;
    unsigned long long CallingNumDigits;
    unsigned char CallingNOA; // nature of address of calling party
    unsigned long long CalledNumDigits;
    unsigned char CalledNOA; //nature of address of called party
    unsigned long long CallRefNum;
    unsigned char EventType;
    unsigned char ServiceKey;

    int duration;
    int status_code;
    unsigned int quota;
    unsigned char param1[16]; //experimetnal for vpn
    unsigned char param2[16]; //experimental for vpn
};

struct base {
  unsigned long long id;
   int status_code;
    unsigned char param1[16]; //experimetnal for vpn
    unsigned char param2[16]; //experimental for vpn

};


struct acr{

    unsigned int duration_ms;
    unsigned char leg_active;
    unsigned char party_to_charge;

};

struct erb{

    unsigned int event_type;
    unsigned int message_type;
    unsigned char leg_id;
    unsigned char release_cause[2];
    unsigned char busy_cause[2];
    //unsigned char leg_active;
    //unsigned char party_to_charge;

};



typedef struct dlg_cb
{
	IN_CPT cpt;
	u8     reply_prepared;
	u8     state;
	u16    cursor; /* позиция на началный элемент в массиве databuf, с которая начинается предыдущей компоненты в диалоге */
	u8     service; /* какой сервисный режим отработался в диалоге */
    u8 current_invoke_id; /*invoke последней отправленной компоненты */
    int ( *erb_handler)(u32 , struct dlg_cb *, HDR *, IN_CPT *, struct erb *); //pointer to eventreport BCSM handler function
    int ( *acr_handler)(u32 , struct dlg_cb *, HDR *, IN_CPT *, struct acr *);//pointer to acr_handle_function
	DLG_CPT_NODE *cpt_list_head; /*входные для intu компоненты складываются в односвязный список компонент, который формируется при обработке диалога на шлюзе*/;
	struct calldetails call_details;
} DLG_CB;


/* service types for service value */
#define UNDEF 0
#define ROAMING_PREPAID 1 
#define M2M 2


/*
 * Procedures in file intu.c for dialogue handling state machine and the main control loop
 */
int intu_ent(void);
int INTU_dlg_ind(HDR *h);
int INTU_srv_ind(HDR *h);

int INTU_open_dialogue(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_close_dialogue(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 dlg_close_rsn);
int INTU_change_state(u32 ic_dlg_id, DLG_CB *dlg_ptr, u8 new_state);
int INTU_handle_invoke(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h);
int CAMELGW_handle_returnresultlast(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h);

int CAMELGW_prepare_returnresultlast_logic(u32 ic_dlg_id, DLG_CB *dlg_ptr);
int INTU_prepare_service_logic(u32 ic_dlg_id, DLG_CB *dlg_ptr);
int INTU_prepare_u_abort(u32 ic_dlg_id, DLG_CB *dlg_ptr);
int INTU_prepare_error(u32 ic_dlg_id, DLG_CB *dlg_ptr, u16 err_code);
int INTU_prepare_reject(u32 ic_dlg_id, DLG_CB *dlg_ptr);

int INTU_process_pending_req(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_process_invoke(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h);
//int INTU_translate_number(PTY_ADDR *called_party, PTY_ADDR *dest_routing_addr);

//u8  INTU_fmt_called_num(u8 *buf, u8 siz, PTY_ADDR *called_num);
int INTU_rec_called_num(PTY_ADDR *called_num, u8 *pptr, PLEN plen);
DLG_CB *INTU_get_dlg_cb(u32 *dlg_id, HDR *h);
void *INTU_get_protocol_definition(u16 applic_context_index);

/*
 * Message Sending procedures in file intu_sys.c
 */
int INTU_send_open_rsp(u32 ic_dlg_id);
int INTU_send_delimit(u32 ic_dlg_id);
int INTU_send_close(u32 ic_dlg_id, u8 release);
int INTU_send_u_abort(u32 ic_dlg_id);
int INTU_send_error(u32 ic_dlg_id, DLG_CB *dlg_ptr);
int INTU_send_reject(u32 ic_dlg_id, DLG_CB *dlg_ptr);
int INTU_send_message(u8 sending_user_id, u8 inap_module_id, HDR *h);

/*
 * Tracing display procedures in file intu_trc.c
 */
int INTU_disp_other_msg(HDR *h);
int INTU_disp_dlg_msg(u32 dlg_id, HDR *h);
int INTU_disp_srv_ind(u32 dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_disp_srv_req(u32 dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_disp_invalid_srv_ind(u32 dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_disp_invalid_dlg_ind(u32 dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_disp_unexpected_srv_ind(u32 dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_disp_unexpected_dlg_ind(u32 dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_disp_dlg_reopened(u32 dlg_id, DLG_CB *dlg_ptr, HDR *h);
int INTU_disp_state_change(u32 dlg_id, DLG_CB *dlg_ptr, u8 new_state);
int INTU_disp_param(char *param_name_text, u16 param_name_id, PLEN param_len, u8 *param_data);

