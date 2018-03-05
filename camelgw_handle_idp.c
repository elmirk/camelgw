
/********************************************************************************
*                                                                             *
*        INTU_handle_idp: обрабатывает Invoke IDP from MSC                    *
*                                                                             *
*******************************************************************************/

#include "camelgw_backend.h"
#include "camelgw_drivers.h"
#include "camelgw_utils.h"  //arrtonum function
#include "camelgw_prepaid.h"
#include "camelgw_conf.h"
#include "camelgw_dnc.h"
#include "camelgw_parsers.h"

#include "intu_def.h"
#include "time.h"
#include "sys/ipc.h"
#include "helper.h"
#include "globals.h"
#include <error.h>

#define TEST


#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*msc_type*/
#define MSC_RU    0
#define MSC_INT    1
#define MSC_HOME   2

#define PREPAID_DRIVER 0
#define VPN_DRIVER 1
#define M2M_DRIVER 2

#define MAXSLEEP 8


enum OPTIONS_FLAGS {
	DP2 =     0x01, //DP2 is an int type - MO call
	DP12 =    0x00, //MT call
	BYPASS =  0x00, //bypass mode, all services switched off, just continue to MSC
	SIP_ON =  0x04, //SIP service ON
	SIP_OFF = 0x08, //SIP service OFF
	HOME_VLR = 0x00, // user located in Home VLR
	GUEST_VLR =0x40,// user located in GUEST VLR
};


enum EVENT_FLAGS {
    event_DP2 =     0x00, //DP2 is an int type - MO call
    event_DP12 =    0x01, //MT call
    event_BIT2_OFF =  0x00, //bypass mode, all services switched off, just continue to MSC
    event_BIT2_ON =  0x02, //bit numbering goes from bit1 (not bit0)
    event_REDINFO_Y = 0x04, //SIP service OFF
    event_REDINFO_N = 0x00, // user located in Home VLR
    event_REDPTY_Y =0x08,// user located in GUEST VLR
    event_REDPTY_N = 0x00,
    event_SUB_IDLE = 0x00,
    event_SUB_CAMEL_BUSY = 0x10,
    event_SUB_NDNR = 0x20,
    // event_SUB_NDNR_RA = 0xe0, /*restricted area*/
    event_VLR_Y = 0x100,
    event_VLR_N = 0x00,
    event_FP_Y = 0x200, /*ss forwarding-pending in IDP*/
    event_FP_N = 0x00,
};

//extern u8 example_release_cause[2];
//u8 example_release_cause[];
int socket_d, socket_d_vpn, socket_d_m2m;

char *socket_path_vpn;

int CAMELGW_connect_cmd(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, IN_CPT *buffer_cpt_ptr);
int CAMELGW_connect_vpn_cmd(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, IN_CPT *buffer_cpt_ptr);


int CAMELGW_get_data_from_ora(struct calldetails *ptr);

//int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen);
//unsigned long long arrtonum(unsigned char *data, int length);
//ssize_t sendn_test(int fd, const void *vptr, size_t n); 
//int CAMELGW_rec_called_num_test(struct called_pty_addr *p_called_num, unsigned char *pptr, unsigned short plen, unsigned char num_type);

//int CAMELGW_rec_redirecting_num()

u8 TNo_Answer_Timer = 30;
u8 ONo_Answer_Timer = 30;

static unsigned short event_flags;

volatile sig_atomic_t ora_cli_status_flag;


///*
//function - fill (recover) redirecting number structure
//in - raw array of redirecting party id
//out - filled RedirectingAddr strucutre
//
//  */
//static int CAMELGW_rec_redirecting_num()
//{
//
//
//}

/*call type = true MO
function - set calling party number as string
for billing
 */
static inline int mo_set_calling_billing(struct calling_pty_addr *pso_CallingAddr,  char *pc_CallingPartyNumber)
{
    int i;
		    		for (i=0;i < pso_CallingAddr->naddr;i++)
{
				    //dlg_calldetails_ptr->CallingPartyNumber[i] = digit_buffer[i] + 0x30;
	*(pc_CallingPartyNumber++) = pso_CallingAddr->addr[i] + 0x30;			
}

    *(pc_CallingPartyNumber) = '\0'; //add term null for end of string

    return 0;
}
/*call type = true MO
function - set called party number as string
for billing

 */
static inline int mo_set_called_billing(struct called_pty_addr *pso_CalledAddr,  char *pc_CalledPartyNumber)
{
    int i;
    
    for (i=0;i < pso_CalledAddr->naddr;i++) {
	//sprintf(string,"%x",calling_pty_dgt_str[i]);
	//dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
	*(pc_CalledPartyNumber++) = pso_CalledAddr->addr[i] + 0x30;
    }

    *pc_CalledPartyNumber = '\0'; //add term null for end of string
    
    return 0;
}

/*delete first 3 digits */

//camelgw_dnc2()
//{



//}

/* set CountryCodeB parameter for billing */
/* CountryCodeB for pure MO call = bcd called party number from IDP in international format */
/* if dialed 89xx xxx xx xx we should change to 79xxxxxxx   */
/* if dialed 810 we should remove 810   */
/*if dialed digits begin with + we should do nothing, becaue dialed already in int format */

static inline int mo_set_ccb_billing(struct called_pty_addr *pso_CalledAddr, const char *pc_CalledPartyNumber, char *pc_CountryCodeB, unsigned char msc_type)
{

    if( msc_type || (pso_CalledAddr->bcd_toni) )
	{
    memcpy(pc_CountryCodeB, pc_CalledPartyNumber, strlen(pc_CalledPartyNumber) + 1 );
    return 0;
	}

    //TODO - this part need to be done with pcre library and with some config file like patterns.txt


 if  (strncmp(pc_CalledPartyNumber,"810", 3) == 0)
     {
	 //	 camelgw_dnc2;

	 memcpy(pc_CountryCodeB, pc_CalledPartyNumber + 3, strlen(pc_CalledPartyNumber) -3 + 1 );
	 return 0;
     }

 if (strncmp(pc_CalledPartyNumber,"8", 1) == 0)
     {     
	 //camelgw_dnc1;
	 *pc_CountryCodeB = '7';
	 memcpy(pc_CountryCodeB + 1, pc_CalledPartyNumber + 1, strlen(pc_CalledPartyNumber) -1 + 1 );
 return 0;
     }

 return 1;
}

/*camelgw should pass bcd called party number according to rules

mo call in msc russia - pass unknown and international noa
mo call in msc int    - pass only international noa
return 0 - call with dialed bcd allowed
return 1 - call denied
msc_type = 1  msc_int
msc_type = 0 msc_ru
*/
static inline unsigned char mo_called_num_preanalyze(struct called_pty_addr *pso_CalledAddr, unsigned char msc_type)
{
    //    unsigned char called_num_analyze(unsigned char toni, unsigned char msc_type)
    //    {
	if(msc_type)

	    return ( !(msc_type&&(pso_CalledAddr->bcd_toni)) );
	else
	    return msc_type;
	//    }
}


static inline int mf_set_calling_billing(struct redirecting_pty_addr *pso_RedirectingAddr,  char *pc_CallingPartyNumber)
{

    int i;
		    		for (i=0;i < pso_RedirectingAddr->naddr;i++) {
				    //dlg_calldetails_ptr->CallingPartyNumber[i] = digit_buffer[i] + 0x30;
	*(pc_CallingPartyNumber++) = pso_RedirectingAddr->addr[i] + 0x30;			
}

    *pc_CallingPartyNumber = '\0'; //add term null for end of string



    return 0;
}

  
static inline int  mf_set_called_billing( struct called_pty_addr *pso_CalledAddr,  char *pc_CalledPartyNumber)
{

   int i;
    
    for (i=0;i < pso_CalledAddr->naddr;i++) {
	//sprintf(string,"%x",calling_pty_dgt_str[i]);
	//dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
	*(pc_CalledPartyNumber++) = pso_CalledAddr->addr[i] + 0x30;
    }

    *pc_CalledPartyNumber = '\0'; //add term null for end of string
 

    return 0;
}

static inline int mt_set_calling_billing(struct calling_pty_addr *pso_CallingAddr,  char *pc_CallingPartyNumber)
{

  int i;
    
    for (i=0;i < pso_CallingAddr->naddr;i++) {
	//sprintf(string,"%x",calling_pty_dgt_str[i]);
	//dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
	*(pc_CallingPartyNumber++) = pso_CallingAddr->addr[i] + 0x30;
    }

    *pc_CallingPartyNumber = '\0'; //add term null for end of string
 

    return 0;
}
  
static inline int  mt_set_called_billing(struct called_pty_addr *pso_CalledAddr,  char *pc_CalledPartyNumber)
{

  int i;
    
    for (i=0;i < pso_CalledAddr->naddr;i++) {
	//sprintf(string,"%x",calling_pty_dgt_str[i]);
	//dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
	*(pc_CalledPartyNumber++) = pso_CalledAddr->addr[i] + 0x30;
    }

    *pc_CalledPartyNumber = '\0'; //add term null for end of string
 


    return 0;
}		    
static inline int mtr_set_calling_billing(struct redirecting_pty_addr *pso_RedirectingAddr,  char *pc_CallingPartyNumber)
{

   int i;
		    		for (i=0;i < pso_RedirectingAddr->naddr;i++) {
				    //dlg_calldetails_ptr->CallingPartyNumber[i] = digit_buffer[i] + 0x30;
	*(pc_CallingPartyNumber++) = pso_RedirectingAddr->addr[i] + 0x30;			
}

    *pc_CallingPartyNumber = '\0'; //add term null for end of string



    return 0;
}
  
