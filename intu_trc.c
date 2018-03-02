/*
   Copyright (C) Dialogic Corporation 1998-2016. All Rights Reserved.

   Name:    intu_trc.c

   Description:  Display procedures for tracing the INAP Test Utility - INTU

   Functions:

   INTU_disp_other_msg             INTU_disp_dlg_msg
   INTU_disp_srv_ind               INTU_disp_srv_req
   INTU_disp_invalid_srv_ind       INTU_disp_invalid_dlg_ind
   INTU_disp_unexpected_srv_ind    INTU_disp_unexpected_dlg_ind
   INTU_disp_dlg_reopened          INTU_disp_state_change
   INTU_disp_param

   -----   ---------  -----     ------------------------------------
   Issue    Date       By        Changes
   -----   ---------  -----     ------------------------------------
     A     22-Dec-98   JET      - Initial code.
     1     09-Sep-04   GNK      - Eliminate warnings in state_text declaraion.
                                - Update copyright owner & year.
     -     20-Jul-06   JTD      - Updates to support INAPAPI DLL
     2     13-Dec-06   ML       - Change to use of Dialogic Corporation copyright.                       
     3     29-Mar-10   JLP      - INTU_disp_param() param_len changed from u8 to PLEN
     4     16-Mar-16   CJM      - Use API Extended Dialog ID functions
*/

#include "intu_def.h"

/*
 * Module variables, stored in intu.c updated by command line options in
 * intu_main.c
 */
extern u16 intu_options;     /* Defines which tracing options are configured */

static char* state_text[4] =
{
  "IDLE",                /* 0 */
  "OPEN",                /* 1 */
  "PENDING_DELIMIT",     /* 2 */
  "CLOSING"              /* 3 */
};

/*
 *******************************************************************************
 *                                                                             *
 * Message display procedures                                                  *
 *                                                                             *
 *******************************************************************************
 */

/*
 * INTU_disp_other_msg handles any other messages
 *
 * Always returns zero.
 */
int INTU_disp_other_msg(h)
  HDR   *h;             /* received message */
{
  int   instance;       /* message instance */
  u16   mlen;           /* message length */
  u8    *pptr;          /* pointer into parameter area */
  char  char_pair[2];   /* stores two ascii chars used to represent one u8 */

  instance = GCT_get_instance(h);
  printf("INTU: I%02x M t%04x i%04x f%02x d%02x s%02x", instance, h->type,
          h->id, h->src, h->dst, h->status);

  if ((mlen = ((MSG*) h)->len) > 0)
  {
    if (mlen > MAX_PARAM_LEN)
      mlen = MAX_PARAM_LEN;
    printf(" p");
    pptr = get_param((MSG *) h);
    while (mlen--)
    {
      bintoasc(char_pair, (unsigned char) *pptr++);
      printf("%c%c", char_pair[0], char_pair[1]);
    }
  }
  printf("\n");
  return(0);
}

/*
 * INTU_disp_invalid_dlg_ind()
 * Displays a warning a dialogue indication was received that was of an
 * invalid type.
 *
 * Returns 0
 */
int INTU_disp_invalid_dlg_ind(dlg_id, dlg_ptr, h)
  u32  dlg_id;             /* Dialogue ID for message */
  DLG_CB *dlg_ptr;         /* Pointer to the per-dialogue id control block */
  HDR    *h;               /* Received message */
{
  /*
   * If tracing was on we don't need to display the message because
   * it will have been done already, otherwise do it now.
   */
  if ( !(intu_options & INTU_OPT_TR_DLG_IND) )
    INTU_disp_other_msg(h);

  fprintf(stderr,
  "INTU: *** Invalid dialogue indication - dialogue 0x%08lx in %s state ***\n",
  dlg_id, state_text[dlg_ptr->state]);

  return(0);
}

/*
 * INTU_disp_invalid_srv_ind()
 * Displays a warning a service indication was received that was of an
 * invalid type.
 *
 * Returns 0
 */
