#include "camelgw_backend.h"

extern u8 intu_mod_id;      // The task id of this module 
extern u8 inap_mod_id;      // The task id of the INAP binary module
extern u16 intu_options;     /* Defines which tracing options are configured */

//extern u8 TNo_Answer_Timer;
//extern u8 ONo_Answer_Timer;

//extern u8 example_release_cause[2];

//extern u8 example_release_cause2[2];


/*Cause codes user in invoke ReleaseCall*/

#define INTU_cause_value_normal_unspecified (31)  //for ReleaseCall this one should be in args by specification!
#define INTU_cause_value_user_busy (17)
#define INTU_cause_value_subscriber_absent (20)
#define INTU_cause_call_rejected (21)
#define INTU_cause_call_bearer_capability_not_authorized (57)

u8 example_release_cause[2]= {0,0x80+INTU_cause_value_normal_unspecified};
u8 example_release_cause2[2] = {0x80, 0x80 + INTU_cause_value_subscriber_absent}; 



/***************************************************************************************************************************************


slice volume = 0.1 s unit
example, for 15 s slice volume = 150
for 3 min slice volume = 1800
***************************************************************************************************************************************/
int CAMELGW_invoke_apply_charging(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, u8 ReleaseFlag, unsigned int slice_volume) {

   
    IN_CPT	og_cpt1;			// Holds INAP component whilst building into message 
     HDR	*og_h1;				// Message used to send immediate replies 
    struct calldetails *dlg_calldetails_ptr;  
    u8	max_call_duration[]={0,0};	
    u8	Rel_If_Dur_Excd[] = {129,1,0};
    u8	apc[]={36,0,2}; 			// max call duration in apply_charging message 
    int status;

    IN_CPT *dlg_cpt_ptr;


    dlg_cpt_ptr = &(dlg_ptr->cpt); 
    dlg_calldetails_ptr = &(dlg_ptr->call_details);


    printf("debug:invoke_apply_charging start\n");
	/* INOP_ApplyCharging */
#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt1,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt1);
#endif


#ifdef IN_LMSGS
	if ((og_h1 = IN_alloc_message(0)) == 0)
#else
	if ((og_h1 = IN_alloc_message()) == 0)
#endif
	    {
		/*
		* We can't get a message to reply with so just return
		*/
		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	    }

	//elmir max_call_duration[1]=t[1];
	//prod variant
	/* max_call_duration[0] = (dlg_calldetails_ptr->quota * 10) >> 8; */
	/* max_call_duration[1] = 0x00ff & (dlg_calldetails_ptr->quota * 10); */
	//	printf("Отправляем в ответ сообщение ApplyCharging\n");

	//test variant
	max_call_duration[0] = slice_volume >> 8;
	max_call_duration[1] = 0x00ff & slice_volume;


	IN_set_operation(INOP_ApplyCharging, INTU_ReleaseCall_timeout, &og_cpt1);
	//	IN_set_component_param(INPN_InvokeID, 1, &dlg_ptr->current_invoke_id, &og2_cpt);
	IN_set_component_param(INPN_InvokeID, 1, &dlg_ptr->current_invoke_id, &og_cpt1);

	dlg_ptr->current_invoke_id++;

	IN_set_component_param(INPN_MaxCallPeriodDuration, 2, max_call_duration, &og_cpt1); // max_call_duration here comes as a pointer
	
	if (ReleaseFlag == RELEASE ){
	    IN_set_component_param(INPN_Tone, 1, Rel_If_Dur_Excd+2, &og_cpt1); //set Tone = False and release if dur exceed automatically enabled
	}

	IN_set_component_param(INPN_SendingSideID(0), 1, apc+2, &og_cpt1);

	status = IN_code_operation_invoke(ic_dlg_id, &og_cpt1, og_h1);
	

	if (status == IN_SUCCESS)
	  {
		if ( INTU_send_message(intu_mod_id, inap_mod_id, og_h1) != IN_SUCCESS )
		    IN_free_message(og_h1);
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
	//	fprintf(stderr,ANSI_COLOR_YELLOW "is_dlg_id %x" ANSI_COLOR_RESET "\n",ic_dlg_id);
    return 0;
}
/*************************************************************************************
INPUT: slice_volume - quota duration in 0.1s units (CAP spec) = quota
as it goes in ACR, for example for 3 minutes will be 1800 in AC
*********************************************************************************/
int CAMELGW_call_duration_control(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, u8 ReleaseFlag, unsigned int quota)
{
    
    IN_CPT	og_cpt1;			// Holds INAP component whilst building into message 
    IN_CPT	og_cpt2;	
    IN_CPT	og_cpt3;	
    
    HDR	*og_h1;				// Message used to send immediate replies 
    HDR	*og_h2;
    HDR	*og_h3;
    u8	max_call_duration[]={0,0};	
    u8	Rel_If_Dur_Excd[] = {129,1,0};
    u8	apc[]={36,0,2}; 			// max call duration in apply_charging message 


u8 TNo_Answer_Timer = 30;
u8 ONo_Answer_Timer = 30;

    
    static	u8 eventTypeBCSM_array_DP2[8]; //events for MO
    static	u8 eventTypeBCSM_array_DP12[8]; //events for MT
    static	u8 monitorMode_array[2];
    static	u8 legID_array[2];
    
    
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

    
    
    
    IN_CPT	*dlg_cpt_ptr;
    struct calldetails *dlg_calldetails_ptr; 
    int status;
    /* необходимо разорвать установление голосового соединения */
    dlg_cpt_ptr = &(dlg_ptr->cpt); 
    
    
    
    dlg_calldetails_ptr = &(dlg_ptr->call_details);


    //max duration of quota should be value of 2 octest maximum

    /*production code */
    /*1800 in ACR = 180 seconds = 3 minutes*/
   
    //    max_call_duration[0] = (dlg_calldetails_ptr->quota * 10) >> 8;
    //max_call_duration[1] = 0x00ff & (dlg_calldetails_ptr->quota * 10);
    
   
    max_call_duration[0] = quota >> 8;
    max_call_duration[1] = 0x00ff & quota;


    /*test code for m2m */
    //    max_call_duration[0] = ( ( dlg_calldetails_ptr->quota) ) >> 8;
    //max_call_duration[1] = 0x00ff & ( (dlg_calldetails_ptr->quota) );


#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt1,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt1);
#endif

	/* INOP_ApplyCharging */
#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt2,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt2);
#endif

	/* INOP_RequestReportBCSMEvent */
#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt3,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt3);
#endif

#ifdef IN_LMSGS
	if ((og_h1 = IN_alloc_message(0)) == 0)
#else
	if ((og_h1 = IN_alloc_message()) == 0)
#endif
	    {
		/*
		* We can't get a message to reply with so just return
		*/
		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	    }
#ifdef IN_LMSGS
	if ((og_h2 = IN_alloc_message(0)) == 0)
#else
	if ((og_h2 = IN_alloc_message()) == 0)
#endif
	    {

		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	    }	
	
#ifdef IN_LMSGS
	if ((og_h3 = IN_alloc_message(0)) == 0)
#else
	if ((og_h3 = IN_alloc_message()) == 0)
#endif
	{
		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	}

			if (dlg_calldetails_ptr->uc_EventTypeBCSM == 12)
			    {
				IN_set_operation(INOP_RequestReportBCSMEvent, INTU_ReleaseCall_timeout, &og_cpt3);
				//				IN_set_component_param(INPN_InvokeID,1,&invokeID,&og3_cpt);
				IN_set_component_param(INPN_InvokeID, 1, &dlg_ptr->current_invoke_id, &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(0), 1, &eventTypeBCSM_array_DP12[0], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(0), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(0), 1, &legID_array[1], &og_cpt3);  //sendingside = legid =2	
				IN_set_component_param(INPN_EventTypeBCSM(1), 1, &eventTypeBCSM_array_DP12[1] , &og_cpt3); 
				IN_set_component_param(INPN_MonitorMode(1), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(1), 1, &legID_array[1], &og_cpt3);
				IN_set_component_param(INPN_ApplicationTimer(1), 1, &TNo_Answer_Timer, &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(2), 1,  &eventTypeBCSM_array_DP12[2], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(2), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(2), 1, &legID_array[1], &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(3), 1, &eventTypeBCSM_array_DP12[3], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(3), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(3), 1, &legID_array[0], &og_cpt3); 
				IN_set_component_param(INPN_EventTypeBCSM(4), 1, &eventTypeBCSM_array_DP12[4], &og_cpt3);   
				IN_set_component_param(INPN_MonitorMode(4), 1, &monitorMode_array[0], &og_cpt3); //interrupted mode 
				IN_set_component_param(INPN_SendingSideID(4), 1, &legID_array[1], &og_cpt3); 
				IN_set_component_param(INPN_EventTypeBCSM(5), 1, &eventTypeBCSM_array_DP12[5], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(5), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(5), 1, &legID_array[0], &og_cpt3); //legid = 1
			    }

			else if (dlg_calldetails_ptr->uc_EventTypeBCSM == 2)
			    {

				IN_set_operation(INOP_RequestReportBCSMEvent, INTU_ReleaseCall_timeout, &og_cpt3);
				IN_set_component_param(INPN_InvokeID,1, &dlg_ptr->current_invoke_id, &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(0), 1, &eventTypeBCSM_array_DP2[0], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(0), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(1), 1, &eventTypeBCSM_array_DP2[1], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(1), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(2), 1, &eventTypeBCSM_array_DP2[2]  , &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(2), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_ApplicationTimer(2), 1, &ONo_Answer_Timer, &og_cpt3);

				IN_set_component_param(INPN_EventTypeBCSM(3), 1, &eventTypeBCSM_array_DP2[3], &og_cpt3); //ODisconnect
				IN_set_component_param(INPN_MonitorMode(3), 1, &monitorMode_array[0], &og_cpt3); //mode interrupted
				IN_set_component_param(INPN_SendingSideID(3), 1, &legID_array[0], &og_cpt3); //legid = 1
			

				
				IN_set_component_param(INPN_EventTypeBCSM(4), 1, &eventTypeBCSM_array_DP2[4], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(4), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(4), 1, &legID_array[1], &og_cpt3);
				
				IN_set_component_param(INPN_EventTypeBCSM(5), 1, &eventTypeBCSM_array_DP2[5] , &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(5), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(6), 1, &eventTypeBCSM_array_DP2[6], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(6), 1, &monitorMode_array[1], &og_cpt3);
				//	printf("test2\n");
			    }
			else
			    {
		fprintf(stderr, "INTU: *** Unknown eventtype value for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return 1;
			    }
			    
			status = IN_code_operation_invoke(ic_dlg_id, &og_cpt3, og_h3);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h3) != IN_SUCCESS)
			IN_free_message(og_h3);
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
			IN_set_operation(INOP_ApplyCharging, INTU_ReleaseCall_timeout, &og_cpt2);
			IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og_cpt2);
			IN_set_component_param(INPN_MaxCallPeriodDuration, 2, max_call_duration, &og_cpt2); /* max_call_duration here comes as a pointer */
			//	status = IN_set_component_param(INPN_ReleaseIfDurExceeded, 1, apc+1, &og2_cpt);
			if (ReleaseFlag == RELEASE ){
				IN_set_component_param(INPN_Tone, 1, Rel_If_Dur_Excd+2, &og_cpt2); //set Tone = False and release if dur exceed automatically enabled
			}
			//status = IN_set_component_param(INPN_RelIfDurEx_Ellipsis, 2, Rel_If_Dur_Excd +1, &og2_cpt);
			IN_set_component_param(INPN_SendingSideID(0), 1, apc+2, &og_cpt2);
			status = IN_code_operation_invoke(ic_dlg_id, &og_cpt2, og_h2);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h2) != IN_SUCCESS)
			IN_free_message(og_h2);
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
			IN_set_operation(INOP_Continue, INTU_ReleaseCall_timeout, &og_cpt1);
			IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og_cpt1);
			status = IN_code_operation_invoke(ic_dlg_id, &og_cpt1, og_h3);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h3) != IN_SUCCESS)
			IN_free_message(og_h3);
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
			//			fprintf(stderr, ANSI_COLOR_YELLOW "is_dlg_id %x at the end of handle IDP function" ANSI_COLOR_RESET "\n",ic_dlg_id);
			//}
