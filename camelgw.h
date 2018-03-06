#ifndef CAMELGW_H
#define CAMELGW_H

#define LOC_NUM_MAX_DIGITS_STR 33 //33 digits + 1 for null term
#define IMSI_MAX_DIGITS_STR 16 //15 digist for IMSI and 1 for null-term
#define SUB_ID_MAX_DIGITS_STR 20 //11 ==//10 digits for sub_id and 1 for null term it shuld be 11 but we reserve 20 digits for something goes wrong

//TODO - need add status2 element - for signalling what happend with oracle side to intu process


struct called_pty_addr {
    unsigned char num_type; //called, called_bcd, or calling? or another?
	unsigned char    bcd_exti;           /* extension: 1=no_extension */
	unsigned char    bcd_toni;           /* type of number, 0001b - international number */
	unsigned char    both_npi;            /* Numbering plan indicator 0001b ISDN E164 */
    unsigned char oddi; //odd-even indicator in calledpartynumber
    unsigned char noai; //nature of address indicator, used in CalledParty, DP12
    unsigned char inni;
    unsigned char    naddr;          /* number of address digits , this is our element not exist in standart calledBCDnumber*/
    unsigned char    addr[15]; //array of unpacked address digits
};

struct calling_pty_addr {
    unsigned char num_type; //called, called_bcd, or calling? or another?
    unsigned char oddi;
    unsigned char noai;//nature of address indicator 
    unsigned char ni;            /* Number incomplete indicator, 0 = complete */
    unsigned char npi;      //numbering plan indicator 
    unsigned char apri;     //adress presentation restricted indicator
    unsigned char si;       //screening indicator
    unsigned char naddr;     /* number of address digits , this is our element not exist in standart calledBCDnumber*/
    unsigned char addr[15]; //mixed pairs of digits, not unpacked yet
};

struct redirecting_pty_addr {
        //need to fill
    unsigned char oddi;
    unsigned char noai;//nature of address indicator 
    //    unsigned char ni;            /* Number incomplete indicator, 0 = complete */
    unsigned char npi;      //numbering plan indicator 
    unsigned char apri;     //adress presentation restricted indicator
    //    unsigned char si;       //screening indicator
    unsigned char naddr;     /* number of address digits */
    unsigned char addr[15]; //mixed pairs of digits, not unpacked yet
};

struct subscriber {
    unsigned char num_length; //how many digits in sub id, should be 10 for national format sub id, we use national format
    char sub_msisdn[SUB_ID_MAX_DIGITS_STR];  //sub id goes as string with null-term, example 9027111287,it is should be like this, but we reserve 20 digits at all
};


struct calldetails {
    //  unsigned long long ull_Subscriber_id[16];
    char Subscriber_id[SUB_ID_MAX_DIGITS_STR];  //subscriber id goes as string with null-term, example 9027111287
    struct subscirber subscriber_id;
    unsigned char forwarding_pending;
    unsigned char camel_sub_state;
    char CallReferenceNumber[17]; //call reference is 8 octets total
    unsigned long long ull_CallReferenceNumber;
    char CallingPartyNumber[16]; // 15 max digits in calling and +1 for null term
    char RedirectingNumber[16];
    char LocationNumber[LOC_NUM_MAX_DIGITS_STR];
    char IMSI[IMSI_MAX_DIGITS_STR];
    unsigned long long ull_IMSI;
    char VLR[16];
    char MSC[16];
    char CalledPartyNumber[19]; // 15 - max for msisdn 1 for f filler and 1 for null-term 0
    char CallingPartysCategory[5];
    char TimeAndTimezone[17];
    char CountryCodeA[17];
    char ServiceKey[2];
    unsigned char uc_ServiceKey;
    char EventTypeBCSM[3];
    unsigned char uc_EventTypeBCSM;
    char CountryCodeB[19];
    char CELLID[25];
    int leg_type;          //mo, mf, mt or mtr leg
    unsigned int id; //identity for processing timeouted replys from billing and also general id
    //    int op_code; //op_code for what we should ask from oracle
    unsigned int quota;
    long long call_id;
    int status_code;
    unsigned char param1[16]; //experimetnal for vpn
    unsigned char param2[16]; //experimental for vpn
    int duration;
    struct calling_pty_addr CallingAddr;
    struct called_pty_addr CalledAddr;
    struct redirecting_pty_addr RedirectingAddr;
    //int id;
};

#endif
