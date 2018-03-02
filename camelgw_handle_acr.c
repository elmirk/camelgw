#include "intu_def.h"
#include "time.h"
#include "sys/ipc.h"
#include "helper.h"
#include "globals.h"
#include <error.h>

#include "camelgw_backend.h"
#include "camelgw_drivers.h"
#include "camelgw_prepaid.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"



/* definitions used in intu_handle_acr function */

#define TAG_LEGSTATE 130	//82h, tag value for LegActive parameter in ACR message
#define TAG_DURATION 128 //80h tag value for call duration value in ACR message
#define TAG_PARTYTOCHARGE 129 //81h, partytoCharge param in ACR

#define LEG_ACTIVE_FALSE 0
#define LEG_ACTIVE_TRUE 1

//
//struct acr{

//  unsigned int duration_ms;
//  unsigned char leg_active;
//  unsigned char party_to_charge;

//};


/*
*******************************************************************************
*                                                                             *
* INTU_handle_applychargingreport:
* backend function for Apply Charging Report received from SSF processing              *
*               when we receive ACR with LegActive = false -> no need to close dialog  *
* dialogue should be closed only when receive BCSMevent reports                        *
*******************************************************************************
*/
//int CAMELGW_invoke_apply_charging(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, u8 ReleaseFlag);
//int CAMELGW_test(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, u8 ReleaseFlag );