//	}

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

/*************************************************************************************
INPUT: slice_volume - quota duration in 0.1s units (CAP spec) = quota
as it goes in ACR, for example for 3 minutes will be 1800 in AC
*********************************************************************************/
int CAMELGW_invoke_triplet(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 ReleaseFlag, unsigned int quota)
{
    
    IN_CPT	og_cpt1;			// Holds INAP component whilst building into message 
    IN_CPT	og_cpt2;	
    IN_CPT	og_cpt3;	
    
    HDR	*og_h1;				// Message used to send immediate replies 
    HDR	*og_h2;
    HDR	*og_h3;
    u8	max_call_duration[]={0,0};	
    u8	Rel_If_Dur_Excd[] = {129,1,0};
    u8	apc[]={36,0,2}; 			// max call duration in apply_charging message 


    //static u8 TNo_Answer_Timer = 30;
    //static u8 ONo_Answer_Timer = 30;

    
    //    static	u8 eventTypeBCSM_array_DP2[8]; //events for MO
    //static	u8 eventTypeBCSM_array_DP12[8]; //events for MT
    //static	u8 monitorMode_array[2];
    //static	u8 legID_array[2];

 u8 TNo_Answer_Timer = 30;
 u8 ONo_Answer_Timer = 30;

    
   u8 eventTypeBCSM_array_DP2[8]; //events for MO
   u8 eventTypeBCSM_array_DP12[8]; //events for MT
   u8 monitorMode_array[2];
   u8 legID_array[2];

   u8 _legID;
    
    
    monitorMode_array[0] = interrupted;
    monitorMode_array[1] = notifyAndContinue;
    
    legID_array[0] = 1;
    legID_array[1] = 2;
    
    
    
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

    
    
    
    IN_CPT	*dlg_cpt_ptr;
    struct calldetails *dlg_calldetails_ptr; 
    int status;
    /* необходимо разорвать установление голосового соединения */
    dlg_cpt_ptr = &(dlg_ptr->cpt); 
    
    
    
    dlg_calldetails_ptr = &(dlg_ptr->call_details);


    //max duration of quota should be value of 2 octest maximum

    /*production code */
    /*1800 in ACR = 180 seconds = 3 minutes*/
   
    //    max_call_duration[0] = (dlg_calldetails_ptr->quota * 10) >> 8;
    //max_call_duration[1] = 0x00ff & (dlg_calldetails_ptr->quota * 10);
    
   
    max_call_duration[0] = quota >> 8;
    max_call_duration[1] = 0x00ff & quota;


    /*test code for m2m */
    //    max_call_duration[0] = ( ( dlg_calldetails_ptr->quota) ) >> 8;
    //max_call_duration[1] = 0x00ff & ( (dlg_calldetails_ptr->quota) );


#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt1,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt1);
#endif

	/* INOP_ApplyCharging */
#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt2,0);
#else
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt2);
#endif

	/* INOP_RequestReportBCSMEvent */
