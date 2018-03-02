

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


#include "intu_def.h"
#include "camelgw_dnc.h"
#include "camelgw_utils.h"


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

    /*in called_party_bcd there is no info about odd/even indicator but we already fill it in this function because we now number of digits */


	if( ( (p_called_num->naddr) & 1 ) == 0)
	     p_called_num->oddi = 0; /*even*/
	else  p_called_num->oddi = 1; /*odd */


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



static int fill_subscriber_id(struct calldetails *dlg_calldetails_ptr)
{
    int i;
    struct calling_pty_addr *pso_CallingAddr = &dlg_calldetails_ptr->CallingAddr; 
    struct called_pty_addr *pso_CalledAddr = &dlg_calldetails_ptr->CalledAddr; 
    char *pc_Subscriber_id = &dlg_calldetails_ptr->Subscriber_id[0];

    //struct calling_pty_addr *pso_CallingAddr,  char *pc_CallingPartyNumber)

    switch(dlg_calldetails_ptr->leg_type)

	{ 
	case 0: /*pure MO leg */
	    //copy calling to subscriber ID as national (as in IDP in first variant of code)
    //	box.space.camel_services:update({'79027111287'}, {{'=', 2, 'vip'}})
    //int i;
	    for (i=0;i < pso_CallingAddr->naddr;i++)
		{
		    //dlg_calldetails_ptr->CallingPartyNumber[i] = digit_buffer[i] + 0x30;
		    *(pc_Subscriber_id++) = pso_CallingAddr->addr[i] + 0x30;			
		}
	    
	    *(pc_Subscriber_id) = '\0'; //add term null for end of string
	    
	    if (dlg_calldetails_ptr->Subscriber_id[0] == '7')
		camelgw_dnc_remove7(&dlg_calldetails_ptr->Subscriber_id[0]); 

  //	    memcpy(&dlg_calldetails_ptr->Subscriber_id[0], &dlg_calldetails_ptr->CallingPartyNumber[0], 16);
	    break;
	case 1: /*MF leg spawned from MSC */

    break;

case 2:

	    for (i=0;i < pso_CalledAddr->naddr;i++)
		{
		    //dlg_calldetails_ptr->CallingPartyNumber[i] = digit_buffer[i] + 0x30;
		    *(pc_Subscriber_id++) = pso_CalledAddr->addr[i] + 0x30;			
		}
	    
	    *(pc_Subscriber_id) = '\0'; //add term null for end of string
	    
	    if (dlg_calldetails_ptr->Subscriber_id[0] == '7')
		camelgw_dnc_remove7(&dlg_calldetails_ptr->Subscriber_id[0]); 

	    break;

case 3: break;

	default:

	    break;
}
		    return 0;
}


int idp_parser(IN_CPT *buffer_cpt_ptr, 	struct calldetails *dlg_calldetails_ptr)
{
    int status;
    int i;
	unsigned char digits_buffer[32];  //one elemen is pair of digits from octets string
	unsigned char digit_buffer[32];   // one element is one digit
	u16 data_length;
	int num_digits;


	struct calling_pty_addr *pso_CallingAddr;
	struct called_pty_addr *pso_CalledAddr;
	struct redirecting_pty_addr *pso_RedirectingAddr;


	    memset(dlg_calldetails_ptr->IMSI, 0, sizeof(dlg_calldetails_ptr->IMSI));   
	    memset(dlg_calldetails_ptr->LocationNumber, 0, sizeof(dlg_calldetails_ptr->LocationNumber));    
	    memset(dlg_calldetails_ptr->CallingPartyNumber, 0, sizeof(dlg_calldetails_ptr->CallingPartyNumber)); 	
            memset(dlg_calldetails_ptr->RedirectingNumber, 0, sizeof(dlg_calldetails_ptr->RedirectingNumber));
            memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber));
	    memset(dlg_calldetails_ptr->EventTypeBCSM, 0, sizeof(dlg_calldetails_ptr->EventTypeBCSM));
	    memset(dlg_calldetails_ptr->CountryCodeA, 0, sizeof(dlg_calldetails_ptr->CountryCodeA));
	    //memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber));
	    memset(dlg_calldetails_ptr->CountryCodeB, 0, sizeof(dlg_calldetails_ptr->CountryCodeB));