int camelgw_handle_acr(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, IN_CPT *buffer_cpt_ptr)
{

	typedef struct in_cpt dlg_ptr_new;

	struct acr acr_info;

	struct calldetails *dlg_calldetails_ptr;
	PTY_ADDR	called_pty;     // Stores the recovered calling party number

	PLEN	Time_number_plen;
	PLEN	Call_Active_plen;
	PLEN	called_pty_plen;	        // Size of param 
	PLEN	invokeID_len;		        // Octet length of InvokeID 
	PLEN	elvina_plen;HDR *og_h;  	// Message used to send immediate replies

	HDR	*og2_h;

	IN_CPT	*dlg_cpt_ptr;	// Holds recovered INAP component 
	IN_CPT	buffer_cpt;		// создается буфферная структура для работы с текущим значеним ApplyChargingReport
	IN_CPT	og2_cpt;		// ELVINA this is for outgoing ApplyCharging component
	IN_CPT	og3_cpt;
	IN_CPT	og_cpt;			// Holds INAP component whilst building into message 

	int	Time_pty_dgt_str_new[6];	
	int data_length, d_length;
	int QUOTA[2], t[8];
	int i, j;		        // Return code from INAP API functions

	char	Time[3];
	char	Time_1[14];
	char	string[14];

	u16	param_length;

	u8	called_pty_param[INTU_max_called_party_num_len]; 
	u8	databuf1[IN_MAX_DATABUF_SIZE];
	u8	max_call_duration[]={0,0};
	u8	CallActive_pty_param;	
	u8	Time_pty_param[25];
	u8	Rel_If_Dur_Excd[] = {129,1,0};
	u8	apc[]={36,0,2};	     // max call duration in apply_charging message 

	u8	buffer[64];
	
u8	data[32];
	u8	invokeID;
	//unsigned int total_duration_ms;				
	int status;
	//u8	invokeID3;
	int ret;
	unsigned char duration_buff[4];
	int ( *pf_acr_handler)(u32 , DLG_CB *, HDR *, IN_CPT *, struct acr *);//pointer to acr_handle_function


	dlg_cpt_ptr = &(dlg_ptr->cpt);
	//buffer_cpt.type = 0; //инициализируем переменную
	data_length =0;

	dlg_calldetails_ptr = &(dlg_ptr->call_details);

	//int ( *pf_acr_handler)(u32 , DLG_CB *, HDR *, IN_CPT *);//pointer to acr_handle_function

	//	pf_acr_handler = dlg_ptr->acr_handler;
	//pf_acr_handler(ic_dlg_id, dlg_ptr, h, buffer_cpt_ptr);

	//return 0;


	/*
	* в буфферную компоненту копируем текущие данные 
	* и заполняем ее только воими данными а не всей портянкой databuf 
	*/
	//	memcpy(&buffer_cpt, dlg_cpt_ptr, sizeof(IN_CPT));
	//memset(&(buffer_cpt.databuf[0]), 0, IN_MAX_DATABUF_SIZE);
	//memcpy(&(buffer_cpt.databuf[0]), (dlg_cpt_ptr->databuf) + 
	//		dlg_ptr->cursor, (dlg_cpt_ptr->databuf_offset) - 
	//		dlg_ptr->cursor);
	//printf("!!!!!!!!!!!!!!!!!!!!!!  ACR operation  = %i\n", buffer_cpt_ptr->operation);
	//printf("!!!!!!!!!!!!!!!!!!!!!!  ACR databuf_offset  = %i\n", buffer_cpt_ptr->databuf_offset);
	/*copy INPN_CallResult array into buffer array */
	status = IN_get_component_param(INPN_CallResult, &param_length, buffer, sizeof(buffer), buffer_cpt_ptr);
	/*TODO - need to process returned status! */
	
//	printf("debug:handle acr start\n");
	//printf("debug:handle_acr: servicy type = %d\n", dlg_ptr->service);
	//printf("call result length = %i\n", param_length);
	//	d_length = param_length;
	//dlg_calldetails_ptr = &(dlg_ptr->call_details);


	if ( CAMELGW_get_callresult_param(buffer, param_length, TAG_DURATION, &data[0]) == 1)
	    {
		printf("no duration or param_length = 0 !\n");
		return 0;
		//TODO! need some logic here!
	    }
	memset(&duration_buff[0], 0, sizeof(duration_buff));

	if (data[0] > 4)
	    {
		printf("integer not enought for duration from ACR\n");
		return 0;
		//TODO! need some logic here!
	    }


	    memcpy(&duration_buff[0], &data[1], data[0]);
	//dlg_calldetails_ptr = &(dlg_ptr->call_details);

	reversearray(&duration_buff[0], data[0]);

unsigned int  *p_duration;

//pointer to call duration value from ACR (in number of 100 ms)

	p_duration = (int *) &duration_buff[0];

acr_info.duration_ms  = 100 * (*p_duration); //convert received from ACR value into pure milliseconds

	/* есть длина буффера и сам массив буффер в котором содержаться все данные 
	* Call Result из сообщения ApplyChargingReport
	* если LegActive = false , то надо закрывать диалог 
	* + логика тут же (если надо передать общую длительность в БД) 
	*/	

        printf(" time received in ACR  = %d\n", *p_duration );
 
         //now check leg_state parameter in ACR
 
         if ( CAMELGW_get_callresult_param(buffer, param_length, TAG_LEGSTATE, &data[0]) == 0 )
             {
                //leg state exist but we need check false or true

                if (data[1] == LEG_ACTIVE_FALSE) /* when call LEG already disconected by SSF */
                    {
			acr_info.leg_active = 0;
			printf("leg active = false \n" );
 
  //this should goes to prepaid.c
  //
  //prepaid_close_call(dlg_calldetails_ptr, total_duration_ms, ic_dlg_id);
  //
			// printf("close call  status code = %d\n", dlg_calldetails_ptr->status_code);

//		if (dlg_calldetails_ptr->status_code == 993)
//		    printf("ERROR: Произошла ошибка при записи итоговой длительности разговора в ирбис!\n");
//		else if ( dlg_calldetails_ptr->status_code == 992 )
//		printf("intu_handle_acr: call ended params: call_id[%i] and duration = %i\n", dlg_calldetails_ptr->call_id, dlg_calldetails_ptr->duration);
//		else {
//		    printf("ERROR:some shit happens in ACR function\n");
//		}
//        return 0;
                    }
		else
		    {
			acr_info.leg_active = 1;
			printf("leg active = true \n" );
}
            }
	 else
	     {
        //if leg active = true or leg stat not exist in call result
        printf("leg active = true or not exist in call result \n" );
	acr_info.leg_active = 1; /*true */
	     }

	CAMELGW_get_callresult_param(buffer, param_length, TAG_PARTYTOCHARGE, &data[0]);

acr_info.party_to_charge = data[1];


	pf_acr_handler = dlg_ptr->acr_handler;
	pf_acr_handler(ic_dlg_id, dlg_ptr, h, buffer_cpt_ptr, &acr_info);

	return 0;




		//in 100 milliseconds
		//dlg_calldetails_ptr->duration = HexToDec(Time);

		//dlg_calldetails_ptr->duration = 100 * HexToDec(Time); //pure milliseconds


		switch (dlg_ptr->service)
		    {
		    case M2M:
			//old!!			dlg_calldetails_ptr->op_code = CAMELGW_M2M_CALL_END_IND;
			//TODO!			dlg_calldetails_ptr->duration =  HexToDec(Time);
			if ( ( ret = CAMELGW_RequestDriver(M2M_DRIVER, dlg_calldetails_ptr) ) != 0)
			    printf("debug: error -  intu_handle_acr, ret = %d\n", ret);
			return 0;
			break;
		    default:
			break;
		    }


		//!! todo - need change hexto dex to another way	dlg_calldetails_ptr->duration = 100 * HexToDec(Time); //pure milliseconds
		//printf("call id from billing =  %i and total duration = Time(Dec) ms %d\n", dlg_calldetails_ptr->call_id, dlg_calldetails_ptr->duration);
		//old!!		dlg_calldetails_ptr->op_code = CAMELGW_ORACLE_CALL_END_IND; /*op code = 1 means that intu should inform oracle that call ended */
		//CAMELGW_get_data_from_ora(dlg_calldetails_ptr);
   /*			-----------------Коды возврата при обращение к БД через функцию CAMELGW_get_data_from_ ora----------------
			status_code = 1 - у абонента положительный баланс but also need to check quota value, may be quota = 0
			status_code = 2 - у абонента баланс <=  0 (абонента нет в базе) 
			status_code = 992 stmt execute успешно выполнился
			status_code = 996 - в течение 2 секунд не получен ответ от БД при вызове функции запроса первой длительности
			status_code = 993 - Ошибка функции OCI Execute
			status_code = 999 - Ошибка со стороны БД */ 
		//		if (dlg_calldetails_ptr->status_code == 993)
		///  printf("ERROR: Произошла ошибка при записи итоговой длительности разговора в ирбис!\n");
		//else if ( dlg_calldetails_ptr->status_code == 992 )
		//printf("intu_handle_acr: call ended params: call_id[%i] and duration = %i\n", dlg_calldetails_ptr->call_id, dlg_calldetails_ptr->duration);
		//else {
		//   printf("ERROR:some shit happens in ACR function\n");
		//}
		//	printf("на входе single ACR with leg active = false  - ничего не делаем!!!\n");
		//INTU_send_delimit(ic_dlg_id); /* add this one */
		//INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN);			
		//		return 0;
		
		//	else{
		//INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
		//INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
		//return 0;
		//}
	//dlg_ptr->cursor=dlg_cpt_ptr->databuf_offset; курсор фиксируется в функции handle_invoke

	//	fprintf(stderr, ANSI_COLOR_YELLOW "Databuf_Offset_1(dex) %i" ANSI_COLOR_RESET "\n", end);

	/*
	* Initialise the component used to build invoke replies
	*/  
	//	if (data[1] == LEG_ACTIVE_TRUE) /*call is ongoing*/
		    // {
	/* INOP_ApplyCharging */
#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og2_cpt,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og2_cpt);
#endif

	/*
	* Get the invokeID, add 128 (0x80) and reply with that.
	* InvokeID chosen to be (0   to 127) Incoming
	*                       (128 to 255) Outgoing
	*/
status = IN_get_component_param(INPN_InvokeID, &invokeID_len, &invokeID, 1, buffer_cpt_ptr);//dlg_cpt_ptr);


 if (status == IN_SUCCESS)
     {

     }
 else
     {
	 printf("get component error in ACR function = %d\n", status);
     }

		invokeID = 4;
		IN_set_component_param(INPN_InvokeID, 1, &dlg_ptr->current_invoke_id, &og2_cpt);

 
#ifdef IN_LMSGS
	if ((og_h = IN_alloc_message(0)) == 0)
#else
	if ((og_h = IN_alloc_message()) == 0)
#endif
	{
		/*
		* We can't get a message to reply with so just return
		*/
		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return(INTUE_MSG_ALLOC_FAILED);
	}



 	if ( dlg_ptr->service == M2M ) 
 		    { 
 printf("debug: hande_acr: before driver request for next quota\n"); 
 //old!!! 			dlg_calldetails_ptr->op_code = CAMELGW_M2M_QUOTA_REQ; 
 			if ( ( ret = CAMELGW_RequestDriver(M2M_DRIVER, dlg_calldetails_ptr) ) != 0) 
 			    {			     
 printf("debug: error -  intu_handle_acr, ret = %d\n", ret); 
/* //TODO - should do something when error in request driver function; */
 			    } 
/* 			printf("debug:handle_acr:before status code analyze\n"); */
 if ( dlg_calldetails_ptr->status_code == M2M_SUCCESS)  
     {
	 printf("debug:hanlde_acr: status code success = %d\n", dlg_calldetails_ptr->status_code );
 		switch(dlg_calldetails_ptr->param1[0]) 
 		    {
 		    case M2M_RESPONSE_REQNEXTQUOTA: 
 			printf("debug:intu_handle_acr:req next quota\n"); 
			CAMELGW_invoke_apply_charging(ic_dlg_id, dlg_ptr, h, &invokeID, CONTINUE, dlg_calldetails_ptr->quota); 
			//			CAMELGW_test(ic_dlg_id, dlg_ptr, h, &invokeID, CONTINUE);
 			return 0; 
 			break; 
 		    case M2M_RESPONSE_LASTQUOTA: 
 			printf("debug:intu_handle_acr:m2m:last quota, release\n"); 
/* 			CAMELGW_invoke_apply_charging(ic_dlg_id, dlg_ptr, h, &invokeID, RELEASE); */
/* //			CAMELGW_call_duration_control(ic_dlg_id, dlg_ptr, h, &invokeID, RELEASE); */
/* 			return 0; */
 			break; 
/* 		    case M2M_RESPONSE_RELEASECALL: */
/* 	printf("debug:intu_handle_idp:m2m:release cmd from m2m received\n"); */
/* 			break; */
/* 		    case M2M_RESPONSE_DEFAULTACTION: */
/* 	printf("debug:intu_handle_idp:m2m:default action\n"); */
/* 			break; */
/* 		    case M2M_RESPONSE_OUTOFORDER_DUE_MAINT:	 */
/* printf("debug:intu_handle_idp:m2m:out of order\n"); */
/* 			break; */
 		    default: 
 	printf("debug:intu_handle_idp:m2m:default case\n"); 
 			break; 
 		    } 
     } 
 else
     {
	 printf("debug:hanlde_acr: status code error = %d\n", dlg_calldetails_ptr->status_code );

}
 		    } 



	//old!! 	dlg_calldetails_ptr->op_code = CAMELGW_ORACLE_NEXT_QUOTA_REQ; /*op code = 2 means intu asks for next quota for current call */

	//fprintf(stderr, ANSI_COLOR_YELLOW "string CallReference as it goes to IRBIS before ora call in ACR processing function=  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallReferenceNumber);
	// fprintf(stderr, ANSI_COLOR_YELLOW "Call ID  as it goes to IRBIS before ora call in ACR processing function=  %i" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->call_id);
//fprintf(stderr, ANSI_COLOR_YELLOW "string ServiceKey as it goes to IRBIS before ora call in ACR processing function=  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->ServiceKey);
//fprintf(stderr, ANSI_COLOR_YELLOW "string CalledPartyNumber as it goes to IRBIS before ora call in ACR processing function=  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CalledPartyNumber);
//fprintf(stderr, ANSI_COLOR_YELLOW "string CountryCodeB  as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CountryCodeB);

//fprintf(stderr, ANSI_COLOR_YELLOW "string MSC as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->MSC);

//fprintf(stderr, ANSI_COLOR_YELLOW "string CountryCodeA as it goes to IRBIS before ora_ cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CountryCodeA);
//fprintf(stderr, ANSI_COLOR_YELLOW "string CallingPartyNumber as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->CallingPartyNumber);
// fprintf(stderr, ANSI_COLOR_YELLOW "string RedirectingNumber as it goes to IRBIS before ora_cli =  %s" ANSI_COLOR_RESET "\n", dlg_calldetails_ptr->RedirectingNumber);

//should go to prepaid.c
//
//	prepaid_update_call(dlg_calldetails_ptr, total_duration_ms, ic_dlg_id);
//

	//old! CAMELGW_get_data_from_ora(dlg_calldetails_ptr);

//	dlg_calldetails_ptr->quota = 180;
//	dlg_calldetails_ptr->status_code = 1;
/*
			-----------------Коды возврата при обращение БД----------------
			status_code = 1 - у абонента положительный баланс but also need to check quota value, may be quota = 0
			status_code = 2 - у абонента баланс <=  0 (абонента нет в базе) 
			status_code = 991
			status_code = 997 - в течение 2 секунд не получен ответ от БД при вызове функции запроса первой длительности
			status_code = 998 - Ошибка функции OCI Execute
			status_code = 999 - Ошибка со стороны БД  */

//	printf("Максимально допустимая длительность квоты разговора from irbis %i\n", dlg_calldetails_ptr->quota*10);
 printf("handle_ACR: data from update call function : QUOTA(dec) = %i CALL_ID = %i Return Status Code = %i \n", dlg_calldetails_ptr->quota, dlg_calldetails_ptr->call_id, dlg_calldetails_ptr->status_code);
 printf("static in handle ACR function id  = %i\n", dlg_calldetails_ptr->id);

	    	if (dlg_calldetails_ptr->status_code == 2)
		    {
		    /*balanc less than 0 - release call */
		IN_set_component_param (INPN_Cause, sizeof(example_release_cause), example_release_cause, &og_cpt);

		IN_set_operation(INOP_ReleaseCall, INTU_ReleaseCall_timeout, &og_cpt);

		status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);

		if (status == IN_SUCCESS)
		{
			if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
				IN_free_message(og_h);

			INTU_send_close(ic_dlg_id, INAPRM_normal_release);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
			return 0;
		}
	}
	/* 998 - this is our error code */
	if (dlg_calldetails_ptr->status_code == 998) {
		IN_set_operation(INOP_Continue, INTU_Connect_timeout, &og_cpt);
		status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
		if (status == IN_SUCCESS)
		{
			if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
				IN_free_message(og_h);

			INTU_send_delimit(ic_dlg_id);
			INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
			return 0;
		}
	}

	//elmir portion_duration(dlg_calldetails_ptr->quota*10, t); //Для комерции
	//portion_duration(60*10, t); //Для тестов
	//elmir max_call_duration[0]=t[0];
	//elmir max_call_duration[1]=t[1];
	max_call_duration[0] = (dlg_calldetails_ptr->quota * 10) >> 8;
	max_call_duration[1] = 0x00ff & (dlg_calldetails_ptr->quota * 10);
	//	printf("Отправляем в ответ сообщение ApplyCharging\n");

	//old code	INTU_send_message(intu_mod_id, inap_mod_id, og2_h);
	//old code og2_h = IN_alloc_message(0);

	IN_set_operation(INOP_ApplyCharging, INTU_ReleaseCall_timeout, &og2_cpt);
	//	IN_set_component_param(INPN_InvokeID, 1, &dlg_ptr->current_invoke_id, &og2_cpt);
	IN_set_component_param(INPN_MaxCallPeriodDuration, 2, max_call_duration, &og2_cpt); // max_call_duration here comes as a pointer
	
	if (dlg_calldetails_ptr->quota < 180) {
		IN_set_component_param(INPN_Tone, 1, Rel_If_Dur_Excd+2, &og2_cpt); //set Tone = False and release if dur exceed automatically enabled
	}
	IN_set_component_param(INPN_SendingSideID(0), 1, apc+2, &og2_cpt);

	status = IN_code_operation_invoke(ic_dlg_id, &og2_cpt, og_h);
	

	if (status == IN_SUCCESS)
	  {
		if ( INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS )
		    IN_free_message(og_h);
	  }
	else
	    {
	printf("status =! IN_SUCCESS\n");
		fprintf(stderr, "INTU: *** Failed to encode invoke for dialogue  ID 0x%04x [%i] ***\n",ic_dlg_id, status);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
		return INTUE_CODING_FAILURE;
		    }
	/*
	* Modified by Elmir to construct TCAP continue instead of TC-end when 
	* send IN reply on received IDP 
	*/
	INTU_send_delimit(ic_dlg_id); // add this one 
	//fprintf(stderr, "INTU: *** Отправил delimit in function handle ACR***\n");
	//fprintf(stderr, "INTU: *** Далее пытаюсь сменить состояние на OPEN ***\n");
	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN);
	//fprintf(stderr, "INTU: *** Далее попытался сменить состояние на OPEN ***\n");
	fprintf(stderr,ANSI_COLOR_YELLOW "is_dlg_id %x" ANSI_COLOR_RESET "\n",ic_dlg_id);
	return 0;

}