//#ifdef IN_LMSGS
status = 	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt3,0);

 printf("status of inited component = %d\n", status);
 printf("pointer of inited component = %p\n", &og_cpt3);
//#else
//	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt3);
//#endif

#ifdef IN_LMSGS
	if ((og_h1 = IN_alloc_message(0)) == 0)
#else
	if ((og_h1 = IN_alloc_message()) == 0)
#endif
	    {
		/*
		* We can't get a message to reply with so just return
		*/
		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	    }
#ifdef IN_LMSGS
	if ((og_h2 = IN_alloc_message(0)) == 0)
#else
	if ((og_h2 = IN_alloc_message()) == 0)
#endif
	    {

		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	    }	
	
#ifdef IN_LMSGS
	if ((og_h3 = IN_alloc_message(0)) == 0)
#else
	if ((og_h3 = IN_alloc_message()) == 0)
#endif
	{
		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	}

printf("pointer of inited msg = %p\n", &og_h3);

			if (dlg_calldetails_ptr->uc_EventTypeBCSM == 12)
			    {
				_legID = 1;
				IN_set_operation(INOP_RequestReportBCSMEvent, INTU_RequestReportBCSMEvent_timeout,&og_cpt3);
				//				IN_set_component_param(INPN_InvokeID,1,&invokeID,&og3_cpt);
				IN_set_component_param(INPN_InvokeID, 1, &dlg_ptr->current_invoke_id, &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(0), 1, &eventTypeBCSM_array_DP12[0], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(0), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(0), 1, &legID_array[1], &og_cpt3);  //sendingside = legid =2	
				IN_set_component_param(INPN_EventTypeBCSM(1), 1, &eventTypeBCSM_array_DP12[1] , &og_cpt3); 
				IN_set_component_param(INPN_MonitorMode(1), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(1), 1, &legID_array[1], &og_cpt3);
				IN_set_component_param(INPN_ApplicationTimer(1), 1, &TNo_Answer_Timer, &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(2), 1,  &eventTypeBCSM_array_DP12[2], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(2), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(2), 1, &legID_array[1], &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(3), 1, &eventTypeBCSM_array_DP12[3], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(3), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(3), 1, &legID_array[0], &og_cpt3); 
				IN_set_component_param(INPN_EventTypeBCSM(4), 1, &eventTypeBCSM_array_DP12[4], &og_cpt3);   
				IN_set_component_param(INPN_MonitorMode(4), 1, &monitorMode_array[0], &og_cpt3); //interrupted mode 
				IN_set_component_param(INPN_SendingSideID(4), 1, &legID_array[1], &og_cpt3); 
				IN_set_component_param(INPN_EventTypeBCSM(5), 1, &eventTypeBCSM_array_DP12[5], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(5), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(5), 1, &legID_array[0], &og_cpt3); //legid = 1


			    }

			else if (dlg_calldetails_ptr->uc_EventTypeBCSM == 2)
			    {
				_legID = 2;
				status = IN_set_operation(INOP_RequestReportBCSMEvent, 3, &og_cpt3);
				status = IN_set_component_param(INPN_InvokeID,1, &dlg_ptr->current_invoke_id, &og_cpt3);
				status = IN_set_component_param(INPN_EventTypeBCSM(0), 1, &eventTypeBCSM_array_DP2[0], &og_cpt3);
				status =IN_set_component_param(INPN_MonitorMode(0), 1, &monitorMode_array[1], &og_cpt3);
				status =IN_set_component_param(INPN_EventTypeBCSM(1), 1, &eventTypeBCSM_array_DP2[1], &og_cpt3);
				status =IN_set_component_param(INPN_MonitorMode(1), 1, &monitorMode_array[1], &og_cpt3);
				status =IN_set_component_param(INPN_EventTypeBCSM(2), 1, &eventTypeBCSM_array_DP2[2]  , &og_cpt3);
				status =IN_set_component_param(INPN_MonitorMode(2), 1, &monitorMode_array[1], &og_cpt3);
				status =IN_set_component_param(INPN_ApplicationTimer(2), 1, &ONo_Answer_Timer, &og_cpt3);

				status =IN_set_component_param(INPN_EventTypeBCSM(3), 1, &eventTypeBCSM_array_DP2[3], &og_cpt3); //ODisconnect
				status =IN_set_component_param(INPN_MonitorMode(3), 1, &monitorMode_array[0], &og_cpt3); //mode interrupted
				status =IN_set_component_param(INPN_SendingSideID(3), 1, &legID_array[0], &og_cpt3); //legid = 1

				IN_set_component_param(INPN_EventTypeBCSM(4), 1, &eventTypeBCSM_array_DP2[4], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(4), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_SendingSideID(4), 1, &legID_array[1], &og_cpt3);

				IN_set_component_param(INPN_EventTypeBCSM(5), 1, &eventTypeBCSM_array_DP2[5] , &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(5), 1, &monitorMode_array[1], &og_cpt3);
				IN_set_component_param(INPN_EventTypeBCSM(6), 1, &eventTypeBCSM_array_DP2[6], &og_cpt3);
				IN_set_component_param(INPN_MonitorMode(6), 1, &monitorMode_array[1], &og_cpt3);

			    }
			else
			    {
		fprintf(stderr, "INTU: *** Unknown eventtype value for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return 1;
			    }

			int error;
			status = IN_get_component_first_error(&og_cpt3, &error);
			printf("error code = %d\n", error);			    


 			printf("prot spec  = %p\n",og_cpt3.prot_spec);      /* Pointer to the protocol spec used by the component */


    			printf("first_error = %d\n", og_cpt3.first_error);                   /* API Error code of the first failure  */
  			printf("first_error_reason = %d\n", og_cpt3.first_error_reason) ;            /* Identifier reason of the first error */
  			printf(" operation       = %d\n", og_cpt3.operation);                     /* Operation Code */
  			printf(" timeout         = %d\n", og_cpt3.timeout);                      /* Operation timeout value */
  			printf("  err            = %d\n", og_cpt3.err);                          /* Operation Error Code */
  			printf("  type           = %d\n", og_cpt3.type);                         /* Is it an invoke, result or error */
  			printf("  options        = %d\n", og_cpt3.options);                      /* Options mask */
  			printf("  databuf_offset = %d\n", og_cpt3.databuf_offset);               /* Size of completed databuf */
	

			status = IN_code_operation_invoke(ic_dlg_id, &og_cpt3, og_h3);

			status = IN_get_component_first_error(&og_cpt3, &error);
			printf("error code = %d\n", error);			    


			printf("code operation invoke status = %d\n", status);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h3) != IN_SUCCESS)
		    {
			IN_free_message(og_h3);
			printf("error!\n");
		    }
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
			IN_set_operation(INOP_ApplyCharging, INTU_ApplyCharging_timeout, &og_cpt2);
			IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og_cpt2);
			IN_set_component_param(INPN_MaxCallPeriodDuration, 2, max_call_duration, &og_cpt2); /* max_call_duration here comes as a pointer */
			//	status = IN_set_component_param(INPN_ReleaseIfDurExceeded, 1, apc+1, &og2_cpt);
			if (ReleaseFlag == RELEASE )
{
				IN_set_component_param(INPN_Tone, 1, Rel_If_Dur_Excd+2, &og_cpt2); //set Tone = False and release if dur exceed automatically enabled
			}
			//status = IN_set_component_param(INPN_RelIfDurEx_Ellipsis, 2, Rel_If_Dur_Excd +1, &og2_cpt);

			//
			//
			IN_set_component_param(INPN_SendingSideID(0), 1, &_legID, &og_cpt2); //leg id 1 or 2
			status = IN_code_operation_invoke(ic_dlg_id, &og_cpt2, og_h2);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h2) != IN_SUCCESS)
			IN_free_message(og_h2);
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
			IN_set_operation(INOP_Continue, INTU_ReleaseCall_timeout, &og_cpt1);
			IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og_cpt1);
			status = IN_code_operation_invoke(ic_dlg_id, &og_cpt1, og_h1);

	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h1) != IN_SUCCESS)
			IN_free_message(og_h1);
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

			fprintf(stderr, "CAMELGW: *** Далее пытаюсь сменить состояние на OPEN in file %s function %s ***\n", __FILE__, __PRETTY_FUNCTION__);
			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_OPEN);
			//			fprintf(stderr, ANSI_COLOR_YELLOW "is_dlg_id %x at the end of handle IDP function" ANSI_COLOR_RESET "\n",ic_dlg_id);
			fprintf(stderr,  "ic_dlg_id %x at the end of handle IDP function\n",ic_dlg_id);
			//}