static inline int   mtr_set_called_billing(struct called_pty_addr *pso_CalledAddr,  char *pc_CalledPartyNumber)
{

  int i;
    
    for (i=0;i < pso_CalledAddr->naddr;i++) {
	//sprintf(string,"%x",calling_pty_dgt_str[i]);
	//dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
	*(pc_CalledPartyNumber++) = pso_CalledAddr->addr[i] + 0x30;
    }

    *pc_CalledPartyNumber = '\0'; //add term null for end of string
 

    return 0;
}


static int CAMELGW_rec_called_num(struct called_pty_addr *p_called_num, unsigned char *pptr, unsigned short plen, unsigned char num_type)
//     PTY_ADDR  *called_num;  /* called party number recovered into here */
//     u8        *pptr;        /* pointer into parameter buffer */
//     PLEN       plen;         /* length of parameter being recovered */
// num_type = 2 for recovering called party bcd number and num_type = 12 for recovering called party number
{
    unsigned char         *addr;

    //    if ((plen < INTU_min_called_party_num_len) ||
    //	(plen > INTU_max_called_party_num_len))
    //	{
    //	    /*
    //	     * Parameter length is outside supported range.
    //	     */
    //	    return(INTUE_NUM_RECOVERY_FAILED);
    //	}
    /*
     * recover fixed length fields
     */
    // p_calling_num->oddi = bit_from_byte(*pptr, 7);
    p_called_num->num_type = num_type;

    if ( num_type  == 2 )
	{
     plen -= 1; //one  octets in called bcd num digit string is service octets

    p_called_num->bcd_exti = bit_from_byte(*pptr, 7);
    p_called_num->bcd_toni = bits_from_byte(*pptr, 4, 3);
    p_called_num->both_npi =  bits_from_byte(*pptr++, 0, 4);

	//    p_calling_num->ni = bit_from_byte(*pptr, 7);
	//p_calling_num->npi = bits_from_byte(*pptr, 4, 3);
	//p_calling_num->apri = bits_from_byte(*pptr, 2, 2);
	//p_calling_num->si = bits_from_byte(*pptr++, 0, 2);


    p_called_num->naddr = (unsigned char)(plen << 1); /*equal to multiple to 2 operation */
    //   called_num->noai = bits_from_byte(*pptr++, 0, 7);
    //called_num->inni = bit_from_byte(*pptr, 7);
    //called_num->npi = bits_from_byte(*pptr, 4, 3);
    //called_num->ni = 0;
    //called_num->pri = 0;
    //called_num->si =  0;
    /*
     * Now recover (variable length) address digits
     */
    //    addr = p_calling_num->addr;
    //while(plen--)
    //	*addr++ = *++pptr;
    /*
     * Force filler to zero (if naddr odd)
     */
    //if (p_calling_num->oddi)
    //	*--addr &= 0x0f;

    unpack_digits(p_called_num->addr, pptr, 0, p_called_num->naddr);

    if (p_called_num->addr[ p_called_num->naddr - 1] == 0x0f)
	p_called_num->naddr = p_called_num->naddr -1;
	



	}

    if ( num_type == 12 )
	{

     plen -= 2; //two octets in calling num digit string is service octets
     //	unsigned char    both_npi;            /* Numbering plan indicator 0001b ISDN E164 */
    p_called_num->oddi = bit_from_byte(*pptr, 7);; //odd-even indicator in calledpartynumber
    p_called_num->noai = bits_from_byte(*pptr++, 0, 7);
    p_called_num->inni = bit_from_byte(*pptr, 7);; //odd-even indicator in calledpartynumber;
    //   unsigned char    naddr;          /* number of address digits , this is our element not exist in standart calledBCDnumber*/
 p_called_num->both_npi =  bits_from_byte(*pptr++, 4, 3);


    p_called_num->naddr = (unsigned char)(plen << 1) - p_called_num->oddi;
    //   called_num->noai = bits_from_byte(*pptr++, 0, 7);
    //called_num->inni = bit_from_byte(*pptr, 7);
    //called_num->npi = bits_from_byte(*pptr, 4, 3);
    //called_num->ni = 0;
    //called_num->pri = 0;
    //called_num->si =  0;
    /*
     * Now recover (variable length) address digits
     */
    //    addr = p_calling_num->addr;
    //while(plen--)
    //	*addr++ = *++pptr;
    /*
     * Force filler to zero (if naddr odd)
     */
    //if (p_calling_num->oddi)
    //	*--addr &= 0x0f;

    unpack_digits(p_called_num->addr, pptr, 0, p_called_num->naddr);

	}

    return 0;
}

 /*
 * CAMELGW_rec_calling_num()
 *
 * Returns 0 on success
 *         INTUE_NUM_RECOVERY_FAILED otherwise
 */
static int CAMELGW_rec_calling_num(struct calling_pty_addr *p_calling_num, u8 *pptr, u16 plen) {
//     struct calling_pty_addr  *p_calling_num;  /* called party number recovered into here */
//     u8        *pptr;        /* pointer into parameter buffer */
//     u16       plen;         /* length of parameter being recovered, number of octest in parameter buffer */

    unsigned char         *addr;

        if ((plen < INTU_min_calling_party_num_len) ||
    	(plen > INTU_max_calling_party_num_len))
    	{
    	    /*
    	     * Parameter length is outside supported range.
    	     */
    	    return(INTUE_NUM_RECOVERY_FAILED);
    	}

      plen -= 2; //two octets in calling num digit string is service octets

    p_calling_num->oddi = bit_from_byte(*pptr, 7);
    p_calling_num->noai = bits_from_byte(*pptr++, 0, 7);
    
    p_calling_num->ni = bit_from_byte(*pptr, 7);
    p_calling_num->npi = bits_from_byte(*pptr, 4, 3);
    p_calling_num->apri = bits_from_byte(*pptr, 2, 2);
    p_calling_num->si = bits_from_byte(*pptr++, 0, 2);


    p_calling_num->naddr = (unsigned char)(plen << 1) - p_calling_num->oddi;
  
    unpack_digits(p_calling_num->addr, pptr, 0, p_calling_num->naddr);

    return 0;
}

/*
function - fill (recover) redirecting number structure
in - raw array of redirecting party id
out - filled RedirectingAddr strucutre

  */
static int CAMELGW_rec_redirecting_num(struct redirecting_pty_addr *p_redirecting_num, u8 *pptr, u16 plen)
{


      plen -= 2; //two octets in redirecting num digit string is service octets

    p_redirecting_num->oddi = bit_from_byte(*pptr, 7);
    p_redirecting_num->noai = bits_from_byte(*pptr++, 0, 7);
    
    //p_calling_num->ni = bit_from_byte(*pptr, 7);
    p_redirecting_num->npi = bits_from_byte(*pptr, 4, 3);
    p_redirecting_num->apri = bits_from_byte(*pptr++, 2, 2); //need to move pointer to digits octet
    //p_calling_num->si = bits_from_byte(*pptr++, 0, 2);


    p_redirecting_num->naddr = (unsigned char)(plen << 1) - p_redirecting_num->oddi;
  
    unpack_digits(p_redirecting_num->addr, pptr, 0, p_redirecting_num->naddr);

    return 0;
}


extern struct config camelgw_conf;

/***********************************************************************************************
function used to:
- parse received IDP
- request Tarantool for idp_handler function pointer of subscriber = SubscriberID
- call idp_handler function by received from tarantool pointer

*************************************************************************************************/