int INTU_disp_invalid_srv_ind(dlg_id, dlg_ptr, h)
  u32  dlg_id;             /* Dialogue ID for message */
  DLG_CB *dlg_ptr;         /* Pointer to the per-dialogue id control block */
  HDR    *h;               /* Received message */
{
  /*
   * If tracing was on we don't need to display the message because
   * it will have been done already, otherwise do it now.
   */
  if ( !(intu_options & INTU_OPT_TR_SRV_IND) )
    INTU_disp_other_msg(h);

  fprintf(stderr,
  "INTU: *** Invalid service indication - dialogue 0x%08lx in %s state ***\n",
  dlg_id, state_text[dlg_ptr->state]);

  return(0);
}


/*
 * INTU_disp_unexpected_srv_ind()
 * Displays an unexpectedly received service indication,
 * message valid but not in this state.
 *
 * Returns 0
 */
int INTU_disp_unexpected_srv_ind(dlg_id, dlg_ptr, h)
  u32  dlg_id;             /* Dialogue ID for message */
  DLG_CB *dlg_ptr;         /* Pointer to the per-dialogue id control block */
  HDR    *h;               /* Received message */
{
  /*
   * If tracing was on we don't need to display the message because
   * it will have been done already, otherwise do it now.
   */
  if ( !(intu_options & INTU_OPT_TR_SRV_IND) )
    INTU_disp_srv_ind(dlg_id, dlg_ptr, h);

  fprintf(stderr,
  "INTU: *** Unexpected service indication - dialogue 0x%08lx in %s state ***\n",
  dlg_id, state_text[dlg_ptr->state]);

  return(0);
}

/*
 * INTU_disp_unexpected_dlg_ind()
 * Displays an unexpectedly received dialogue indication
 *
 * Returns 0
 */
int INTU_disp_unexpected_dlg_ind(dlg_id, dlg_ptr, h)
  u32  dlg_id;             /* Dialogue ID for message */
  DLG_CB *dlg_ptr;         /* Pointer to the per-dialogue id control block */
  HDR    *h;               /* Received message */
{
  /*
   * If tracing was on we don't need to display the message because
   * it will have been done already, otherwise do it now.
   */
  if ( !(intu_options & INTU_OPT_TR_DLG_IND) )
    INTU_disp_dlg_msg(dlg_id, h);

  fprintf(stderr,
  "INTU: *** Unexpected dialogue indication - dialogue 0x%08lx in %s state ***\n",
  dlg_id, state_text[dlg_ptr->state]);

  return(0);
}

/*
 * INTU_disp_dlg_reopened()
 * Displays an unexpectedly re-opened dialogue
 *
 * Returns 0
 */
int INTU_disp_dlg_reopened(dlg_id, dlg_ptr, h)
  u32  dlg_id;             /* Dialogue ID for message */
  DLG_CB *dlg_ptr;         /* Pointer to the per-dialogue id control block */
  HDR    *h;               /* Received message */
{
  /*
   * If tracing was on we don't need to display the message because
   * it will have been done already, otherwise do it now.
   */
  if ( !(intu_options & INTU_OPT_TR_DLG_IND) )
    INTU_disp_dlg_msg(dlg_id, h);

  fprintf(stderr,
  "INTU: *** Reopening dialogue 0x%08lx in %s state ***\n",
  dlg_id, state_text[dlg_ptr->state]);

  return(0);
}

/*
 * INTU_disp_state_change()
 * Displays a state change
 *
 * Returns 0
 */
int INTU_disp_state_change(ic_dlg_id, dlg_ptr, new_state)
  u32    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
  DLG_CB *dlg_ptr;       /* Pointer to the per-dialogue id control block */
  u8     new_state;      /* New state to change the dlg pointed to by dlg_ptr */
{

  if (intu_options & INTU_OPT_TR_STATE)
  {
    if (dlg_ptr->state == new_state)
    {
      fprintf(stderr,
      "INTU: *** Invalid state change for dialogue 0x%08lx from %s to %s ***\n",
      ic_dlg_id, state_text[dlg_ptr->state], state_text[new_state]);
    }
    else
    {
      printf(
      "INTU: State change for dialogue 0x%08lx from %s to %s\n",
      ic_dlg_id, state_text[dlg_ptr->state], state_text[new_state]);
    }
  }

  return(0);
}

/*
 * INTU_disp_dlg_msg()
 * Displays to the screen the message being sent
 *
 * Returns 0 if the disp was successfull
 *         INTUE_INVALID_DLG_ID if the id was invalid
 */