//	}

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


/*invoke continue on SSF and do prearranged end on SCP */

/*simple Invoke Continue goes with prearranged end */
/* dlg_inaprm = dialogue release method, user prearranged end here*/

//int CAMELGW_invoke_continue(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr)//, u8 dlg_inaprm)
int CAMELGW_invoke_continue(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h)//, u8 *invokeid_ptr)//, u8 dlg_inaprm)
{

	IN_CPT	og_cpt;			// Holds INAP component whilst building into message 
	HDR	*og_h;				// Message used to send immediate replies 
IN_CPT	*dlg_cpt_ptr;
	int status;
				/* необходимо разорвать установление голосового соединения */
	dlg_cpt_ptr = &(dlg_ptr->cpt); 

	//#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);
	//#else
	//	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt);
	//#endif

	    //#ifdef IN_LMSGS
		if ( (og_h = IN_alloc_message(0) ) == 0)
		    //#else
		    //		    if ((og_h = IN_alloc_message()) == 0)
		    //#endif
			{
			    fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
			    return INTUE_MSG_ALLOC_FAILED;
			}

		IN_set_operation(INOP_Continue, INTU_Continue_timeout, &og_cpt);
		//		IN_set_component_param(INPN_InvokeID,1,invokeid_ptr,&og_cpt);
		IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og_cpt);

	dlg_ptr->current_invoke_id++;

		if (intu_options & INTU_OPT_EXT_DLG)
		    status = IN_EXT_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
		else
		    status = IN_code_operation_invoke((u16)ic_dlg_id, &og_cpt, og_h);

		//		status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
		
		if (status == IN_SUCCESS)
		    {
			if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
			    IN_free_message(og_h);
			
			INTU_send_delimit(ic_dlg_id);
			INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
			//			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
			return 0;
		    }
		else
		    {
			fprintf(stderr, "INTU: *** Failed to encode Continue event invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
			INTU_send_close(ic_dlg_id, INAPRM_normal_release);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			return INTUE_CODING_FAILURE;
		    }
//		INTU_send_delimit(ic_dlg_id);
//			INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
//			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
//			return 0;
}


