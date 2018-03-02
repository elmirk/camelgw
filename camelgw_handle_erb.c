#include "intu_def.h"
#include "time.h"
#include "sys/ipc.h"
#include "helper.h"
#include "globals.h"
#include <error.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*
*******************************************************************************
*                                                                             *
* INTU_handle_eventreportbcsm: подготовка ответа на eventReportBCSM           *
*                                                                             *
********************************************************************************/
/*
* на каждое полученное от MSC сообщение eventReportBCSM необходимо что то делать 
* пока в коде ничего не делаем, просто меняем state на open
* 
* this code doesnt send Continue on EventReportBCSM'
* 
* необходимо также проверить переключение статусов диалогов                      
*/
//int camelgw_handle_erb(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, IN_CPT *buffer_cpt_ptr, u8 mode)
int camelgw_handle_erb(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, IN_CPT *buffer_cpt_ptr)
{

    u8 buffer[32];
    //    int i;
    IN_CPT	*dlg_cpt_ptr;	// Holds recovered INAP component
	IN_CPT	og_cpt; 

	HDR	*og2_h;
	HDR	*og_h;				// Message used to send immediate replies

	u16	param_length;
	int status;
	u8 invokeID;
	struct calldetails *dlg_calldetails_ptr;
	int ( *pf_erb_handler)(u32 , DLG_CB *, HDR *, IN_CPT *, struct erb *);//pointer to acr_handle_function

	dlg_cpt_ptr = &(dlg_ptr->cpt);
	dlg_calldetails_ptr = &(dlg_ptr->call_details);

	struct erb erb_info;


	status = IN_get_component_param(INPN_EventTypeBCSM(0), &param_length, buffer, 32, buffer_cpt_ptr);
//printf("!!!!!!!!!!!!!!!!!!!!!!  event type code  = %i\n", buffer[0]);
	//need to process tbusy - do nothind if Dialog already closed by ACR message

	erb_info.event_type = buffer[0];
	
	status = IN_get_component_param(INPN_MessageType, &param_length, buffer, 32, buffer_cpt_ptr);

	erb_info.message_type = buffer[0];

	if (erb_info.event_type == eventTypeBCSM_ODisconnect)
	    {
		status = IN_get_component_param(INPN_ReleaseCause, &param_length, buffer, 32, buffer_cpt_ptr);

				/* 	printf("release cause data in Odisconnect = \n"); */
				/* for (i = 0; i<param_length;i++) */
				/*     { */
				/*     printf( " %x ", buffer[i]); */
		//		    }
				memcpy(&erb_info.release_cause[0], &buffer[0], 2);
	    }


	if (erb_info.event_type == eventTypeBCSM_TDisconnect)
	    {
		status = IN_get_component_param(INPN_ReleaseCause, &param_length, buffer, 32, buffer_cpt_ptr);

		//					printf("release cause data in Tdisconnect = \n");
				/* for (i = 0; i<param_length;i++) */
				/*     { */
				/*     printf( " %x ", buffer[i]); */
		//				    }
				memcpy(&erb_info.release_cause[0], &buffer[0], 2);
	    }


	if (erb_info.event_type == eventTypeBCSM_TBusy )
	{

status = IN_get_component_param(INPN_BusyCause, &param_length, buffer, 32, buffer_cpt_ptr);
				if (status == IN_SUCCESS)
				    {
					//			printf("busy cause data in tbusy = \n");
					//	for (i = 0; i<param_length;i++)
					//printf( " %x ", buffer[i]);
				    
					memcpy(&erb_info.busy_cause[0], &buffer[0], 2);
				    }
				
else
				    printf("tbusy busy cause error!\n");
	}

	if (erb_info.event_type == eventTypeBCSM_OCalledPartyBusy )
	{

status = IN_get_component_param(INPN_BusyCause, &param_length, buffer, 32, buffer_cpt_ptr);
 if (status == IN_SUCCESS)
     {
	 /* printf("busy cause data in Obusy = \n"); */
	 /* for (i = 0; i<param_length;i++) */
	 /*     printf( " %x ", buffer[i]); */
				    
	memcpy(&erb_info.busy_cause[0], &buffer[0], 2);
}
				
else
				    printf("obusy busy cause error!\n");
	}

	/* printf("data parsed in    %s:\n", __PRETTY_FUNCTION__); */
	/* printf("erb event type =                    %d\n", erb_info.event_type); */
	/* printf("erb message type =                  %d\n", erb_info.message_type); */

	pf_erb_handler = dlg_ptr->erb_handler;
	pf_erb_handler(ic_dlg_id, dlg_ptr, h, buffer_cpt_ptr, &erb_info);

	return 0;

	/* if (buffer[0] == eventTypeBCSM_RouteSelectFailure ) */
	/* { */
	/* 	printf("на входе eventTypeBCSM_RouteSelectFailure - закрываем диалог pre-arrahged end!!!\n"); */
	/* 	INTU_send_close(ic_dlg_id, INAPRM_prearranged_end); */
	/* 	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING); */
	/* 	fprintf(stderr,ANSI_COLOR_YELLOW "is_dlg_id in handle ERB function  %x" ANSI_COLOR_RESET "\n",ic_dlg_id); */
	/* 	return 0; */
	/* } */


/* 	if (buffer[0] == eventTypeBCSM_TBusy ) */
/* 	{ */
/* 		printf("eventTypeBCSM = tBusy\n"); */
/* 				status = IN_get_component_param(INPN_EventSpecInfo_Ellipsis, &param_length, buffer, 32, buffer_cpt_ptr); */
/* 				if (status == IN_SUCCESS) */
/* 				    { */
/* 					printf("ellipsis data in tbusy = \n"); */
/* 				for (i = 0; i<param_length;i++) */
/* 				    printf( " %x ", buffer[i]); */
/* 				    } */
/* 				else */
/* 				    printf("tbusy ellipsis error!\n"); */
/* status = IN_get_component_param(INPN_BusyCause, &param_length, buffer, 32, buffer_cpt_ptr); */
/* 				if (status == IN_SUCCESS) */
/* 				    { */
/* 					printf("busy cause data in tbusy = \n"); */
/* 				for (i = 0; i<param_length;i++) */
/* 				    printf( " %x ", buffer[i]); */
/* 				    } */
/* 				else */
/* 				    printf("tbusy busy cause error!\n"); */
								

/* 						INTU_send_close(ic_dlg_id, INAPRM_prearranged_end); */
/* 				INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING); */
/* 				//  } */
/* 			//			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN); */
/* 			//fprintf(stderr, "INTU: *** Далее попытался сменить состояние на OPEN ***\n"); */
/* 			fprintf(stderr,ANSI_COLOR_YELLOW "is_dlg_id in handle ERB function  %x" ANSI_COLOR_RESET "\n",ic_dlg_id); */

/* 		return 0; */
//	}

	/* if (buffer[0] == eventTypeBCSM_TNoAnswer) */
	/* { */
	/* 	printf("eventTypeBCSM =  tNoAnswer\n"); */
	/* 	INTU_send_close(ic_dlg_id, INAPRM_prearranged_end); */
	/* 	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING); */
	/* 	fprintf(stderr,ANSI_COLOR_YELLOW "is_dlg_id in handle ERB function  %x" ANSI_COLOR_RESET "\n",ic_dlg_id); */
	/* 	return 0; */
	/* } */
	/* if ( buffer[0] == eventTypeBCSM_TAnswer ) */
	/* { */
	/* 	printf("eventTypeBCSM =  tAnswer\n"); */
	/* 	//INTU_send_delimit(ic_dlg_id); /\* add this one *\/ */
	/* 	//		fprintf(stderr, "INTU: *** Отправил delimit при обработке eventreport с tAnswer\n"); */

	/* 	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN);			 */
	/* 	return 0; */
	/* } */


	/* if (buffer[0] == eventTypeBCSM_TAbandon) */
	/* { */
	/* 	printf("eventTypeBCSM = tAbandon\n"); */
	/* 	INTU_send_close(ic_dlg_id, INAPRM_prearranged_end); */
	/* 	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING); */
	/* 	fprintf(stderr,ANSI_COLOR_YELLOW "is_dlg_id in handle ERB function  %x" ANSI_COLOR_RESET "\n",ic_dlg_id); */
	/* 	return 0; */
	/* } */


		//		printf("!!!!!!!!!!!!!!!!!!!!!!  event message type  = %i\n", buffer[0]);
		
		//		if (buffer[0] == interrupted ) { /*message type =request = 0*/
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
		//		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		//		return INTUE_MSG_ALLOC_FAILED;
		//	}
		//	IN_set_operation(INOP_Continue, INTU_ReleaseCall_timeout, &og_cpt);
		//	invokeID = 13;
		//	IN_set_component_param(INPN_InvokeID, 1, &invokeID, &og_cpt);
		//	status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
		//	if (status == IN_SUCCESS)
		//	{
		//		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
		//			IN_free_message(og_h);
		//	}
		//	else
		//	{
		//		printf("error \n");
		//	}  
		//	INTU_send_delimit(ic_dlg_id); /* add this one */
		//	fprintf(stderr, "INTU: *** Отправил delimit при обработке eventreport с реквестом\n");
		//	return 0;
		//	}
		//	else  /*messagetype = notification =1*/
		//{
		//	printf("!!!!!!!!!!!!!!!!!!!!!tDisconnect with notification type, do hnothing  = %i\n", buffer[0]);
		//	return 0;
		//}  
	//	}

/* 	if (buffer[0] == eventTypeBCSM_TDisconnect) */
/* 	{ */
/* 		printf("на входе tDisconnect - легкая предобработка!!!\n"); */
/* 		IN_get_component_param(INPN_MessageType, &param_length, buffer, 32, buffer_cpt_ptr); */
/* 		printf("!!!!!!!!!!!!!!!!!!!!!!  event message type  = %i\n", buffer[0]); */
/* 		if (buffer[0] == 0) { //message type =request = 0 */
/* #ifdef IN_LMSGS */
/* 			IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0); */
/* #else */
/* 			IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt); */
/* #endif */
/* #ifdef IN_LMSGS */
/* 			if ((og_h = IN_alloc_message(0)) == 0) */
/* #else */
/* 			if ((og_h = IN_alloc_message()) == 0) */
/* #endif */
/* 			{ */
/* 				fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue\ */
/* 								ID 0x%04x ***\n", ic_dlg_id); */
/* 				return INTUE_MSG_ALLOC_FAILED; */
/* 			} */
/* 			IN_set_operation(INOP_Continue, INTU_ReleaseCall_timeout, &og_cpt); */
/* 			invokeID = 13; */
/* 			IN_set_component_param(INPN_InvokeID, 1, &invokeID,&og_cpt); */
/* 			status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h); */
/* 			if (status == IN_SUCCESS) */
/* 			{ */
/* 				if (INTU_send_message(intu_mod_id,  */
/* 					inap_mod_id, og_h) != IN_SUCCESS) */
/* 					IN_free_message(og_h); */
/* 			} */
/* 			else */
/* 			{ */
/* 				printf("error \n"); */
/* 			}   */
/* 			INTU_send_delimit(ic_dlg_id); /\* add this one *\/ */
/* 			fprintf(stderr, "INTU: *** Отправил delimit при обработке eventreport с реквестом\n"); */
/* 			INTU_send_close(ic_dlg_id, INAPRM_prearranged_end); */
/* 			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING); */
/* 			fprintf(stderr,ANSI_COLOR_YELLOW "is_dlg_id in handle ERB function  %x" ANSI_COLOR_RESET "\n",ic_dlg_id); */
/* 			return 0; */
/* 		} */
/* 		else  /\*messagetype = notification =1*\/ */
/* 		{ */
/* 			printf("!!!!!!!!!!!!!!!!!!!!!tDisconnect with message type = notification type, do hnothing  = %i\n", buffer[0]); */
/* 			//1408 - decided never do prearranged end, just wait for dialog timeout and then close (intu.c) */
/* 			//1408			INTU_send_close(ic_dlg_id, INAPRM_prearranged_end); */
/* 			// 1408 INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING); */
/* 			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN); */
/* 			fprintf(stderr,ANSI_COLOR_YELLOW "is_dlg_id in handle ERB function  %x" ANSI_COLOR_RESET "\n",ic_dlg_id); */
/* 			return 0; */
//		}  
		//можно просто положить этот эвент в лог файл
		//23012017	INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN);
		//return 0;
	//	}
}