int INTU_disp_dlg_msg(dlg_id, h)
  u32  dlg_id;          /* Dialogue ID for message */
  HDR *h;               /* Received message */
{
  u8    test_data[30];    /* Stores the decoded parameter*/
  PLEN  test_len;         /* Stores the length of the decoded parameter */
  u8    dialogue_type;    /* Is the message an OPEN, CLOSE, etc */

  if (h != 0)
  {
    switch (h->type)
    {
      case INAP_MSG_DLG_REQ: printf("INTU: DLG-REQ "); break;
      case INAP_MSG_DLG_IND: printf("INTU: DLG-IND "); break;
      default:               return(INTUE_INVALID_DLG_ID);
    }

    if (IN_get_dialogue_type(h, &dialogue_type) != IN_SUCCESS)
      return(INTUE_MSG_DECODE_ERROR);

    switch (dialogue_type)
    {
       case INDT_OPEN:
         printf("OPEN, dialogue 0x%08lx\n", dlg_id);
         if (intu_options & INTU_OPT_TR_DLG_PARAM)
         {
           IN_get_dialogue_param(INDP_dest_address, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("dest_address", INDP_dest_address,test_len, test_data);
           IN_get_dialogue_param(INDP_orig_address, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("orig_address", INDP_orig_address,test_len, test_data);
           IN_get_dialogue_param(INDP_dest_FE, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("dest_FE", INDP_dest_FE,test_len, test_data);
           IN_get_dialogue_param(INDP_orig_FE, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("orig_FE", INDP_orig_FE,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("applic_context", INDP_applic_context,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context_index, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("applic_context_index", INDP_applic_context_index,test_len, test_data);
         }
         break;

       case INDT_CLOSE:
         printf("CLOSE, dialogue 0x%08lx\n", dlg_id);
         if (intu_options & INTU_OPT_TR_DLG_PARAM)
         {
           IN_get_dialogue_param(INDP_release_method, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("release_method", INDP_release_method,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("applic_context", INDP_applic_context,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context_index, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("applic_context_index", INDP_applic_context_index,test_len, test_data);
         }
         break;

       case INDT_DELIMIT:
         printf("DELIMIT, dialogue 0x%08lx\n", dlg_id);
         if (intu_options & INTU_OPT_TR_DLG_PARAM)
         {
           IN_get_dialogue_param(INDP_applic_context, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Applic_Context", INDP_applic_context,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context_index, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Applic_Context_Index", INDP_applic_context_index,test_len, test_data);
         }
         break;

       case INDT_U_ABORT:
         printf("U_ABORT, dialogue 0x%08lx\n", dlg_id);
         if (intu_options & INTU_OPT_TR_DLG_PARAM)
         {
           IN_get_dialogue_param(INDP_user_rsn, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("User_Rsn", INDP_user_rsn,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Applic_Context", INDP_applic_context,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context_index, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Applic_Context_Index", INDP_applic_context_index,test_len, test_data);
         }
         break;

       case INDT_P_ABORT:
         printf("P_ABORT, dialogue 0x%08lx\n", dlg_id);
         if (intu_options & INTU_OPT_TR_DLG_PARAM)
         {
           IN_get_dialogue_param(INDP_prov_rsn, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Prov_Rsn", INDP_prov_rsn,test_len, test_data);
           IN_get_dialogue_param(INDP_source, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Source", INDP_source,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Applic_Context", INDP_applic_context,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context_index, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Applic_Context_Index", INDP_applic_context_index,test_len, test_data);
         }
         break;

       case INDT_OPEN_RSP:
         printf("OPEN_RSP, dialogue 0x%08lx\n", dlg_id);
         if (intu_options & INTU_OPT_TR_DLG_PARAM)
         {
           IN_get_dialogue_param(INDP_result, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Result",INDP_result,test_len, test_data);
           IN_get_dialogue_param(INDP_refuse_rsn, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Refuse_Rsn", INDP_refuse_rsn,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Applic_Context", INDP_applic_context,test_len, test_data);
           IN_get_dialogue_param(INDP_applic_context_index, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Applic_Context_Index", INDP_applic_context_index,test_len, test_data);
         }
         break;

       case INDT_NOTICE:
         printf("NOTICE, dialogue 0x%08lx\n", dlg_id);

         if (intu_options & INTU_OPT_TR_DLG_PARAM)
         {
           IN_get_dialogue_param(INDP_prob_diag, &test_len, test_data, sizeof(test_data),h);
           INTU_disp_param("Prob_Diag", INDP_prob_diag,test_len, test_data);
         }
         break;

       default:
         printf("UNKNOWN, dialogue 0x%08lx\n", dlg_id);
         break;
    }
  }

  return(0);
}

/*
 * INTU_disp_srv_ind()
 * Displays to the screen the message received
 *
 * Returns 0 if the disp was successfull
 *         INTUE_INVALID_DLG_ID if the id was invalid
 */
int INTU_disp_srv_ind(dlg_id, dlg_ptr, h)
  u32  dlg_id;       /* Dialogue ID for message */
  DLG_CB *dlg_ptr;   /* Pointer to the per-dialogue id control block */
  HDR    *h;         /* Received message */
{
  u8     *ind_type;  /* Service Indication component type */

  if (h != 0)
  {
    ind_type = get_param(h);

    switch (*ind_type)
    {
      case INAPST_SRV_INVOKE_IND:
        printf("INTU: SRV-IND Invoke - dialogue 0x%08lx in %s state\n",
              dlg_id, state_text[dlg_ptr->state]);
        break;
      case INAPST_SRV_RESULT_IND:
        printf("INTU: SRV-IND Result - dialogue 0x%08lx in %s state\n",
          dlg_id, state_text[dlg_ptr->state]);
        break;
      case INAPST_SRV_ERROR_IND :
        printf("INTU: SRV-IND Invoke - dialogue 0x%08lx in %s state\n",
           dlg_id, state_text[dlg_ptr->state]);
        break;
      default:
        fprintf(stderr,"INTU: *** Invalid service indication - dialogue 0x%08lx in %s state ***\n",
              dlg_id, state_text[dlg_ptr->state]);
        break;
    }
  }
  return(0);
}

/*
 * INTU_disp_srv_req()
 * Displays to the screen the message being sent
 *
 * Returns 0 if the disp was successfull
 *         INTUE_INVALID_DLG_ID if the id was invalid
 */
int INTU_disp_srv_req(dlg_id, dlg_ptr, h)
  u32  dlg_id;       /* Dialogue ID for message */
  DLG_CB *dlg_ptr;   /* Pointer to the per-dialogue id control block */
  HDR    *h;         /* Received message */
{
  u8     *req_type;  /* Service Request component type */

  if (h != 0)
  {
    req_type = get_param(h);

    switch (*req_type)
    {
      case INAPST_SRV_INVOKE_REQ:
        printf("INTU: SRV-REQ Invoke - dialogue 0x%08lx in %s state\n",
              dlg_id, state_text[dlg_ptr->state]);
        break;
      case INAPST_SRV_RESULT_REQ:
        printf("INTU: SRV-REQ Result - dialogue 0x%08lx in %s state\n",
          dlg_id, state_text[dlg_ptr->state]);
        break;
      case INAPST_SRV_ERROR_REQ :
        printf("INTU: SRV-REQ Invoke - dialogue 0x%08lx in %s state\n",
           dlg_id, state_text[dlg_ptr->state]);
        break;
      case INAPST_SRV_REJECT_REQ :
        printf("INTU: SRV-REQ Reject - dialogue 0x%08lx in %s state\n",
           dlg_id, state_text[dlg_ptr->state]);
        break;
      default:
        fprintf(stderr,"INTU: *** Invalid service request - dialogue 0x%08lx in %s state ***\n",
              dlg_id, state_text[dlg_ptr->state]);
        break;
    }
  }
  return(0);
}

/*
 * INTU_disp_param()
 * Displays the given parameter name and data
 *
 * Returns 0
 */
int INTU_disp_param(param_name_text, param_name_id, param_len, param_data)
  char *param_name_text;   /* Name of the parameter as a string */
  u16  param_name_id;      /* The parameter name identifier (PN)*/
  PLEN param_len;          /* The length of param_data */
  u8   *param_data;        /* Pointer to the parameter data as a u8 array */
{
  int i;

  if (param_len > 0)
  {
    printf("      Param : %-35.35s (0x%04x) Len : 0x%02x \n",param_name_text,param_name_id,param_len);
    printf("              ");
    for (i=0;i<param_len;i++)
    {
      printf("%02.2x ",param_data[i]);
    }
    printf("\n");
  }

  return(0);
}