/*invoke continue on SSF and do not close dialogue just change state to INTU_CLOSING */
/* this invoke used when receive ODisconnect with interrupt message type
/*dialogue will be closed after timeoute - Megafone case, Megafon alwwas use basic end!!! */
/* dlg_inaprm = dialogue release method, user prearranged end here*/

int CAMELGW_invoke_continue2(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h)//, u8 *invokeid_ptr)//, u8 dlg_inaprm)
{

	IN_CPT	og_cpt;			// Holds INAP component whilst building into message 
	HDR	*og_h;				// Message used to send immediate replies 
IN_CPT	*dlg_cpt_ptr;
	int status;
				/* необходимо разорвать установление голосового соединения */
	dlg_cpt_ptr = &(dlg_ptr->cpt); 

	//#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);
	//#else
	//	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt);
	//#endif

	    //#ifdef IN_LMSGS
		if ( (og_h = IN_alloc_message(0) ) == 0)
		    //#else
		    //		    if ((og_h = IN_alloc_message()) == 0)
		    //#endif
			{
			    fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
			    return INTUE_MSG_ALLOC_FAILED;
			}

		IN_set_operation(INOP_Continue, INTU_Continue_timeout, &og_cpt);
		//		IN_set_component_param(INPN_InvokeID,1,invokeid_ptr,&og_cpt);

		IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og_cpt);

	dlg_ptr->current_invoke_id++;


		if (intu_options & INTU_OPT_EXT_DLG)
		    status = IN_EXT_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
		else
		    status = IN_code_operation_invoke((u16)ic_dlg_id, &og_cpt, og_h);

		//		status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
		
		if (status == IN_SUCCESS)
		    {
			if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
			    IN_free_message(og_h);
			
			INTU_send_delimit(ic_dlg_id);
			//INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
			//			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
			return 0;
		    }
		else
		    {
			fprintf(stderr, "INTU: *** Failed to encode Continue event invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
			INTU_send_close(ic_dlg_id, INAPRM_normal_release);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			return INTUE_CODING_FAILURE;
		    }
//		INTU_send_delimit(ic_dlg_id);
//			INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
//			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
//			return 0;
}