/************************************************************************/
/* IMSI processing */
/*put IMSI as ull value in calldetails struct and also as string in calldetails structure */
	status = IN_get_component_param(INPN_IMSI, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
     	if (status == IN_SUCCESS)
	    {
		num_digits = data_length  * 2 - 1; //imsi = 15 digits
		unpack_digits(digit_buffer, digits_buffer, 0, num_digits);
		//		memset(dlg_calldetails_ptr->IMSI, 0, sizeof(dlg_calldetails_ptr->IMSI));   
		dlg_calldetails_ptr->ull_IMSI = arrtonum(&digit_buffer[0], 15);
		//		printf("info: IMSI as ull = %llu\n", 	dlg_calldetails_ptr->ull_IMSI );
		for (i=0;i < num_digits;i++)
		    {
			dlg_calldetails_ptr->IMSI[i] = digit_buffer[i] + 0x30;
		    }
	    }
	else
	    {
		printf("debug: error in IMSI getting!\n");
	    }
	/**************************************IMSI processinf finished ******/

/******************* CallReferenceNumber processing ********************************************/

	memset(digits_buffer, 0, 8); //max size of call ref number is 8 octets    
	status = IN_get_component_param (INPN_CallReferenceNumber, &data_length, digits_buffer, sizeof(digits_buffer),buffer_cpt_ptr);

	if (status == IN_SUCCESS)
	{
	    //memset(dlg_calldetails_ptr->CallReferenceNumber, 0, sizeof(dlg_calldetails_ptr->CallReferenceNumber));    
	    memset(dlg_calldetails_ptr->CallReferenceNumber, 0, sizeof(dlg_calldetails_ptr->CallReferenceNumber));    
	    for (i=0; i< data_length;i++)
		{
		    sprintf(dlg_calldetails_ptr->CallReferenceNumber + 2*i,"%02x",digits_buffer[i]);//fill by 0 to 2 znaks
		}
	    printf("info:call ref as string = %s\n", dlg_calldetails_ptr->CallReferenceNumber);
	    reversearray(digits_buffer, data_length);
	    dlg_calldetails_ptr->ull_CallReferenceNumber =  (* (unsigned long long *) &digits_buffer[0]);
	    printf("info:call_ref_num as ull = %llu\n",  dlg_calldetails_ptr->ull_CallReferenceNumber );
	    //	    memset(dlg_calldetails_ptr->CallReferenceNumber, 0, sizeof(dlg_calldetails_ptr->CallReferenceNumber));    
	    //for (i=0; i< data_length;i++){
	    //		sprintf(dlg_calldetails_ptr->CallReferenceNumber + 2*i,"%02x",digits_buffer[i]);//fill by 0 to 2 znaks
	    // }
	}
 else
     {
	 printf("debug: error getting CallRefernce from IDP\n");
     }
/******************** end of CallReferenceNumber processing *************************************/


/*	* we need analyze Calling Party Number and also Called Party Number here, http GET 
	* to sip server should contain Calling Party Number and Called Party Number
	* calling party number in MO in home always NATIONAL and in guest plmn in MO call alwasy INTERNATIONAL
	* tmt billing works with national NUMBER 
	*/
	status = IN_get_component_param(INPN_CallingPartyNumber, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

	if (status == IN_SUCCESS)
	{
	    pso_CallingAddr = &dlg_calldetails_ptr->CallingAddr;
   CAMELGW_rec_calling_num( pso_CallingAddr, digits_buffer, data_length );

//   printf("info:calling: oddi=%d;noai=%d;ni=%d;npi=%d;apri=%d;si=%d;naddr=%d;digits = ", pso_CallingAddr->oddi, pso_CallingAddr->noai, pso_CallingAddr->ni, pso_CallingAddr->npi, pso_CallingAddr->apri, pso_CallingAddr->si, pso_CallingAddr->naddr);

//	    for (i=0; i<pso_CallingAddr->naddr; i++)
// 		printf("info: %x", pso_CallingAddr->addr[i]);
//  	    printf("\n");
	}
	else
	    {
		printf("debug: error to get calling pty!\n");
}
/******************** end of CallingPartyNumber processing *************************************/

/******************* MSCAddress processing ******************************************************/

	/****** first we need MSC GT from IDP****** idea from 05102017 - to analyze type of CF - early or late **********/
	// MSC GT handled like CalledPartyBCDnumber
 status = IN_get_component_param(INPN_MSCAddress, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
 
 if (status == IN_SUCCESS)
     {
	 num_digits = (data_length - 1) * 2;
	 unpack_digits(digit_buffer, digits_buffer, 2, num_digits);
	 if (digits_buffer [0] == 145) /*ton and npi = 0x91 = international number*/
	     {
		 memset(dlg_calldetails_ptr->MSC, 0, sizeof(dlg_calldetails_ptr->MSC)); 	
		 for (i=0;i < num_digits;i++) {                            // -1 here because of odd number last digits it is just filler
		     dlg_calldetails_ptr->MSC[i] = digit_buffer[i] + 0x30;
		 }
		 
		 if (dlg_calldetails_ptr->MSC[num_digits-1] == '?')
		     {
			 dlg_calldetails_ptr->MSC[num_digits-1] = '\0';
			 num_digits--;
		     }
	     }
	 else
	     printf("some error with MSC GT in IDP\n");
     }
 else
     printf("error: couldnt get MSC from IDP\n");
 /****** MSC processinf end*****/


/*time and timezone getting from IDP */

status = IN_get_component_param(INPN_TimeAndTimezone, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

 if (status == IN_SUCCESS)
	{
	    num_digits = data_length * 2;
	    unpack_digits(digit_buffer, digits_buffer, 0, num_digits);//sizeof(digits_buffer));

		memset(dlg_calldetails_ptr->TimeAndTimezone, 0, sizeof(dlg_calldetails_ptr->TimeAndTimezone));    
		for (i=0; i< num_digits;i++){
			dlg_calldetails_ptr->TimeAndTimezone[i] = digit_buffer[i] + 0x30;
		}

	}
	else
	    printf("error getting timezone from IDP component\n");


 //there may be two Location Number in IDP. 1 - standalone and 2 in LocationInformation group.
 // we use standalone Location Number, received from ISUP for example in case of MT calls to Letai subs
 //check if LocationNumber valid for DP2 (MO case)
status =  IN_get_component_param(INPN_LocationNumber, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
 if (status == IN_SUCCESS)
     {
	 num_digits = (data_length-2) * 2;
	 unpack_digits(digit_buffer, digits_buffer,  4,  num_digits);
	 
	 //	 memset(dlg_calldetails_ptr->LocationNumber, 0, sizeof(dlg_calldetails_ptr->LocationNumber));    
	
	 if ( num_digits >= (LOC_NUM_MAX_DIGITS_STR-1) )
	     {
	     num_digits = LOC_NUM_MAX_DIGITS_STR-1;
	     //TODO! should log this!
	     }
	 for (i=0; i< num_digits;i++)
	    {
			dlg_calldetails_ptr->LocationNumber[i] = digit_buffer[i] + 0x30;
		}
     }
 else
     {
	 printf("error getting LocationNumber from IDP, use dummy!!! \n");
	 //	 memset(dlg_calldetails_ptr->LocationNumber, 0, sizeof(dlg_calldetails_ptr->LocationNumber));
	 dlg_calldetails_ptr->LocationNumber[0] =  0x30; //dummy LocationNumber when in IDP there is no LocationNumber
     }



	/* 
	 * should define type of VLR - subscriber in home  VLR or roamed in guest VLR
	 VLR number mandatory for MO, conditional for MT and shouldn't exist in CF, also VLR not exist if HLR doesn't have info about it - ME power off
	*/

	memset(dlg_calldetails_ptr->VLR, 0, sizeof(dlg_calldetails_ptr->VLR));

	status = IN_get_component_param(INPN_Vlr_Number, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
	
	//	num_digits = (data_length - 1) * 2;  //we have no info about odd or even - so we also insert filler "f" in num digits number
	
	if (status == IN_SUCCESS)
	    {
		num_digits = (data_length - 1) * 2;  //we have no info about odd or even - so we also insert filler "f" in num digits number
		//		event_flags = event_flags | (unsigned short) event_VLR_Y; 		
		if (digits_buffer[0] == 0x91) {
		    unpack_digits(digit_buffer, digits_buffer, 2, num_digits);
		    //memset(dlg_calldetails_ptr->VLR, 0, sizeof(dlg_calldetails_ptr->VLR)); 	
		    
		    for (i=0;i < num_digits;i++) {                            // -1 here because of odd number last digits it is just filler
			dlg_calldetails_ptr->VLR[i] = digit_buffer[i] + 0x30;
		    }
		    
		    if (dlg_calldetails_ptr->VLR[num_digits-1] == '?'){
			dlg_calldetails_ptr->VLR[num_digits-1] = '\0';
			//num_digits--;
		    }
		}
		else
		    printf("debug: error wrong VLR format\n");
		//should LOG this	    
	    }
	else
	    {
		printf("debug: error getting VLR from IDP\n");
		//	event_flags = event_flags | (unsigned short) event_VLR_N; 		
	    }

status = IN_get_component_param(INPN_CallingPartysCategory, &data_length,digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

 if (status == IN_SUCCESS)
		 {

		memset(dlg_calldetails_ptr->CallingPartysCategory, 0, sizeof(dlg_calldetails_ptr->CallingPartysCategory)); 		
		sprintf(dlg_calldetails_ptr->CallingPartysCategory, "%i", digits_buffer[0]);

}
 else
     {

}

 /******************* EventTypeBCSM processing ******************************************************/

 status = IN_get_component_param (INPN_EventTypeBCSM(0), &data_length, digits_buffer, sizeof(digits_buffer),buffer_cpt_ptr);	

 if (status == IN_SUCCESS)
     {
	 
	 dlg_calldetails_ptr->uc_EventTypeBCSM = digits_buffer[0];
	 printf("info: EventTypeBCSM as uc = %d\n", dlg_calldetails_ptr->uc_EventTypeBCSM);
     }
 else
     {
	 
     }

// memset(dlg_calldetails_ptr->EventTypeBCSM, 0, sizeof(dlg_calldetails_ptr->EventTypeBCSM));
 sprintf(dlg_calldetails_ptr->EventTypeBCSM, "%i", digits_buffer[0]);

	/******************************** type of service definition ***************************************************************************/
	switch (dlg_calldetails_ptr->uc_EventTypeBCSM)
	    {
	    case 2: /*MO leg or MO-CF leg */


		//CalledPartyBCDNumber exist in case DP2
                //memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber));
                //memset(dlg_calldetails_ptr->CountryCodeB, 0, sizeof(dlg_calldetails_ptr->CountryCodeB));
		/**************** when DP = 2, then in MOC called number get from CalledNumberBCDNumber param in IDP  *****/
		/**************** when DP = 2, then in MF called number get from CalledPartyNumber param in IDP  *****/
		/*
		  called party bcd number 
		  3gpp ts 24.008
		  octet 1 - bit1-bit4 :numbering plan identification
		  octet 1 - bit5-bit7 :type of number
		  octet 1 - bit8      :extension
		  octet 2 and next - bcd coded digits
		  if nummer of bcd digits is odd then last octet bit5 to bit8 filled by 1111 filler
		 */
		status = IN_get_component_param(INPN_CalledPartyBCDNumber, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
		if (status == IN_SUCCESS) /*seems to be MOC, exactly MOC! */ 
		    {
			dlg_calldetails_ptr->leg_type = 0;

			pso_CalledAddr = &dlg_calldetails_ptr->CalledAddr;
			CAMELGW_rec_called_num(pso_CalledAddr, digits_buffer, data_length, BCD_CALLED);


			num_digits = (data_length - 1) * 2; /*with filler*/
			unpack_digits(digit_buffer, digits_buffer, 2, num_digits ); //offset goes from 2 octet 

			if (digits_buffer[0] == 145) /*ton and npi = 0x91 international number */
			    {

				for (i=0;i < num_digits;i++) {                            // -1 here because of odd number last digits it is just filler
				    //sprintf(string,"%x",calling_pty_dgt_str[i]);
				    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
				}
				//delete f filler at the end position
				if (dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] == '?') {
				    dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] = '\0';
				    num_digits--;
				}

			    }
			else if (digits_buffer [0] == 129) /* ton and npi = 0x81 = unknown */
			    //		memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber)); 	
			    {
				if (digit_buffer[0] == 11) // means first dialed digit is # (B in hex, * is A in hex or 10dec)
				    {
					//in case when we should release every unknown without 8 at the beginning
					//todo			CAMELGW_releasecall_cmd(ic_dlg_id, dlg_ptr, h, &invokeID);
					//todo return 0;
					dlg_calldetails_ptr->CalledPartyNumber[0] = 35; // char code for #				
					for (i=1;i < ( num_digits-1) ;i++) {                            // -1 here because of odd number last digits it is just filler
					    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
					}
					//delete f filler at the end position
					if (dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] == '?') {
					    dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] = '\0';
					    num_digits--;
					}
				    }
				else if ( (digit_buffer[0] != 11) && (digit_buffer[0] != 8) )
				    {
					//in case when we should release every unknown without 8 at the beginning
					//todo			CAMELGW_releasecall_cmd(ic_dlg_id, dlg_ptr, h, &invokeID);
					//todo return 0;
					for (i=0;i < num_digits;i++) {                            // -1 here because of odd number last digits it is just filler
					    
					    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
					}
					//delete f filler at the end position
					if (dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] == '?') {
					    dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] = '\0';
					    num_digits--;
					}
				    }
				else if ( (digit_buffer[1] == 1) && (digit_buffer[2] == 0)  )
				    {
					num_digits = num_digits - 3;
					for (i=0;i<num_digits;i++) {
					    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i+3] + 0x30;
					}
					if (dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] == '?') {
					    dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] = '\0';
					    num_digits--;
					}
					//	if (dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] == '?') {
					//	    dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] = '\0';
					//	    num_digits--;
					//	}
					//memset(dlg_calldetails_ptr->CountryCodeB, 0, sizeof(dlg_calldetails_ptr->CountryCodeB));
					//memcpy(dlg_calldetails_ptr->CountryCodeB, dlg_calldetails_ptr->CalledPartyNumber, num_digits);
				    }
				else if  (digit_buffer[1] == 9)
				    {
					digit_buffer[0] = 7;
					
					for (i=0;i<num_digits;i++){
					    //sprintf(string,"%x",called_pty_dgt_str[i]);
					    //		strcat(dlg_calldetails_ptr->CalledPartyNumber,string);
					    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
					}
					if (dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] == '?') {
					    dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] = '\0';
					    num_digits--;
					}
					//	}  
					
					//fprintf(stderr, ANSI_COLOR_YELLOW "string =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CalledPartyNumber);
					//	memset(dlg_calldetails_ptr->CountryCodeB, 0, sizeof(dlg_calldetails_ptr->CountryCodeB));
					//memcpy(dlg_calldetails_ptr->CountryCodeB, dlg_calldetails_ptr->CalledPartyNumber, num_digits);
				    }
				else
				    {
					//todo	CAMELGW_releasecall_cmd(ic_dlg_id, dlg_ptr, h, &invokeID);
					//todo return 0;
					for (i=0;i < num_digits;i++) {                            // -1 here because of odd number last digits it is just filler
					    
					    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
					}
					//delete f filler at the end position
					if (dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] == '?') {
					    dlg_calldetails_ptr->CalledPartyNumber[num_digits-1] = '\0';
					    num_digits--;
					}
				    }
			    }
			//0612			memcpy(dlg_calldetails_ptr->CountryCodeB, dlg_calldetails_ptr->CalledPartyNumber, num_digits);
		    

}
		else
		    /* DP=2 and couldn't get CalledPartyBCDNumber, may be CF call ->try to get CalledPartyNumber and RedirectionInformation from IDP*/
		    {
			/******************* CalledPartyNumber and RedirectionInformation processing ********************************************/
		/**************** when DP = 2, then in MF called number get from CalledPartyNumber param in IDP  *****/	
		status = IN_get_component_param(INPN_CalledPartyNumber, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

                if (status == IN_SUCCESS)
			    {
                            pso_CalledAddr = &dlg_calldetails_ptr->CalledAddr;
                            CAMELGW_rec_called_num(pso_CalledAddr, digits_buffer, data_length, CALLED);
			//				if (digits_buffer[0] == 0x84 ) { /*odd and international called num in IDP */
			//	    num_digits = (data_length -2)*2 -1 -1; //not include filler and first 7
			//	}
                //	num_digits = (data_length - 2) * 2;

        //			unpack_digits(digit_buffer, digits_buffer, 4, num_digits);//(sizeof(called_pty_param)+4));
        //			memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber));
				
                //		for (i=0;i < num_digits;i++){
                //		    dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;
                //		}

                //		memcpy(dlg_calldetails_ptr->CountryCodeB, dlg_calldetails_ptr->CalledPartyNumber, num_digits);

		    //		    memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber));
        //			memset(dlg_calldetails_ptr->RedirectingNumber, 0, sizeof(dlg_calldetails_ptr->RedirectingNumber));
				//memset(dlg_calldetails_ptr->CountryCodeB, 0, sizeof(dlg_calldetails_ptr->CountryCodeB));

                            /* itu t Q.763 -- RedirectingPartyID look in this document */
				
				status = IN_get_component_param(INPN_RedirectingPartyID, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
				
				if (status == IN_SUCCESS)
				    {
					dlg_calldetails_ptr->leg_type = 1; /* MF call */
                                        pso_RedirectingAddr = &dlg_calldetails_ptr->RedirectingAddr;
                               CAMELGW_rec_redirecting_num( pso_RedirectingAddr, digits_buffer, data_length );

					// !!					if (digits_buffer[0] == 3) { /*national and even number in calling party param in IDP */
					    //	memset(dlg_calldetails_ptr->CallingPartyNumber, 0, sizeof(dlg_calldetails_ptr->CallingPartyNumber)); 	
                                            //num_digits = (data_length - 2) * 2;
					    // !!					}
					//TODO! should handle over digits_buffer[0] values (odd, international and so on)
                                        //unpack_digits(digit_buffer, digits_buffer, 4, num_digits ); //offset goes from 2 octet
					
                                        //for (i=0;i < num_digits;i++) {                            // -1 here because of odd number last digits it is just filler
                                          //  dlg_calldetails_ptr->RedirectingNumber[i] = digit_buffer[i] + 0x30;
                                        //}
					//user redirecting number as calling party number for oracle
					//in case when we have DP2 and have no BCD called - suppose this is MF call
				    }
				else
				    {
					printf("status of getting redirectingpartyid = %d\n", status);
				    }
				

				status = IN_get_component_param(INPN_RedirectionInformation, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
				if (status == IN_SUCCESS)
				    {
					//					event_flags = event_flags | (unsigned short) event_REDINFO_Y; 

					num_digits = data_length  * 2;
					unpack_digits(digit_buffer, digits_buffer, 0, num_digits);
					for (i=0;i < num_digits;i++) {
					    printf("red digits = %x\n", digit_buffer[i]);
					}
					/* digit_buffer[0] = Redirection indicator */
					/*     digit_buffer[1] = Original Redirection reason */
					/*     digit_buffer[2] = Redirection counter */
					/*     digit_buffer[3] = Redirection reason (current redirection) */
					/*******************************************this should be moved to prepaid so		
			if (digit_buffer[3] == UNCONDITIONAL)
					    {
						CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);	 
						//printf("Continue because IDP with DP2 contain  call forwarding unconditional redirection info\n");
						return 0;
					    }
					if (msc_type == MSC_HOME)
					    {
						CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);	 
						//						printf("Continue because IDP with DP2 contain  some call forwarding redirection info and msc_type = home msc\n");
						return 0;
					    }
					*************************************************************************************/
				    }
				else
				    {
					printf("error: status of getting redirectioninformation = %d\n", status);
				    }
				/***************************************************************************************************/
			    }
			else
			    {

				printf("error:coulndt not get neither called party BCD number no called party number when DP2\n");
			    }
		    }
		break;

	    case 12:
		dlg_calldetails_ptr->leg_type = 2; /*mt call without redirecting number and info in IDP*/

		//event_flags = event_flags | (unsigned short) event_DP12; 
		/*called subscriber state processing - could be one state from 4 available */
		status = IN_get_component_param(INPN_AssumedIdle, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

		if (status == IN_SUCCESS)
		    {
			printf("debug: assumed idle  data lengh = %d\n", data_length);
			//event_flags = event_flags | (unsigned short) event_SUB_IDLE; 		
		    }
		else
		    {
			status = IN_get_component_param(INPN_CamelBusy, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
			if (status == IN_SUCCESS)
			    {
				printf("debug: camel busy state data lengh = %d\n", data_length);
				//		event_flags = event_flags | (unsigned short) event_SUB_CAMEL_BUSY; 		
			    }
			else
			    {
				status = IN_get_component_param(INPN_NetDetNotReachable, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
				if (status == IN_SUCCESS)
				    {

					//	event_flags = event_flags | (unsigned short) event_SUB_NDNR;
					//event_flags = event_flags | (digits_buffer[0] << 6);
					printf("debug: netdetnotreachable data lengh = %d\n", data_length);
					printf("debug: netdetnotreachable digits_buffer[0] = %d\n", digits_buffer[0]);

					if (digits_buffer[0] == 2)
					    {
						//need move to prepaid so lib
						//CAMELGW_invoke_continue(ic_dlg_id, dlg_ptr, h, &invokeID);	 
						//return 0;

					    }
				    }
				else
				    {
					status = IN_get_component_param(INPN_NotProvidedFromVLR, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
					if (status == IN_SUCCESS)
					    {
						//for (i=0;i<sizeof(sub_state_param);i++) {
						printf("debug: not provided from VLR  data lengh = %d\n", data_length);
						//}
					    }
					else
					    printf("error while getting subscriber status!\n");
				    }
			    }
		    }

		status = IN_get_component_param(INPN_CalledPartyNumber, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
		if (status == IN_SUCCESS)
		    {
			pso_CalledAddr = &dlg_calldetails_ptr->CalledAddr;
			CAMELGW_rec_called_num(pso_CalledAddr, digits_buffer, data_length, CALLED);

			// printf("info:called:oddi=%d;noai=%d;inni=%d;npi=%d;naddr=%d;digits = ", pso_CalledAddr->oddi, pso_CalledAddr->noai, pso_CalledAddr->inni, pso_CalledAddr->both_npi, pso_CalledAddr->naddr);

			//	    for (i=0; i<pso_CalledAddr->naddr; i++)
			//printf("info  %x", pso_CalledAddr->addr[i]);
			//printf("\n");
	}
		else
		    {
			printf("error");
		    }	
		/* check GSM forwarding pending indicator   */
                /* INPN_GSM_ForwardingPending   --- exist only in case CFU and MT call and in case of some CFNRc forwardings */

                status = IN_get_component_param(INPN_RedirectingPartyID, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

                if (status == IN_SUCCESS)
                    {
                        dlg_calldetails_ptr->leg_type = 3; /* MT call with Redirecting info and Redirecing Number in idp */
                        pso_RedirectingAddr = &dlg_calldetails_ptr->RedirectingAddr;
               CAMELGW_rec_redirecting_num( pso_RedirectingAddr, digits_buffer, data_length );
}
 else
                {

                }



		status = IN_get_component_param(INPN_RedirectionInformation, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
		if (status == IN_SUCCESS)
                {
    //                call_type = 1; /* MF call */
      //              pso_RedirectingAddr = &dlg_calldetails_ptr->RedirectingAddr;
        //   CAMELGW_rec_redirecting_num( pso_RedirectingAddr, digits_buffer, data_length );

		    printf("debug: have redirection information with DP12\n" );
                }
		status =  IN_get_component_param(INPN_GSM_ForwardingPending, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);
		
		if (status == IN_SUCCESS)
		    {

			dlg_calldetails_ptr->forwarding_pending = 1;
			//inform cdr_logger
			//DP12;sub_state;VLR;forwarding_pending; Continue
			//			return 0;
		    }
		else
		    {
			//			printf("error getting ForwardingPending from IDP\n");
			dlg_calldetails_ptr->forwarding_pending = 0;
		    }
		
	        break;
		
	    default:     //should be some code for other values of DP
		printf("error: DP in IPD no DP2 neither DP12\n");
		break;
	    }


		fill_subscriber_id(dlg_calldetails_ptr);
		printf("subscriber id = %s\n", dlg_calldetails_ptr->Subscriber_id);


  		memset(dlg_calldetails_ptr->CELLID, 0, sizeof(dlg_calldetails_ptr->CELLID));    
status = IN_get_component_param(INPN_LAIFixedLength, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

 if (status == IN_SUCCESS)
     {
	  printf("lai fixed length exist in IDP\n");
	for (i=0; i< data_length;i++) {
	    //	    printf("lai fixed length exist in IDP\n");
	    printf(" %x", digits_buffer[i]);
	     sprintf(dlg_calldetails_ptr->CELLID + 2*i,"%02x",digits_buffer[i]);//fill by 0 to 2 znaks
	    //dlg_calldetails_ptr->CallReferenceNumber[i] = digit_buffer[i] + 0x30;//CallReference include not only digits but also a,b,c,d,e,f - code like this doesn't work'
	    //sprintf(dlg_calldetails_ptr->CallReferenceNumber + 2*i,"%02x",digits_buffer[i]);//fill by 0 to 2 znaks
	}
	 
     }
 else
     {
status = IN_get_component_param(INPN_CellIdFixedLength, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

 if(status == IN_SUCCESS)
     {
	 	  printf("cell id fixed length exist in IDP\n");
		  	for (i=0; i< data_length;i++) {
	    //	    printf("lai fixed length exist in IDP\n");
	    printf(" %x", digits_buffer[i]);
	     sprintf(dlg_calldetails_ptr->CELLID + 2*i,"%02x",digits_buffer[i]);//fill by 0 to 2 znaks

	}

     }
 else
     {
	 printf("error getting LAI or CellID from IDP\n");
     }
     }


 /* fprintf(stderr, ANSI_COLOR_YELLOW "string CallReferenceNumber as it goes after idp parse function =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallReferenceNumber);  */
 /* fprintf(stderr, ANSI_COLOR_YELLOW "string ServiceKey as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->ServiceKey);  */
 /*  fprintf(stderr, ANSI_COLOR_YELLOW "string IMSI as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->IMSI);  */
 /*  fprintf(stderr, ANSI_COLOR_YELLOW "string LocationNumber as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->LocationNumber);  */
 /*  fprintf(stderr, ANSI_COLOR_YELLOW "string CELLID as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CELLID);  */
 /* fprintf(stderr, ANSI_COLOR_YELLOW "string CalledPartyNumber as it goes before ora call =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CalledPartyNumber);  */
 /* fprintf(stderr, ANSI_COLOR_YELLOW "string CountryCodeB  as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CountryCodeB);  */
 /* fprintf(stderr, ANSI_COLOR_YELLOW "string MSC as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->MSC);  */
 /* fprintf(stderr, ANSI_COLOR_YELLOW "string CountryCodeA as it goes to IRBIS before ora_ cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CountryCodeA);  */
 /* fprintf(stderr, ANSI_COLOR_YELLOW "string CallingPartyNumber as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallingPartyNumber);  */
 /* fprintf(stderr, ANSI_COLOR_YELLOW "string RedirectingNumber as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->RedirectingNumber); */

    return 0;
}