int camelgw_handle_idp(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, IN_CPT *buffer_cpt_ptr)
{

	struct calldetails	*dlg_calldetails_ptr; 
	SERVICE_KEY_TYPE	srv_key;
	PTY_ADDR	*called_num;	
	PTY_ADDR	called_pty;			// Stores the recovered calling party number
	PTY_ADDR	dest_routing_addr;	// Holds the outgoing destination number 
	PLEN	called_pty_plen;		// Size of param 
	PLEN	dest_addr_plen;			// Size of param 
	PLEN	invokeID_len;			// Octet length of InvokeID
	PLEN	calling_pty_plen;

	IN_CPT	*dlg_cpt_ptr;	// Holds recovered INAP component 
	IN_CPT	og_cpt;			// Holds INAP component whilst building into message 
	IN_CPT	og2_cpt;		// this is for outgoing ApplyCharging component
	IN_CPT	og3_cpt;
	HDR	*og_h;				// Message used to send immediate replies 
	HDR	*og2_h;
	HDR *og1_h;
	HDR *og3_h;
	int	status, status_ap;	// Return code from INAP API functions 
	int	regime=0;
	int	service_mode;
	int i, n, m, s, j;
	int start_str, end_str, answer_IDP; 
	int ret;
	char	VLR[15];
	char	CELLID[16];
	char	LAI[14];
	char	IMSI[25];
	char	LocationNumber[11];
		char	CalledPartyNumber[11];
	char	CallingPartyNumber[12];
	char	CountryCodeB[14];
	char	CountryCodeA[14];
	//char	CallingPartysCategory[2];

	unsigned char digits_buffer[32];  //one elemen is pair of digits from octets string
	unsigned char digit_buffer[32];   // one element is one digit
	u16 data_length;
	int num_digits;
	int result;
	char *char_ptr;

	long	http_code = 0;
	unsigned char	options_flag = 0x00;
	u8	Rel_If_Dur_Excd[] = {129,1,0};
	u8	apc[]={36,0,2}; 			// max call duration in apply_charging message 
		u8	called_pty_param[INTU_max_called_party_num_len]; // Called party 
	
		u8	invokeID;//, invokeID2, invokeID3;	
	u8	max_call_duration[]={0,0};	

	u8	callingCategory_pty_param[10];
	u8	msc_type;
	u8	calling_pty_param[11];
	//u8	eventtypebcsm_param[3];
	//u8  sub_state_param[5];
	//u8  call_forward_param[3];
	unsigned char call_type;
	u16	op_code;

	struct calling_pty_addr *pso_CallingAddr;
	struct called_pty_addr *pso_CalledAddr;
	struct redirecting_pty_addr *pso_RedirectingAddr;

	u8 eventTypeBCSM_array_DP2[8]; //events for MO
	u8 eventTypeBCSM_array_DP12[8]; //events for MT
	u8 monitorMode_array[2];
	u8 legID_array[2];
	//	int ret;
	//	u8 TNo_Answer_Timer, ONo_Answer_Timer;

	//	fprintf(stderr, "INTU: *** отправил OPEN_RESPONSE на модуль INAP ***\n");
	//printf("Длина списка компонент IDP = %d\n", CAMELGW_list_length(&(dlg_ptr->cpt_list_head)));
	//CAMELGW_(&(dlg_ptr->cpt_list_head), op_code, dlg_cpt_ptr->databuf_offset);

	/*	if (CAMELGW_list_length(&(dlg_ptr->cpt_list_head)) == 1) /*если в списке только одна компонента, то обрабатываем только ее */
	//	fprintf(stderr, ANSI_COLOR_YELLOW "Type of message in process IDP function  op(dec) %i" ANSI_COLOR_RESET "\n", h->type);

 monitorMode_array[0] = interrupted;
 monitorMode_array[1] = notifyAndContinue;

 legID_array[0] = Leg_01;
 legID_array[1] = Leg_02;

 eventTypeBCSM_array_DP2[0] = eventTypeBCSM_OAnswer; 
 eventTypeBCSM_array_DP2[1] = eventTypeBCSM_OCalledPartyBusy, 
 eventTypeBCSM_array_DP2[2] = eventTypeBCSM_ONoAnswer; 
 eventTypeBCSM_array_DP2[3] = eventTypeBCSM_ODisconnect;
 eventTypeBCSM_array_DP2[4] = eventTypeBCSM_ODisconnect;
 eventTypeBCSM_array_DP2[5] = eventTypeBCSM_OAbandon;
 eventTypeBCSM_array_DP2[6] = eventTypeBCSM_RouteSelectFailure;


 eventTypeBCSM_array_DP12[0] = eventTypeBCSM_TBusy;
 eventTypeBCSM_array_DP12[1] = eventTypeBCSM_TNoAnswer; 
 eventTypeBCSM_array_DP12[2] = eventTypeBCSM_TAnswer; 
 eventTypeBCSM_array_DP12[3] = eventTypeBCSM_TDisconnect; 
 eventTypeBCSM_array_DP12[4] = eventTypeBCSM_TDisconnect;
 eventTypeBCSM_array_DP12[5] = eventTypeBCSM_TAbandon; 

 /*	for(i=0;i<(buffer_cpt_ptr->databuf_offset); i++){
	printf("buffer cpt  databuf in handle idp[%i]  =====  %x\n", i, buffer_cpt_ptr->databuf[i]);
	} */

	dlg_cpt_ptr = &(dlg_ptr->cpt); 
	dlg_calldetails_ptr = &(dlg_ptr->call_details);

	/*********************************** incoming InvokeID precheking **********************************/
	/*
	* Get the invokeID, add 128 (0x80) and reply with that.
	* InvokeID chosen to be (0   to 127) Incoming
	*                       (128 to 255) Outgoing
	*/
	status = IN_get_component_param(INPN_InvokeID, &invokeID_len, &invokeID, 1, buffer_cpt_ptr);


	if (status == IN_SUCCESS)
	    {
	    }
	else
	    {
	    printf("error: couldnt get InvokeID from IDP\n");
	    }
	//TODO - must place here  if (status == IN_SUCCESS)

	/*
	* Check the invokeID isn't outside the expected range
	*/
	if (invokeID > 127)
	{
		printf("error: *** Invalid Invoke ID 0x%02x for dialogue ID 0x%04x ***\n", invokeID, ic_dlg_id);
		IN_set_error(dlg_ptr->cpt.operation, INER_UnexpectedDataValue, &(dlg_ptr->cpt));
		INTU_send_error(ic_dlg_id, dlg_ptr);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
		return INTUE_INVALID_INVOKE_ID;
	}
	else
	{
	    invokeID += 128;  /* invokeID prepared for replies */
	    dlg_ptr->current_invoke_id = invokeID; /*save current value for outgoing component*/

	    memset(dlg_calldetails_ptr->IMSI, 0, sizeof(dlg_calldetails_ptr->IMSI));   
	    memset(dlg_calldetails_ptr->LocationNumber, 0, sizeof(dlg_calldetails_ptr->LocationNumber));    
	    memset(dlg_calldetails_ptr->CallingPartyNumber, 0, sizeof(dlg_calldetails_ptr->CallingPartyNumber)); 	
            memset(dlg_calldetails_ptr->RedirectingNumber, 0, sizeof(dlg_calldetails_ptr->RedirectingNumber));
            memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber));
	    memset(dlg_calldetails_ptr->EventTypeBCSM, 0, sizeof(dlg_calldetails_ptr->EventTypeBCSM));
	    memset(dlg_calldetails_ptr->CountryCodeA, 0, sizeof(dlg_calldetails_ptr->CountryCodeA));
	    //memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber));
	    memset(dlg_calldetails_ptr->CountryCodeB, 0, sizeof(dlg_calldetails_ptr->CountryCodeB));
        }
	//	dlg_cpt_ptr = &(dlg_ptr->cpt); 
	/*
	fprintf(stderr, ANSI_COLOR_YELLOW "Type of message before op(dec) %i" ANSI_COLOR_RESET "\n", og2_h->type);
	fprintf(stderr, ANSI_COLOR_YELLOW "Module instantiation(hex) %x" ANSI_COLOR_RESET "\n", og2_h->id);
	fprintf(stderr, ANSI_COLOR_YELLOW "Sending Module ID(hex) %x" ANSI_COLOR_RESET "\n", og2_h->src);
	fprintf(stderr,ANSI_COLOR_YELLOW "Destination Module ID(hex) %x" ANSI_COLOR_RESET "\n", og2_h->dst);
	fprintf(stderr,ANSI_COLOR_YELLOW "Response Required(hex) %x" ANSI_COLOR_RESET "\n", og2_h->rsp_req);
	fprintf(stderr, ANSI_COLOR_YELLOW "Generic MSG type(hex) %x" ANSI_COLOR_RESET "\n",og2_h->hclass);
	fprintf(stderr, ANSI_COLOR_YELLOW "Returned Status(hex) %x" ANSI_COLOR_RESET "\n",og2_h->status);
	*/
	/*
	*/
	//dlg_calldetails_ptr = &(dlg_ptr->call_details);

	//fprintf(stderr, ANSI_COLOR_YELLOW "string ServiceKey as it goes at the beginning in function handle IDP =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->ServiceKey);

	idp_parser(buffer_cpt_ptr, dlg_calldetails_ptr);

			printf("Service Key in HandleInvoke function as digit = %d\n", dlg_calldetails_ptr->uc_ServiceKey);
 fprintf(stderr, ANSI_COLOR_YELLOW "string CallReferenceNumber as it goes after idp parse function =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallReferenceNumber);
 fprintf(stderr, ANSI_COLOR_YELLOW "string ServiceKey as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->ServiceKey);
 fprintf(stderr, ANSI_COLOR_YELLOW "string IMSI as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->IMSI);
 fprintf(stderr, ANSI_COLOR_YELLOW "string LocationNumber as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->LocationNumber);
 fprintf(stderr, ANSI_COLOR_YELLOW "string CELLID as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CELLID);
 fprintf(stderr, ANSI_COLOR_YELLOW "string CalledPartyNumber as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CalledPartyNumber);  
 fprintf(stderr, ANSI_COLOR_YELLOW "string CountryCodeB  as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CountryCodeB);
 fprintf(stderr, ANSI_COLOR_YELLOW "string MSC as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->MSC); 
 fprintf(stderr, ANSI_COLOR_YELLOW "string CountryCodeA as it goes to IRBIS before ora_ cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CountryCodeA); 
 fprintf(stderr, ANSI_COLOR_YELLOW "string CallingPartyNumber as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallingPartyNumber);
 fprintf(stderr, ANSI_COLOR_YELLOW "string RedirectingNumber as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->RedirectingNumber);


/*TODO - we should block if imsi not equal to 25027xxxxxxxxxx
  /*********************************************************************************************/


 /*prepare type of msc - home, russia or international */ 

 if  (strncmp(dlg_calldetails_ptr->MSC,"79506651021", 11) == 0)
     msc_type = MSC_HOME;
 else if (strncmp(dlg_calldetails_ptr->MSC,"79",2) == 0)
     msc_type = MSC_RU;
 else
     msc_type = MSC_INT;


 // if  (strncmp(msc,"79506651021", 11) == 0)
     // msc_type = MSC_HOME;
 //     printf("home msc\n");  

 // else if (strncmp(msc,"79",2) == 0)
 //  printf("msc russian\n");
 // else
 //    printf("msc international\n");


#ifdef TEST
		result = strncmp(dlg_calldetails_ptr->VLR,"7950665102", 11);    //uncomment this для тестов
#else
		result = strncmp(dlg_calldetails_ptr->VLR,"79506651021", 11);    //  uncomment this для комерции
#endif

		if (result == 0)
		    options_flag = options_flag | (unsigned char) HOME_VLR;
		else
		    options_flag = options_flag | (unsigned char) GUEST_VLR; //or CF call

	/*
	* проверка на действующие на шлюзе сервисы (задаются в lua)
	* script_get_int - запрашиваем выбор режима у lua скрипта
	*/
	//int service_option = script_get_int(REGIME);
		//		int service_option = read_lua_config();

		unsigned char service_option = camelgw_conf.bypass_type;
		//TODO - where is prepaid bypass here?
	//	printf("service option received from Lua config = %i\n", service_option);

	switch (service_option)
{
 case 1: //bypass all services
	 //bypass all service logic
	 options_flag = options_flag | (unsigned char) BYPASS; 
	 break;
 case 2:
	case 3:
	 /*
	 * SIP service is ON and camel gw should contac 
	 * with sip server (letaika service)
	 */
	 options_flag = options_flag | (unsigned char) SIP_ON; 
	 break;
 case 4:
	 /*
	 * SIP service switched off and no need to contact with sip server
	 */ 
	 options_flag = options_flag | (unsigned char) SIP_OFF; 
	 break;

 default: //should be some code for other values
	 break;
	}



	/***** if service key = 2 then this is M2M service case **********/

	if  ( dlg_calldetails_ptr->uc_ServiceKey == 2)

	    {

		dlg_ptr->service = M2M;
		//TODO - refactor as for prepaid call handling case!		
//old		dlg_calldetails_ptr->op_code = CAMELGW_M2M_QUOTA_REQ;
		printf("debug: status code before m2m RequestDriver= %d\n", dlg_calldetails_ptr->status_code);
		//	CAMELGW_RequestDriver(M2M_DRIVER, dlg_calldetails_ptr);
	if ( ( ret = CAMELGW_RequestDriver(M2M_DRIVER, dlg_calldetails_ptr) ) != 0)
	    {
			    printf("debug: error -  intu_handle_idp, ret = %d\n", ret);
			    printf("debug: error - but continue m2m subs call\n");
			    //			    CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);
			    return 0;
	    }
		printf("debug: intu_handle_idp :status code after RequestDriver for m2m = %d\n", dlg_calldetails_ptr->status_code);

if ( dlg_calldetails_ptr->status_code == M2M_SUCCESS) 
    {
		switch(dlg_calldetails_ptr->param1[0])
		    {
		    case M2M_RESPONSE_REQNEXTQUOTA:
			printf("debug:req next quota\n");
			CAMELGW_call_duration_control(ic_dlg_id, dlg_ptr, h, &invokeID, CONTINUE, dlg_calldetails_ptr->quota);
			return 0;
			break;
		    case M2M_RESPONSE_LASTQUOTA:
			printf("debug:intu_handle_idp:m2m:last quota, release\n");
			CAMELGW_call_duration_control(ic_dlg_id, dlg_ptr, h, &invokeID, RELEASE, dlg_calldetails_ptr->quota);
			return 0;
			break;
		    case M2M_RESPONSE_RELEASECALL:
			printf("debug:intu_handle_idp:m2m:release cmd from m2m received\n");
			CAMELGW_invoke_releasecall(ic_dlg_id, dlg_ptr, h, &invokeID);
			return 0;
			break;
		    case M2M_RESPONSE_DEFAULTACTION:
	printf("debug:intu_handle_idp:m2m:default action\n");
			break;
		    case M2M_RESPONSE_OUTOFORDER_DUE_MAINT:	
printf("debug:intu_handle_idp:m2m:out of order\n");
			break;
		    default:
	printf("debug:intu_handle_idp:m2m:default case\n");
			break;
		}

	    }
	    }
	/******************************************************************/


  /* 	switch (options_flag) { */
  /* 	 case 0: */
  /* 	 case 1: */
  /* 	 case 5: */
  /* 	 case 8: */
  /* 	 case 9: */
  /* 	 case 64: */
  /* 	 case 65: */
  /* 		 service_mode = MODE_CONTINUE_ONLY; // Camel GW reply with Continue, mode = MODE_CONTINUE_ONLY */
  /* 		 break; */
  /* 	 case 4: 									// 4 dec == 00 0001 00 bin, home VLR */

  /* 	     // check if we should send call to sip letajka service */

  /* 		    mt_set_calling_billing(&dlg_calldetails_ptr->CallingAddr,  &dlg_calldetails_ptr->CallingPartyNumber[0]); */
  /* mt_set_called_billing(&dlg_calldetails_ptr->CalledAddr,  &dlg_calldetails_ptr->CalledPartyNumber[0]); */

  /* 	     http_code = get(dlg_calldetails_ptr->CalledPartyNumber, dlg_calldetails_ptr->CallingPartyNumber); */
  /* 		 if (http_code != 200) */
  /* 			 service_mode = MODE_CONTINUE_ONLY; // Camel GW reply with Continue */
  /* 		 else */
  /* 			 /\* */
  /* 			 * Camel GW reply with Connect, B-sub in SIP domen,  */
  /* 			 * this is SIP_TERMINATION mode */
  /* 			 *\/ */
  /* 			 service_mode = MODE_SIP_TERMINATION;  */
  /* 		 break; */
  /* 	 case 68:  //68 dec == 01 0001 00, guest VLR */

  /* 		    mt_set_calling_billing(&dlg_calldetails_ptr->CallingAddr,  &dlg_calldetails_ptr->CallingPartyNumber[0]); */
  
  /* mt_set_called_billing(&dlg_calldetails_ptr->CalledAddr,  &dlg_calldetails_ptr->CalledPartyNumber[0]); */

	     
  /* 		 http_code = get(dlg_calldetails_ptr->CalledPartyNumber, dlg_calldetails_ptr->CallingPartyNumber); */
  /* 		 if (http_code !=200) */
  /* 			 /\*  */
  /* 			 * Camel GW reply with  */
  /* 			 * Continue + Apply Charging + RequestReportBCSMEvent */
  /* 			 *\/ */
  /* 			 service_mode = MODE_PREPAID;  */
  /* 		 else */
  /* 			 service_mode = MODE_SIP_TERMINATION; */
  /* 		 break; */
  /* 	 case 69: */
  /* 	 case 72: */
  /* 	 case 73: */
  /* 		 service_mode = MODE_PREPAID; */
  /* 		 break; */
  /* 	 default: 									//should be some code for other values */
  /* 		 break; */
  /* 	} */


	//	declare pointer to idp handler function
    int ( *idp_handler)(u32 , DLG_CB *, HDR *, u8 *, IN_CPT *, struct calldetails *);

	idp_handler = camelgw_tnt_select(&dlg_calldetails_ptr->Subscriber_id[0]);

	printf("idp handler pointer = %p\n", idp_handler);

	//when error in tarantool idp handler wiil be 1 - we should process this case by some way!

	idp_handler(ic_dlg_id, dlg_ptr, h, &invokeID, buffer_cpt_ptr, dlg_calldetails_ptr);

	printf("before return from handle idp function!\n");

	return 0;


	/*************************************************************************/
	/* логика обработки IDP зависит от полученного набора флагов service_mode */
	/*************************************************************************/
	if (service_mode == MODE_CONTINUE_ONLY) // only Continue message and do prearrange end for dialog
	    {
		
		dlg_calldetails_ptr->status_code = 1234;
	printf("info:vpn_driver: status code before RequestDriver= %d\n", dlg_calldetails_ptr->status_code);
		CAMELGW_RequestDriver(VPN_DRIVER, dlg_calldetails_ptr);
		printf("info: vpn_driver: status code after RequestDriver= %d\n", dlg_calldetails_ptr->status_code);
		if ( dlg_calldetails_ptr->status_code == VPN_CONNECT)
		    {
			printf("info: vpn_driver: going to to vpn connect cmd!\n");
			CAMELGW_connect_vpn_cmd(ic_dlg_id, dlg_ptr, h, &invokeID, buffer_cpt_ptr);
			return 0;
		    }
	if ( dlg_calldetails_ptr->status_code == VPN_RELEASE)
		    {
			CAMELGW_invoke_releasecall(ic_dlg_id, dlg_ptr, h, &invokeID);
			return 0;
		    }
		if ( dlg_calldetails_ptr->status_code == VPN_CONTINUE)
		    {
			//CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);
			return 0;
		    }
		
		if ( dlg_calldetails_ptr->status_code == VPN_CONTINUE_NOK)
		    {
			//CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);
			return 0;
		    }
		if ( dlg_calldetails_ptr->status_code == VPN_TIMEOUT)
		    {
			//CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);
			return 0;
		    }	
		else
		    {
			//			CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);
			return 0;
		    }	
	    }
	
	if (service_mode == MODE_SIP_TERMINATION) // do Connect
	    {
		CAMELGW_connect_cmd(ic_dlg_id, dlg_ptr, h, &invokeID, buffer_cpt_ptr);
		return 0;
	    }


	if (service_mode == MODE_PREPAID)
	    {


 //TODO!!!!     

// IN_get_component_param(INPN_LocationNumber, &location_number_plen,location_number_param, sizeof(location_number_param), buffer_cpt_ptr);
     //IN_get_component_param(INPN_CellIdFixedLength, &cellid_number_plen,cellid_number_param, sizeof(cellid_number_param), buffer_cpt_ptr);
     //cell id or lai

		if (service_mode == MODE_PREPAID)
		    {
			//old!!			dlg_calldetails_ptr->op_code = CAMELGW_ORACLE_INITIAL_QUOTA_REQ; /*op code = 0 means intu asks ora_cli for initial quota */

			      ora_cli_status_flag = 2;

      prepaid_init_call(dlg_calldetails_ptr, ic_dlg_id);


/* блокирующая функция*/
//old!!! CAMELGW_get_data_from_ora(dlg_calldetails_ptr);//TODO - need to process return of this function


 printf("INTU_HANDLE_IDP: data from ora_cli : QUOTA(dec) = %i\n", dlg_calldetails_ptr->quota);
 printf(" CALL_ID = %i\n", dlg_calldetails_ptr->call_id);
 printf(" Return Status Code = %i \n", dlg_calldetails_ptr->status_code);
 printf("static id in handle_idp function = %i\n", dlg_calldetails_ptr->id);

 //fprintf(stderr, ANSI_COLOR_YELLOW "string CallReference as it goes  after get data from ora in  handle IDP function =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallReferenceNumber);
 
 //uncomment below if test without oracle

 //	dlg_calldetails_ptr->quota = 180;
 //	dlg_calldetails_ptr->status_code = 1;
 /*
			-----------------Коды возврата при обращение БД----------------
			status_code = 1 - у абонента положительный баланс but also need to check quota value, may be quota = 0, if so then release call
			status_code = 2 - у абонента баланс <=  0 (абонента нет в базе) 
			//status_code = 991
			status_code = 995 - в течение 2 секунд не получен ответ от БД при вызове функции запроса первой длительности
			status_code = 991 - Ошибка функции OCI Execute, абонент в таблице БД не найден
			status_code = 999 - Ошибка со стороны БД 
			*/

			switch (dlg_calldetails_ptr->status_code)
			    {
			case 1: answer_IDP=AplyChar; //should sent triplet to INAP - camel gw allow call starting in MSC
			    if (dlg_calldetails_ptr->quota == 0)
				answer_IDP = ReleaseCall;
			    break;
			case 2: answer_IDP=ReleaseCall; //data couldn't find in irbis, something wrong or balance negative - should release call, когда баланс отрицательный
	break;
			case 991: answer_IDP=Continue; //ошибка при вызове OCI execute при запросе первой квоты, например когда абонент не найден в БД IN Balance
	break;
			    case 995:
				printf("error: f1 timeout, continue call!!\n");
				answer_IDP=Continue;
				break;
			case 999: answer_IDP=Continue; //error in DB we should continue call anyway
	break;
			    default:
				printf("error: unsupported status value = %d, continue call!!\n", dlg_calldetails_ptr->status_code);
				answer_IDP=Continue;
				break;
			}

			if (answer_IDP == ReleaseCall)
			    {
				//printf("intu should release call\n");
				/* необходимо или разорвать установление голосового соединения или отправить Continue*/
				CAMELGW_invoke_releasecall(ic_dlg_id, dlg_ptr, h, &invokeID);
				released_call_cdr(dlg_calldetails_ptr, ic_dlg_id, 2);

				//CAMELGW_continue_cmd(ic_dlg_id, dlg_ptr, h, &invokeID);
				//CAMELGW_releasecall_cmd(ic_dlg_id, dlg_ptr, h, &invokeID);
			return 0;


			    }

			else if (answer_IDP == Continue)
			    {

				//				printf("intu should continue call\n");
				//				CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);
			   
			    }
		    }


		// int service_mode =2;
		if ((service_mode == MODE_PREPAID)&&(answer_IDP == AplyChar))
		    {
	/* INOP_CONTINUE */
			//		fprintf(stderr, "INTU:handle_idp: Prepaid mode  for dialogue ID 0x%04x ***\n", ic_dlg_id);

	max_call_duration[0] = (dlg_calldetails_ptr->quota * 10) >> 8;
	max_call_duration[1] = 0x00ff & (dlg_calldetails_ptr->quota * 10);
		
	//#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);
	//#else
	//IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt);
	//#endif

	/* INOP_ApplyCharging */
#ifdef IN_LMSGS
	IN_init_component(buffer_cpt_ptr->prot_spec,&og2_cpt,0);
#else
	IN_init_component(buffer_cpt_ptr->prot_spec,&og2_cpt);
#endif

	/* INOP_RequestReportBCSMEvent */
#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og3_cpt,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og3_cpt);
#endif

#ifdef IN_LMSGS
	if ((og1_h = IN_alloc_message(0)) == 0)
#else
	if ((og1_h = IN_alloc_message()) == 0)
#endif
	{
		/*
		* We can't get a message to reply with so just return
		*/
		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	}
#ifdef IN_LMSGS
	if ((og2_h = IN_alloc_message(0)) == 0)
#else
	if ((og2_h = IN_alloc_message()) == 0)
#endif
	{

		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	}	

#ifdef IN_LMSGS
	if ((og3_h = IN_alloc_message(0)) == 0)
#else
	if ((og3_h = IN_alloc_message()) == 0)
#endif
	{
		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	}

			if (dlg_calldetails_ptr->uc_EventTypeBCSM == 12)
			{
				IN_set_operation(INOP_RequestReportBCSMEvent, INTU_ReleaseCall_timeout, &og3_cpt);
				//				IN_set_component_param(INPN_InvokeID,1,&invokeID,&og3_cpt);
				IN_set_component_param(INPN_InvokeID, 1, &dlg_ptr->current_invoke_id, &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(0), 1, &eventTypeBCSM_array_DP12[0], &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(0), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_SendingSideID(0), 1, &legID_array[1], &og3_cpt);  //sendingside = legid =2	
				IN_set_component_param(INPN_EventTypeBCSM(1), 1, &eventTypeBCSM_array_DP12[1] , &og3_cpt); 
				IN_set_component_param(INPN_MonitorMode(1), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_SendingSideID(1), 1, &legID_array[1], &og3_cpt);
				IN_set_component_param(INPN_ApplicationTimer(1), 1, &TNo_Answer_Timer, &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(2), 1,  &eventTypeBCSM_array_DP12[2], &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(2), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_SendingSideID(2), 1, &legID_array[1], &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(3), 1, &eventTypeBCSM_array_DP12[3], &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(3), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_SendingSideID(3), 1, &legID_array[0], &og3_cpt); 
				IN_set_component_param(INPN_EventTypeBCSM(4), 1, &eventTypeBCSM_array_DP12[4], &og3_cpt);   
				IN_set_component_param(INPN_MonitorMode(4), 1, &monitorMode_array[0], &og3_cpt); //interrupted mode 
				IN_set_component_param(INPN_SendingSideID(4), 1, &legID_array[1], &og3_cpt); 
				IN_set_component_param(INPN_EventTypeBCSM(5), 1, &eventTypeBCSM_array_DP12[5], &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(5), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_SendingSideID(5), 1, &legID_array[0], &og3_cpt); //legid = 1
			}

			else if (dlg_calldetails_ptr->uc_EventTypeBCSM == 2)
			{

				IN_set_operation(INOP_RequestReportBCSMEvent, INTU_ReleaseCall_timeout, &og3_cpt);
				IN_set_component_param(INPN_InvokeID,1, &dlg_ptr->current_invoke_id, &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(0), 1, &eventTypeBCSM_array_DP2[0], &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(0), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(1), 1, &eventTypeBCSM_array_DP2[1], &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(1), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(2), 1, &eventTypeBCSM_array_DP2[2]  , &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(2), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_ApplicationTimer(2), 1, &ONo_Answer_Timer, &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(3), 1, &eventTypeBCSM_array_DP2[3], &og3_cpt); //ODisconnect
				IN_set_component_param(INPN_MonitorMode(3), 1, &monitorMode_array[0], &og3_cpt); //mode interrupted
				IN_set_component_param(INPN_SendingSideID(3), 1, &legID_array[0], &og3_cpt); //legid = 1
				IN_set_component_param(INPN_EventTypeBCSM(4), 1, &eventTypeBCSM_array_DP2[4], &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(4), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_SendingSideID(4), 1, &legID_array[1], &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(5), 1, &eventTypeBCSM_array_DP2[5] , &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(5), 1, &monitorMode_array[1], &og3_cpt);
				IN_set_component_param(INPN_EventTypeBCSM(6), 1, &eventTypeBCSM_array_DP2[6], &og3_cpt);
				IN_set_component_param(INPN_MonitorMode(6), 1, &monitorMode_array[1], &og3_cpt);
				//	printf("test2\n");
			}
			else
			    {
		fprintf(stderr, "INTU: *** Unknown eventtype value for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return 1;
			    }
			    
			status = IN_code_operation_invoke(ic_dlg_id, &og3_cpt, og1_h);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og1_h) != IN_SUCCESS)
			IN_free_message(og1_h);
	}
	else
	{
		fprintf(stderr, "INTU: *** Failed to encode RRBCSM event invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
		return INTUE_CODING_FAILURE;
	}

	//invokeID++;
	dlg_ptr->current_invoke_id++;
	//			INTU_send_message(intu_mod_id, inap_mod_id, og1_h);
	/**** build and send ApplyCharging ******/
			IN_set_operation(INOP_ApplyCharging, INTU_ReleaseCall_timeout, &og2_cpt);
			IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og2_cpt);
			IN_set_component_param(INPN_MaxCallPeriodDuration, 2, max_call_duration, &og2_cpt); /* max_call_duration here comes as a pointer */
			//	status = IN_set_component_param(INPN_ReleaseIfDurExceeded, 1, apc+1, &og2_cpt);
			if (dlg_calldetails_ptr->quota < 180 ){
				IN_set_component_param(INPN_Tone, 1, Rel_If_Dur_Excd+2, &og2_cpt); //set Tone = False and release if dur exceed automatically enabled
			}
			//status = IN_set_component_param(INPN_RelIfDurEx_Ellipsis, 2, Rel_If_Dur_Excd +1, &og2_cpt);
			IN_set_component_param(INPN_SendingSideID(0), 1, apc+2, &og2_cpt);
			status = IN_code_operation_invoke(ic_dlg_id, &og2_cpt, og2_h);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og2_h) != IN_SUCCESS)
			IN_free_message(og2_h);
	}
	else
	{
		fprintf(stderr, "INTU: *** Failed to encode ApplyCharging event invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
		return INTUE_CODING_FAILURE;
	}

	//			INTU_send_message(intu_mod_id, inap_mod_id, og2_h);
			//			og2_h = IN_alloc_message(0);
	//invokeID++;
	dlg_ptr->current_invoke_id++;
	/**** build and send Continue ******/
			IN_set_operation(INOP_Continue, INTU_ReleaseCall_timeout, &og_cpt);
			IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og_cpt);
			status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og3_h);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og3_h) != IN_SUCCESS)
			IN_free_message(og3_h);
	}
	else
	{
		fprintf(stderr, "INTU: *** Failed to encode Continue event invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
		return INTUE_CODING_FAILURE;
	}

dlg_ptr->current_invoke_id++;
	//		INTU_send_message(intu_mod_id, inap_mod_id, og3_h);
			/*
			* Modified by Elmir to construct TCAP continue instead of 
			* TC-end when send IN reply on received IDP 
			*/
			INTU_send_delimit(ic_dlg_id); /* add this one */

			//			fprintf(stderr, "INTU: *** Далее пытаюсь сменить состояние на OPEN ***\n");
			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN);
			fprintf(stderr, ANSI_COLOR_YELLOW "is_dlg_id %x at the end of handle IDP function" ANSI_COLOR_RESET "\n",ic_dlg_id);
		}
	}

	/*	status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) 
			!= IN_SUCCESS)
			IN_free_message(og_h);
	}
	else
	{
		fprintf(stderr, "INTU: *** Failed to encode invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
		return INTUE_CODING_FAILURE;
	}
	INTU_send_delimit(ic_dlg_id);
	fprintf(stderr, "INTU: *** IDP processing finished & Delimit sent! \n");
	fprintf(stderr, "INTU: *** Dialog Cursor = %i\n", dlg_ptr->cursor);
	*/
	//	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN);
	return 0;
}

/*******************************************************************************/
/* CAMELGW_releasecall_cmd - used to send ReleaseCall invoke to SSF from CAMELGW*/
/*******************************************************************************/
//int CAMELGW_releasecall_cmd(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr) {

//	IN_CPT	og_cpt;			// Holds INAP component whilst building into message 
//	HDR	*og_h;				// Message used to send immediate replies 
//IN_CPT	*dlg_cpt_ptr;
//	int status;
				/* необходимо разорвать установление голосового соединения */
//	dlg_cpt_ptr = &(dlg_ptr->cpt); 

//#ifdef IN_LMSGS
//	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);
//#else
//	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt);
//#endif

//#ifdef IN_LMSGS
//	if ((og_h = IN_alloc_message(0)) == 0)
//#else
//	if ((og_h = IN_alloc_message()) == 0)
//#endif
//	{
//
//		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
//		return INTUE_MSG_ALLOC_FAILED;
//	}
//	IN_set_operation(INOP_ReleaseCall, INTU_ReleaseCall_timeout, &og_cpt);
//IN_set_component_param(INPN_InvokeID,1,invokeid_ptr,&og_cpt);
//	IN_set_component_param (INPN_Cause, sizeof(example_release_cause), example_release_cause, &og_cpt);
//	//IN_set_operation(INOP_ReleaseCall, INTU_ReleaseCall_timeout, &og_cpt);
//				status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);

//	if (status == IN_SUCCESS)
//	{
//		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
//			IN_free_message(og_h);
//	}
//	else
//	{
//		fprintf(stderr, "INTU: *** Failed to encode ReleaseCall event invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
//		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
//		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
//		return INTUE_CODING_FAILURE;
//	}
//
//	//			if (status == IN_SUCCESS)
//	//			    {
//	//				if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
//	//				    IN_free_message(og_h);
//					//may be send delimit missed here
//	//INTU_send_delimit(ic_dlg_id); /* add this one */
//	INTU_send_close(ic_dlg_id, INAPRM_normal_release);
//					INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
//					return 0;
//					//    }
//			    }
/*******************************************************************************/
/*    CAMELGW_connect_cmd - used to send Connect invoke to SSF from CAMELGW    */
/*******************************************************************************/
int CAMELGW_connect_cmd(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, IN_CPT *buffer_cpt_ptr) {

//int CAMELGW_connect_cmd(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, struct calldetails *dlg_calldetails_ptr) {
    	IN_CPT	og_cpt;
	HDR	*og_h;
	IN_CPT	*dlg_cpt_ptr;
	u8	dest_addr_param[INTU_max_called_party_num_len];  // Dest Addr
	//u8	called_pty_paramz[8];
	int status;
	unsigned char digits_buffer[32];  //one elemen is pair of digits from octets string
	unsigned char digit_buffer[32];   // one element is one digit
	u16 data_length;
	int num_digits;
	//unsigned char tp;
	u8 dest_addr_plen;

	dlg_cpt_ptr = &(dlg_ptr->cpt); 

	u8 example_routing_number[15] = {0xd, 1, 6, 0 ,0}; //d1600 - prefix for CONNECT
	u8 nat_number[]={9,5,0,6,6,5,6,0,0,3};

status = IN_get_component_param(INPN_CalledPartyNumber, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);


	if (status == IN_SUCCESS)
	{

	if (digits_buffer[0] == 0x84 ) /*odd and international called num in IDP */
	    {
		num_digits = (data_length -2)*2 -1 -1; //do not include filler and first 7
	    }

	//unpack_digits(digit_buffer, digits_buffer, 4, num_digits);//(sizeof(called_pty_param)+4));
	unpack_digits(digit_buffer, digits_buffer, 5, num_digits);//offset = 5 because we need not include first 7 into digit_buffer
	//		memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber)); 	

	//	for (i=0;i < num_digits;i++){
	//	    //sprintf(string,"%x",called_pty_dgt_str[i]);
	//	    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;//,string);
	//	}
	}
	else
	    {
		printf("something wrong in connect cmd!\n");
	    }
	
#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt);
#endif

	    /*allocate message buffer for outgoing message(s) from intu module*/
#ifdef IN_LMSGS
		if ((og_h = IN_alloc_message(0)) == 0)
#else
		    if ((og_h = IN_alloc_message()) == 0)
#endif
			{
			    fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
			    return INTUE_MSG_ALLOC_FAILED;
			}

	/*
	* InvokeID received as pointer
	*/
		//		 this should be redone!
		/*		INTU_translate_number(&called_pty,&dest_routing_addr,regime);
		dest_addr_plen = INTU_fmt_called_num(dest_addr_param, sizeof(dest_addr_param), &dest_routing_addr, eventtypebcsm_param[0]);
		tp=bits_from_byte(*called_pty_param,4,3);
		
		fprintf(stderr,ANSI_COLOR_YELLOW "tp %x" ANSI_COLOR_RESET "\n",tp);
		if((tp==0)||(tp==1))
			bits_to_byte(called_pty_paramz,132,0,8);
		else
			return 0;
		np=bits_from_byte(*called_pty_param,0,4);
		fprintf(stderr,ANSI_COLOR_YELLOW "np %x" ANSI_COLOR_RESET "\n",np);
		if ((np==1)||(np==4))
			bits_to_byte(called_pty_paramz+1,16,4,3);
		else
			return 0;
		*/
		dest_addr_param[0]=0x83;
		dest_addr_param[1]=16;
			/*	dest_addr_param[2]=0x1d;
	dest_addr_param[3]=6;
	dest_addr_param[4]=0x90;
	dest_addr_param[5]=0x05;
	dest_addr_param[6]=0x66;
	dest_addr_param[7]=0x65;
	dest_addr_param[8]=0x00;
	dest_addr_param[9]=0x03;*/
		
	//dest_addr_param[0]=152;

		//memcpy(example_routing_number + 5, nat_number, sizeof(nat_number) );
		memcpy(example_routing_number + 5, digit_buffer, num_digits );//construct destination number from D1600 and digit_buffer
		pack_digits(dest_addr_param, 4 , example_routing_number, sizeof (example_routing_number)); //4 - offset
		dest_addr_plen = 10;
		IN_set_component_param(INPN_InvokeID,1,invokeid_ptr,&og_cpt);
		IN_set_component_param(INPN_OriginalCalledPartyID, data_length, digits_buffer, &og_cpt);
		IN_set_component_param(INPN_DestinationRoutingAddress(0), dest_addr_plen, dest_addr_param, &og_cpt);
		IN_set_operation(INOP_Connect, INTU_Connect_timeout, &og_cpt);

		status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);

		if (status == IN_SUCCESS)
		{
			if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
				IN_free_message(og_h);
		}
		else
		{
			fprintf(stderr, "INTU: *** Failed to encode invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
			INTU_send_close(ic_dlg_id, INAPRM_normal_release);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			return INTUE_CODING_FAILURE;
		}
		/*
		* when intu send Connect we should close dialogue 
		* in prearranged end mode 
		*/
		INTU_send_delimit(ic_dlg_id);
		INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
		INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
		/* 
		* added by Elmir - need this one to send message 
		* to message queye for sip_connect_logger module 
		*/
		//LOG_MSG message; 
		//message - structure of type msg_to_logger
		//message.msg_type =1;
		//message.msg_time = time(NULL); //get current time
		//sendMessage(&message, sizeof(message) - sizeof(long));

    return 0;
}
/******* vpn service Connect command ********/
int CAMELGW_connect_vpn_cmd(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, IN_CPT *buffer_cpt_ptr) {

    	IN_CPT	og_cpt;
	HDR	*og_h;
	IN_CPT	*dlg_cpt_ptr;
	u8	dest_addr_param[INTU_max_called_party_num_len];  // Dest Addr
	//u8	called_pty_paramz[8];
	int status;
	unsigned char digits_buffer[32];  //one elemen is pair of digits from octets string
	unsigned char digit_buffer[32];   // one element is one digit
	u16 data_length;
	int num_digits;
	struct calldetails *dlg_calldetails_ptr; 
	u8 dest_addr_plen;

	dlg_cpt_ptr = &(dlg_ptr->cpt); 

	dlg_calldetails_ptr = &(dlg_ptr->call_details);

	printf("vpn_driver: pointer 1 = %p, pointer 2 = %p\n", dlg_cpt_ptr, dlg_calldetails_ptr);
	//u8 example_routing_number[15] = {0xd, 1, 6, 0 ,0}; //d1600 - prefix for CONNECT
	//u8 nat_number[]={9,5,0,6,6,5,6,0,0,3};
	//u8 generic_num[] = {0x06, 0x82, 0x10, 0x0b, 0x01, 0x02};

	u8 generic_num_length = 6;

status = IN_get_component_param(INPN_CalledPartyBCDNumber, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

	if (status == IN_SUCCESS)
	{
	    /*****/
	
	    //	if (status == IN_SUCCESS)
	    // {
		num_digits = (data_length - 1) * 2;
		//	    unpack_digits(called_pty_dgt_str, digits_buffer, 2, num_digits ); //offset goes from 2 octet 
		//	unpack_digits(digit_buffer, digits_buffer, 2, num_digits ); //offset goes from 2 octet 

	    /******/

		//if (digits_buffer[0] == 0x84 ) /*odd and international called num in IDP */
		      // {
		//	num_digits = (data_length -2)*2 -1 -1; //do not include filler and first 7
		// }
	//	if(called_pty_dgt_str[0] == INTERNATIONAL_NUMBER){
	//		start_str=5; end_str=14; 
	//	}
	//unpack_digits(digit_buffer, digits_buffer, 4, num_digits);//(sizeof(called_pty_param)+4));
	//	unpack_digits(digit_buffer, digits_buffer, 5, num_digits);//offset = 5 because we need not include first 7 into digit_buffer
	unpack_digits(digit_buffer, digits_buffer, 2, num_digits ); //offset goes from 2 octet 

	//		memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber)); 	

	//	for (i=0;i < num_digits;i++){
	//	    //sprintf(string,"%x",called_pty_dgt_str[i]);
	//	    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;//,string);
	//	}
	}
	else
	    {
		printf("vpn_driver: something wrong in connect cmd!\n");
		
	    }
	
#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt);
#endif

	    /*allocate message buffer for outgoing message(s) from intu module*/
#ifdef IN_LMSGS
		if ((og_h = IN_alloc_message(0)) == 0)
#else
		    if ((og_h = IN_alloc_message()) == 0)
#endif
			{
			    fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
			    return INTUE_MSG_ALLOC_FAILED;
			}

	/*
	* InvokeID received as pointer
	*/
		//		 this should be redone!
		/*		INTU_translate_number(&called_pty,&dest_routing_addr,regime);
		dest_addr_plen = INTU_fmt_called_num(dest_addr_param, sizeof(dest_addr_param), &dest_routing_addr, eventtypebcsm_param[0]);
		tp=bits_from_byte(*called_pty_param,4,3);
		
		fprintf(stderr,ANSI_COLOR_YELLOW "tp %x" ANSI_COLOR_RESET "\n",tp);
		if((tp==0)||(tp==1))
			bits_to_byte(called_pty_paramz,132,0,8);
		else
			return 0;

		fprintf(stderr,ANSI_COLOR_YELLOW "np %x" ANSI_COLOR_RESET "\n",np);
		if ((np==1)||(np==4))
			bits_to_byte(called_pty_paramz+1,16,4,3);
		else
			return 0;
		*/
		dest_addr_param[0]=0x83;
		dest_addr_param[1]=16;
			/*	dest_addr_param[2]=0x1d;
	dest_addr_param[3]=6;
	dest_addr_param[4]=0x90;
	dest_addr_param[5]=0x05;
	dest_addr_param[6]=0x66;
	dest_addr_param[7]=0x65;
	dest_addr_param[8]=0x00;
	dest_addr_param[9]=0x03;*/
	       
	//dest_addr_param[0]=152;

data_length = 5;

digits_buffer[0] = 0x82;
digits_buffer[1] = 0x10;
digits_buffer[2] = 0x0c;
digits_buffer[3] = 0x83;
digits_buffer[4] = 0xf7;
		//memcpy(example_routing_number + 5, nat_number, sizeof(nat_number) );
		//memcpy(example_routing_number + 5, digit_buffer, num_digits );//construct destination number from D1600 and digit_buffer
		//pack_digits(dest_addr_param, 4 , example_routing_number, sizeof (example_routing_number)); //4 - offset
		dest_addr_plen = 10;
		//		generic_number_length= 6;
		IN_set_component_param(INPN_InvokeID,1,invokeid_ptr,&og_cpt);
		IN_set_component_param(INPN_OriginalCalledPartyID, data_length, digits_buffer, &og_cpt);
		//IN_set_component_param(INPN_DestinationRoutingAddress(0), dest_addr_plen, dest_addr_param, &og_cpt);
		IN_set_component_param(INPN_DestinationRoutingAddress(0), dest_addr_plen, &dlg_calldetails_ptr->param2[0], &og_cpt);
		//IN_set_component_param(INPN_GenericNumber(1), generic_num_length, generic_num, &og_cpt);
		IN_set_component_param(INPN_GenericNumber(1), generic_num_length, &dlg_calldetails_ptr->param1[0], &og_cpt);
		IN_set_operation(INOP_Connect, INTU_Connect_timeout, &og_cpt);

		status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);

		if (status == IN_SUCCESS)
		{
			if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
				IN_free_message(og_h);
		}
		else
		{
			fprintf(stderr, "INTU: *** Failed to encode invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
			INTU_send_close(ic_dlg_id, INAPRM_normal_release);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			return INTUE_CODING_FAILURE;
		}
		/*
		* when intu send Connect we should close dialogue 
		* in prearranged end mode 
		*/
		INTU_send_delimit(ic_dlg_id);
		INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
		INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
		/* 
		* added by Elmir - need this one to send message 
		* to message queye for sip_connect_logger module 
		*/
		//LOG_MSG message; 
		//message - structure of type msg_to_logger
		//message.msg_type =1;
		//message.msg_time = time(NULL); //get current time
		//sendMessage(&message, sizeof(message) - sizeof(long));

    return 0;
}

int CAMELGW_get_data_from_ora(struct calldetails *ptr) {

    static unsigned long long counter = 0; /*each time when we invoke get data from ora fucntion this counter incremented */
    int n, ret;
    fd_set readfds;
    struct timeval tv;
    //    struct oracle_iface_data oracle_calldetails;

 /*    memcpy(&oracle_calldetails.CallReferenceNumber[0], &ptr->CallReferenceNumber[0], 17); */
 /*   memcpy(&oracle_calldetails.CallingPartyNumber[0], &ptr->CallingPartyNumber[0], 16); */
 /*   memcpy(&oracle_calldetails.RedirectingNumber[0], &ptr->RedirectingNumber[0], 16); */
 /*   memcpy(&oracle_calldetails.LocationNumber[0], &ptr->LocationNumber[0], LOC_NUM_MAX_DIGITS_STR); */
 /*   memcpy(&oracle_calldetails.IMSI[0], &ptr->IMSI[0], IMSI_MAX_DIGITS_STR); */
 /*  memcpy(&oracle_calldetails.VLR[0], &ptr->VLR[0], 16);   */
 /*  memcpy(&oracle_calldetails.MSC[0], &ptr->MSC[0], 16); */
 /*  memcpy(&oracle_calldetails.CalledPartyNumber[0], &ptr->CalledPartyNumber[0], 19); */
 /*  memcpy(&oracle_calldetails.CallingPartysCategory[0], &ptr->CallingPartysCategory[0], 5); */
 /*  memcpy(&oracle_calldetails.TimeAndTimezone[0], &ptr->TimeAndTimezone[0], 17); */
 /*  memcpy(&oracle_calldetails.CountryCodeA[0], &ptr->CountryCodeA[0], 17); */
 /*  memcpy(&oracle_calldetails.ServiceKey[0], &ptr->ServiceKey[0], 2); */
 /* memcpy(&oracle_calldetails.EventTypeBCSM[0], &ptr->EventTypeBCSM[0], 3); */
 /* memcpy(&oracle_calldetails.CountryCodeB[0], &ptr->CountryCodeB[0], 19); */
 /* memcpy(&oracle_calldetails.CELLID[0], &ptr->CELLID[0], 25); */
 /* oracle_calldetails.leg_type = ptr->leg_type; */
 /* oracle_calldetails.op_code = ptr->op_code; //what we should ask from oracle */
 /* oracle_calldetails.call_id = ptr->call_id; */
 /* //      duration; */

    //  unsigned long long id; //identity for processing timeouted replys from billing
 //    unsigned int quota;
 
 // int status_code;

    FD_ZERO(&readfds);
    FD_SET(socket_d, &readfds);

    tv.tv_sec = 2; /* 2 seconds timeout for waiting data from ora_cli process */
    tv.tv_usec = 0;

    //ptr->id = counter;
    //    oracle_calldetails.id = counter;
    //counter++;
    
 /*    switch (oracle_calldetails.op_code) { */

 /*    case CAMELGW_ORACLE_INITIAL_QUOTA_REQ: */
 /* 	//	n = sendn(socket_d, ptr, sizeof(struct calldetails), MSG_NOSIGNAL); */
 /* 	n = sendn(socket_d, &oracle_calldetails , sizeof(struct oracle_iface_data), MSG_NOSIGNAL); */
 /* 	if (n == sizeof(struct oracle_iface_data)) */
 /* 	    printf("oracle: camelgw_get_data_from_ora: data send from idp handler to oracle cli = %d\n ", n); */
 /* 	else */
 /* 	    printf("oracle: error n = %d\n", n); /\*TODO: should process return error value *\/ */
 /* 	/\***********************************************************************\/ */
 /*    while (tv.tv_sec != 0) { */
 /* 	ret = select (socket_d + 1, &readfds, NULL, NULL, &tv); */
 /* 	if (ret == -1) { */
 /* 	    perror("select"); */
 /* 	    return 1; */
 /* 	} else if (!ret) { */
 /* 	    printf("oracle: %d seconds elapsed in case 0 \n", tv.tv_sec); */
 /* 	    ptr->status_code = 995; */
 /* 	    counter++; */
 /* 	    return 0; */
 /* 	} */
 /* if (FD_ISSET(socket_d, &readfds)) { */
 /*          n=recv(socket_d, &oracle_calldetails, sizeof(struct oracle_iface_data), 0); */
 /* 	  if (n) { */
 /* 	      printf("oracle: n received from oracle_driver = %d\n ", n); */
 /* 	  } */
 /* 	    else { */
 /* 	    printf("n received < 0 n = %d\n", n); */
 /* 	    perror("recv:"); */
 /* 	} */
 /*    if (oracle_calldetails.id == counter) */
 /* 	{ */
 /* 	    printf("oracle: received the same data in camel gw get data from ORA!\n"); */
 /* 	    //printf("time elapsed = %d\n", tv.tv_sec); */
 /* 	    counter++; */
 /* 	    //fill calldetails str with results and return */
 /* ptr->call_id = oracle_calldetails.call_id; */
 /* ptr->quota = oracle_calldetails.quota; */
 /* ptr->status_code = oracle_calldetails.status_code; */
 	    
 /* 	    return 0; */
 /* 	} */
 /*    else */
 /* 	{ */
 /* 	    continue; */
 /* 	} */
 /* } */
 /*    } */
 /* 	/\***********************************************************************\/ */
 /*    //	ret = select (socket_d + 1, &readfds, NULL, NULL, &tv); */

 /* 	/\* if (ret == -1) { *\/ */
 /* 	/\*     perror("select"); *\/ */
 /* 	/\*     return 1; *\/ */
 /* 	/\* } else if (!ret) { *\/ */
 /* 	/\*     printf("%d seconds elapsed \n", 2); *\/ */
 /* 	/\*     ptr->status_code = 2; *\/ */
 /* 	/\*     return 5; *\/ */
 /* 	/\* } *\/ */

 /* 	/\* if (FD_ISSET(socket_d, &readfds)) { *\/ */
 /* 	/\* n = recv(socket_d, ptr, sizeof(struct calldetails), 0); *\/ */

 /* 	/\* if (n) *\/ */
 /* 	/\*     printf("n received > 0\n "); *\/ */
 /* 	/\* else { *\/ */
 /* 	/\*     printf("n received < 0 n = %d\n", n); *\/ */
 /* 	/\*     perror("recv:"); *\/ */
 /* 	/\* } *\/ */
 /*    //	} */
 /*  break; */

 /*    case CAMELGW_ORACLE_CALL_END_IND: */

 /*     oracle_calldetails.duration = ptr->duration; */

 /* 	n = sendn(socket_d, &oracle_calldetails, sizeof(struct oracle_iface_data), MSG_NOSIGNAL); */
 /* 	if (n == sizeof(struct oracle_iface_data)) */
 /* 	    printf("camelgw_get_data_from_ora: data send from idp handler to oracle cli = %d\n ", n); */
 /* 	else */
 /*    printf(" error n = %d\n", n); /\*TODO: should process return error value *\/ */

 /*  while (tv.tv_sec != 0) { */
 /* 	ret = select (socket_d + 1, &readfds, NULL, NULL, &tv); */

 /* 	if (ret == -1) { */
 /* 	    perror("select"); */
 /* 	    return 1; */
 /* 	} else if (!ret) { */
 /* 	    printf("%d seconds elapsed \n", tv.tv_sec); */
 /* 	    ptr->status_code = 996; */
 /* 	    counter++; */
 /* 	    return 0; */
 /* 	} */

 /* 	if (FD_ISSET(socket_d, &readfds)) { */
	
 /* 	n = recv(socket_d, &oracle_calldetails, sizeof(struct oracle_iface_data), 0); */

 /* 	if (n) */
 /* 	    printf("n received = %d\n", n); */
 /* 	else { */
 /* 	    printf("n received < 0 n = %d\n", n); */
 /* 	    perror("recv:"); */
 /* 	} */

 /*    if (oracle_calldetails.id == counter) */
 /* 	{ */
 /* 	    printf("oracle: received the same data in camel gw get data from ORA!\n"); */
 /* 	    //printf("time elapsed = %d\n", tv.tv_sec); */
 /* 	    counter++; */
 /* ptr->call_id = oracle_calldetails.call_id; */
 /* ptr->quota = oracle_calldetails.quota; */
 /* ptr->status_code = oracle_calldetails.status_code; */

 /* 	    return 0; */
 /* 	} */
 /*    else */
 /* 	{ */
 /* 	    continue; */
 /* 	} */
 /*    	} */
 /*  } */
	
 /* 	break; */

 /*    case CAMELGW_ORACLE_NEXT_QUOTA_REQ: */

 /* 	n = sendn(socket_d, &oracle_calldetails, sizeof(struct oracle_iface_data), MSG_NOSIGNAL); */
 /* 	if (n == sizeof(struct oracle_iface_data)) */
 /* 	    printf("intu_handle_idp: data send from idp handler to oracle cli = %d\n ", n); */
 /* 	else */
 /* 	    printf(" error n = %d\n", n); /\*TODO: should process return error value *\/ */

 /*  while (tv.tv_sec != 0) { */
 /* 	ret = select (socket_d + 1, &readfds, NULL, NULL, &tv); */

 /* 	if (ret == -1) { */
 /* 	    perror("select"); */
 /* 	    return 1; */
 /* 	} else if (!ret) { */
 /* 	    printf("%d seconds elapsed \n", tv.tv_sec); */
 /* 	    ptr->status_code = 997; */
 /* 	    counter++; */
 /* 	    return 0; */
 /* 	} */

 /* 	if (FD_ISSET(socket_d, &readfds)) { */
	
 /* 	n = recv(socket_d, &oracle_calldetails, sizeof(struct oracle_iface_data), 0); */

 /* 	if (n) */
 /* 	    printf("n received > 0\n "); */
 /* 	else { */
 /* 	    printf("n received < 0 n = %d\n", n); */
 /* 	    perror("recv:"); */
 /* 	} */
 /* if (ptr->id == counter) */
 /* 	{ */
 /* 	    printf("received the same data in camel gw get data from ORA!\n"); */
 /* 	    counter++; */
 /* ptr->call_id = oracle_calldetails.call_id; */
 /* ptr->quota = oracle_calldetails.quota; */
 /* ptr->status_code = oracle_calldetails.status_code; */

 /* 	    return 0; */
 /* 	} */
 /*    else */
 /* 	{ */
 /* 	    continue; */
 /* 	} */
   
 /* 	} */
 /* 	} */
 /* 	break; */
 /*    default: */
 /* 	printf("oracle: some error here!"); */
 /* 	break; */
    //}
    return 0;

}


/* ssize_t                         /\* send "n" bytes to a descriptor. *\/ */
/* sendn(int fd, const void *vptr, size_t n) */
/* { */
/*     size_t nleft; */
/*     ssize_t nsent; */
/*     const char *ptr; */

/*     ptr = vptr; */
/*     nleft = n; */
/*     while (nleft > 0) { */
/* 	if ( (nsent = send(fd, ptr, nleft, MSG_NOSIGNAL)) <= 0) { */
/* 	    if (nsent < 0 && errno == EINTR) */
/* 		nsent = 0;   /\* and call send() again *\/ */
/*             else */
/* 		return (-1);    /\* error *\/ */
/*          } */
/* 	nleft -= nsent; */
/* 	ptr += nsent; */
/* 	} */
/*    return (n); */
/* } */

/*********************************************************************************************************/
/*      dirver id = socket id of connected to driver unix domain socket   */
/*********************************************************************************************************/

/* ssize_t                         /\* send "n" bytes to a socket descriptor. *\/ */
/* sendn_test(int fd, const void *vptr, size_t n) {  */
/*     size_t nleft; */
/*     ssize_t nsent; */
/*     const char *ptr; */

/*     ptr = vptr; */
/*     nleft = n; */
/*     while (nleft > 0) { */

/* 	nsent = send(fd, ptr, nleft, MSG_NOSIGNAL); */

/* 		if ( nsent <= 0) { */
/* 	    //	printf("nsent = %d\n", nsent); */
/* 	    //		perror("send"); */
/* 		// if (nsent < 0 && errno == EPIPE) { */
/* 		//		printf("nsent = %d\n", nsent); */
/* 		//perror("send"); */
/* 		//	return EPIPE;   /\* and call send() again *\/ */

/* 		// } */

/* 	    if (nsent < 0 && errno == EINTR) { */
/* 		//		printf("nsent = %d\n", nsent); */
/* 		//perror("send"); */
/* 		nsent = 0;   /\* and call send() again *\/ */

/* 	    } */
/*             else */
/* 		return errno;    /\* error *\/ */
/*          } */
/* 	nleft -= nsent; */
/* 	ptr += nsent; */
/* 	} */
/*    return (n); */
/* } */

/* int connect_retry(int sockfd, const struct sockaddr *addr, socklen_t alen) { */
/*     int nsec; */
/*     /\* */
/*      * Попытаться установить соединение с экспоненциальной задержкой. */
/*      *\/ */
/*     for (nsec = 1; nsec <= MAXSLEEP; nsec <<= 1) { */
/* 	printf("trying to connect .....\n"); */
/* 	if (connect(sockfd, addr, alen) == 0) { */
/* 	    /\* */
/* 	     * Соединение установлено. */
/* 	     *\/ */
/* 	    return 0; */
/* 	} */
/* 	/\* */
/* 	 * Задержка перед следующей попыткой. */
/* 	 *\/ */
/* 	if (nsec <= MAXSLEEP/2) */
/* 	    sleep(nsec); */
/*     } */
/*     return (-1); */
/* } */