/*invoke continue on SSF and do not close dialogue just change state to INTU_CLOSING */
/* this invoke used when receive ODisconnect with interrupt message type
/*dialogue will be closed after timeoute - Megafone case, Megafon alwwas use basic end!!! */
/* dlg_inaprm = dialogue release method, user prearranged end here*/
/* invoke ActivityTest */
int CAMELGW_invoke_ActivityTest(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h)//, u8 *invokeid_ptr)//, u8 dlg_inaprm)
{

	IN_CPT	og_cpt;			// Holds INAP component whilst building into message 
	HDR	*og_h;				// Message used to send immediate replies 
IN_CPT	*dlg_cpt_ptr;
	int status;
				/* необходимо разорвать установление голосового соединения */
	dlg_cpt_ptr = &(dlg_ptr->cpt); 


	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);


		if ( (og_h = IN_alloc_message(0) ) == 0)

			{
			    fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
			    return INTUE_MSG_ALLOC_FAILED;
			}

		IN_set_operation(INOP_ActivityTest, 0, &og_cpt); //max value of timeout for ActivityTest
		//		IN_set_component_param(INPN_InvokeID,1,invokeid_ptr,&og_cpt);

		IN_set_component_param(INPN_InvokeID,1,&dlg_ptr->current_invoke_id,&og_cpt);

	dlg_ptr->current_invoke_id++;


		if (intu_options & INTU_OPT_EXT_DLG)
		    status = IN_EXT_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
		else
		    status = IN_code_operation_invoke((u16)ic_dlg_id, &og_cpt, og_h);

		
		if (status == IN_SUCCESS)
		    {
			if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
			    IN_free_message(og_h);
			
			INTU_send_delimit(ic_dlg_id);
			//INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
			//			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
			//			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
			return 0;
		    }
		else
		    {
			fprintf(stderr, "INTU: *** Failed to encode Continue event invoke for dialogue ID 0x%04x [%i] ***\n", ic_dlg_id, status);
			INTU_send_close(ic_dlg_id, INAPRM_normal_release);
			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
			return INTUE_CODING_FAILURE;
		    }
//		INTU_send_delimit(ic_dlg_id);
//			INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
//			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
//			return 0;
}


/* int CAMELGW_invoke_FurnishChargingInformation */
/* { */



/*     return0; */
/* } */



int CAMELGW_test(u16 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr, u8 ReleaseFlag )
{
	//ic_dlg_id, dlg_ptr, h, &invokeID, CONTINUE

    IN_CPT *dlg_cpt_ptr;


    dlg_cpt_ptr = &(dlg_ptr->cpt); 




    printf("debug:test: invoke id pointer = %p\n", invokeid_ptr);
    printf("debug:dlg cpt ptr = %p\n", dlg_cpt_ptr);
    printf("debug:ic_dlg_id = %d\n", ic_dlg_id);
    printf("debug:h ptr = %p\n", h);
    printf("debug:flag = %d\n", ReleaseFlag);
    return 0;
}

/*******************************************************************************/
/* CAMELGW_releasecall_cmd - used to send ReleaseCall invoke to SSF from CAMELGW*/
/*
//use pre arranged end for dialogue closing, ReleaseCall invoke is class 4 (without any error or reulst indication from remote end)

/*******************************************************************************/
int CAMELGW_invoke_releasecall(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, u8 *invokeid_ptr)
{

	IN_CPT	og_cpt;			// Holds INAP component whilst building into message 
	HDR	*og_h;				// Message used to send immediate replies 
IN_CPT	*dlg_cpt_ptr;
	int status;
				/* необходимо разорвать установление голосового соединения */
	dlg_cpt_ptr = &(dlg_ptr->cpt); 

	//#ifdef IN_LMSGS
	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);
	//#else
	//	IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt);
	//#endif

	    //#ifdef IN_LMSGS
	if ((og_h = IN_alloc_message(0)) == 0)
	    //#else
	    //if ((og_h = IN_alloc_message()) == 0)
	    //#endif
	{

		fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
		return INTUE_MSG_ALLOC_FAILED;
	}
	IN_set_operation(INOP_ReleaseCall, INTU_ReleaseCall_timeout, &og_cpt);
	IN_set_component_param(INPN_InvokeID,1,invokeid_ptr,&og_cpt);
	//	IN_set_component_param (INPN_Cause, sizeof(example_release_cause), example_release_cause, &og_cpt);
		IN_set_component_param (INPN_Cause, sizeof(example_release_cause2), &example_release_cause2[0], &og_cpt);

//IN_set_operation(INOP_ReleaseCall, INTU_ReleaseCall_timeout, &og_cpt);



	//				status = IN_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);

    if (intu_options & INTU_OPT_EXT_DLG)
    status = IN_EXT_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
 else
     status = IN_code_operation_invoke((u16)ic_dlg_id, &og_cpt, og_h);


	if (status == IN_SUCCESS)
	{
		if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
			IN_free_message(og_h);

			INTU_send_delimit(ic_dlg_id);
			INTU_send_close(ic_dlg_id, INAPRM_prearranged_end);
			//			INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
			INTU_change_state(ic_dlg_id, dlg_ptr, INTU_STATE_CLOSING);
			return 0;

	}
	else
	{
		fprintf(stderr, "INTU: *** Failed to encode ReleaseCall event invoke for dialogue ID 0x%08lx [%i] ***\n", ic_dlg_id, status);
		INTU_send_close(ic_dlg_id, INAPRM_normal_release);
		INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
		return INTUE_CODING_FAILURE;
	}

	//			if (status == IN_SUCCESS)
	//			    {
	//				if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
	//				    IN_free_message(og_h);
					//may be send delimit missed here
	//INTU_send_delimit(ic_dlg_id); /* add this one */
	//	INTU_send_close(ic_dlg_id, INAPRM_normal_release);
	//				INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
	//				return 0;
					//    }
					//*******************************************************

    //    if (intu_options & INTU_OPT_EXT_DLG)
    //status = IN_EXT_code_operation_invoke(ic_dlg_id, &og_cpt, og_h);
    //else
    //status = IN_code_operation_invoke((u16)ic_dlg_id, &og_cpt, og_h);

	// if (status == IN_SUCCESS)
	//{
	// if (INTU_send_message(intu_mod_id, inap_mod_id, og_h) != IN_SUCCESS)
	//   IN_free_message(og_h);

	// INTU_send_close(ic_dlg_id, INAPRM_normal_release);
	// INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_SUCCESS);
	// return(0);
	// }
	// else
	// {
	// fprintf(stderr, "INTU: *** Failed to encode invoke for dialogue ID 0x%08lx [%i] ***\n",
	//	 ic_dlg_id, status);

	// INTU_send_close(ic_dlg_id, INAPRM_normal_release);
	// INTU_close_dialogue(ic_dlg_id, dlg_ptr, h, INTU_DLG_FAILED);
	// return(INTUE_CODING_FAILURE);
	// }

}
/******* vpn service Connect command ********/
/* if ptr == NULL then todo this */
/* dest_addr_ptr = pointer to structure "79027111287" */
/* additional_calling = null terminated string with additional calling, unknown format, example "#0387" */

struct __address{
    unsigned char noai; //nature of address indicator, 2 - unknown, 3 - national, 4 - international
    unsigned char naddr;          /* number of address digits */
    unsigned char number_digits[20]; //one digit in one position of array
};


int CAMELGW_vpn_invoke_connect(u32 ic_dlg_id, DLG_CB *dlg_ptr, HDR *h, IN_CPT *buffer_cpt_ptr, struct __address *dest_addr_ptr, struct __address *generic_ptr)
{

    IN_CPT  og_cpt;
    HDR     *og_h;
    IN_CPT  *dlg_cpt_ptr;
    //    u8      dest_addr_param[10] = {0x4, 0x10, 0, 0 , 0, 0, 0,0,0,0};  // Destination Routing Address
    struct called_pty_addr *pso_CalledAddr;
 
             int status;
         unsigned char digits_buffer[32];  //one elemen is pair of digits from octets string
         unsigned char digit_buffer[32];   // one element is one digit
	 unsigned char buf1[32];
	 unsigned char buf2[32];
	 unsigned char buf3[32];

	 int i;
	 unsigned char *ptr;
     
    u16 data_length;
         int num_digits;
	    struct calldetails *dlg_calldetails_ptr;
         u8 dest_addr_plen;
	 u8 oddn;
       dlg_cpt_ptr = &(dlg_ptr->cpt);
       dlg_calldetails_ptr = &(dlg_ptr->call_details);

       pso_CalledAddr = &dlg_calldetails_ptr->CalledAddr;

        //u8 example_routing_number[15] = {0xd, 1, 6, 0 ,0}; //d1600 - prefix for CONNECT
        //u8 nat_number[]={9,5,0,6,6,5,6,0,0,3};
        //u8 generic_num[] = {0x06, 0x82, 0x10, 0x0b, 0x01, 0x02};

        u8 generic_num_length = 6;
	u8 original_called_plen;

	printf("%s :: %s received data:\n", __FILE__, __PRETTY_FUNCTION__);
	printf("dest addr digits:\n");


	for (i=0; i < dest_addr_ptr->naddr; i++)
	    printf(" %d\n", dest_addr_ptr->number_digits[i]);


	/*destination routing address preparing */


	if( ( (dest_addr_ptr->naddr) & 1 ) == 0)
	    oddn = 0; /*even*/
	else oddn = 1; /*odd */


	ptr = &buf1[0];

	bit_to_byte(ptr, oddn, 7);
	bits_to_byte(ptr++, dest_addr_ptr->noai, 0,7);
	bits_to_byte(ptr, 0, 0, 4);
	bits_to_byte(ptr, 1, 4, 3); /*numbering plan indicator filling, 1= ISDN E164, always should be this */
	bit_to_byte(ptr++, 0, 7); /*INN indicator filling */
	//bits_to_byte(called_pty_paramz,132,0,8);
	pack_digits(ptr, 0, &dest_addr_ptr->number_digits[0], dest_addr_ptr->naddr);

	printf("ptr 0 = %d\n", *ptr);
	printf("ptr 1 = %d\n", *(ptr+1));
	       printf("ptr 2 = %d\n", *(ptr+2));
	       printf("ptr 3 = %d\n", *(ptr+3));
	       printf("ptr 4 = %d\n", *(ptr+4));


	/*add filler if addr is odd */
	       //	if (oddn)
	       //*ptr &= 0x0f;

	/*generic number preparing */

	if( ( (generic_ptr->naddr) & 1 ) == 0)
	    oddn = 0; /*even*/
	else oddn = 1; /*odd */

	ptr = &buf2[0];

	bits_to_byte(ptr++, 6, 0, 8); //add number quilifier indicator , 6 is for additional calling number
	bit_to_byte(ptr, oddn, 7);  //odd even indicator
	bits_to_byte(ptr++, generic_ptr->noai, 0,7); //nature of address indicator

	bit_to_byte(ptr, 0, 7);  //number incomplete - complete = 0
	bits_to_byte(ptr, 1, 4, 3); //numbering plan indicator, should be always 1 = ISDN E164, for generic = 6
	bits_to_byte(ptr, 0, 2, 2); //address presentation restricted indicator, presentation allowed = 0
	bits_to_byte(ptr++, 0, 0, 2); //screenening indicator, 0 0 user provided, not verified

	pack_digits(ptr, 0, generic_ptr-> number_digits, generic_ptr->naddr);

	/*add filler if addr is odd */
	//if (oddn)
	//   *ptr &= 0x0f;

	/*original called  number id  preparing */


	//if( ( (generic_ptr->naddr) & 1 ) == 0)
	//  oddn = 0; /*even*/
	//	else oddn = 1; /*odd */

ptr = &buf3[0];

	bit_to_byte(ptr, pso_CalledAddr->oddi, 7);
	bits_to_byte(ptr++, 2 , 0,7); //nature of address indicator, 0000010 - unknow
	bit_to_byte(ptr, 0, 7);
	bits_to_byte(ptr, 1, 4,3); //nambering plan indicator, 1 = E164 ISDN
	bits_to_byte(ptr, 0, 2,2); //address presentation restricted indicator
	bits_to_byte(ptr++, 0, 0,2); //reserved bit 0 and bit 1 set to 0

	pack_digits(ptr, 0, pso_CalledAddr->addr, pso_CalledAddr->naddr);

	if (oddn)
	    {
		//  *ptr &= 0x0f;
	    original_called_plen = 2 + (pso_CalledAddr->naddr + 1)/2;
	    }
	else
	    original_called_plen = 2 + (pso_CalledAddr->naddr)/2;


	//original_called_plen = 2 + 


status = IN_get_component_param(INPN_CalledPartyBCDNumber, &data_length, digits_buffer, sizeof(digits_buffer), buffer_cpt_ptr);

        if (status == IN_SUCCESS)
             {
            /*****/
            //  if (status == IN_SUCCESS)
            // {
                num_digits = (data_length - 1) * 2;
                //          unpack_digits(called_pty_dgt_str, digits_buffer, 2, num_digits ); //offset goes from 2 octet
                //      unpack_digits(digit_buffer, digits_buffer, 2, num_digits ); //offset goes from 2 octet

	                /******/
 
	                     //if (digits_buffer[0] == 0x84 ) /*odd and international called num in IDP */
	                           // {
	                     //      num_digits = (data_length -2)*2 -1 -1; //do not include filler and first 7
	                     // }
	             //      if(called_pty_dgt_str[0] == INTERNATIONAL_NUMBER){
	             //              start_str=5; end_str=14;
	             //      }
	             //unpack_digits(digit_buffer, digits_buffer, 4, num_digits);//(sizeof(called_pty_param)+4));
	             //      unpack_digits(digit_buffer, digits_buffer, 5, num_digits);//offset = 5 because we need not include first 7 into digit_buffer
	             unpack_digits(digit_buffer, digits_buffer, 2, num_digits ); //offset goes from 2 octet

        //              memset(dlg_calldetails_ptr->CalledPartyNumber, 0, sizeof(dlg_calldetails_ptr->CalledPartyNumber));

        //      for (i=0;i < num_digits;i++){
        //          //sprintf(string,"%x",called_pty_dgt_str[i]);
        //          dlg_calldetails_ptr->CalledPartyNumber[i] = digit_buffer[i] + 0x30;//,string);
        //      }
        }
        else
            {
                printf("vpn_driver: something wrong in connect cmd!\n");

            }


        IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt,0);
	//     2346        IN_init_component(dlg_cpt_ptr->prot_spec,&og_cpt);
	//2347#endif

                     if ((og_h = IN_alloc_message(0)) == 0)
			 //2352#else
     //                   if ((og_h = IN_alloc_message()) == 0)
			 //2354#endif
                             {
                                 fprintf(stderr, "INTU: *** Failed allocate MSG for dialogue ID 0x%04x ***\n", ic_dlg_id);
                                 return INTUE_MSG_ALLOC_FAILED;
                             }
 
        /*
        * InvokeID received as pointer
        */
                //               this should be redone!
                /*              INTU_translate_number(&called_pty,&dest_routing_addr,regime);
 2365                dest_addr_plen = INTU_fmt_called_num(dest_addr_param, sizeof(dest_addr_param), &dest_routing_addr, eventtypebcsm_param[0]);
 2366                tp=bits_from_byte(*called_pty_param,4,3);
 2367
 2368                fprintf(stderr,ANSI_COLOR_YELLOW "tp %x" ANSI_COLOR_RESET "\n",tp);
 2369                if((tp==0)||(tp==1))
 2370                        bits_to_byte(called_pty_paramz,132,0,8);
 2371                else
 2372                        return 0;
 2373
 2374                fprintf(stderr,ANSI_COLOR_YELLOW "np %x" ANSI_COLOR_RESET "\n",np);
 2375                if ((np==1)||(np==4))
 2376                        bits_to_byte(called_pty_paramz+1,16,4,3);
 2377                else
 2378                        return 0;
 2379                */
                     //dest_addr_param[0]=0x4;
                     //dest_addr_param[1]=10;
                         /*      dest_addr_param[2]=0x1d;
        dest_addr_param[3]=6;

        dest_addr_param[4]=0x90;
         dest_addr_param[5]=0x05;
         dest_addr_param[6]=0x66;
         dest_addr_param[7]=0x65;
         dest_addr_param[8]=0x00;
         dest_addr_param[9]=0x03;*/
 
         //dest_addr_param[0]=152;
 
		     //data_length = 5;

		     // 2395digits_buffer[0] = 0x82;
		     //2396digits_buffer[1] = 0x10;
		     //2397digits_buffer[2] = 0x0c;
		     //2398digits_buffer[3] = 0x83;
		     //2399digits_buffer[4] = 0xf7;

                //memcpy(example_routing_number + 5, nat_number, sizeof(nat_number) );
                //memcpy(example_routing_number + 5, digit_buffer, num_digits );//construct destination number from D1600 and digit_buffer
                //pack_digits(dest_addr_param, 4 , example_routing_number, sizeof (example_routing_number)); //4 - offset
                dest_addr_plen = 10;
                //              generic_number_length= 6;
                IN_set_component_param(INPN_InvokeID, 1, &dlg_ptr->current_invoke_id,&og_cpt);

                IN_set_component_param(INPN_OriginalCalledPartyID, original_called_plen, &buf3[0], &og_cpt);
                //IN_set_component_param(INPN_DestinationRoutingAddress(0), dest_addr_plen, dest_addr_param, &og_cpt);

                 IN_set_component_param(INPN_DestinationRoutingAddress(0), dest_addr_plen, &buf1[0], &og_cpt);

                //IN_set_component_param(INPN_GenericNumber(1), generic_num_length, generic_num, &og_cpt);
                //IN_set_component_param(INPN_GenericNumber(1), generic_num_length, &dlg_calldetails_ptr->param1[0], &og_cpt);
 IN_set_component_param(INPN_GenericNumber(1), generic_num_length, &buf2[0], &og_cpt);

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
    return 0;
}






