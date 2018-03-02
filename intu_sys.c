/*
   Copyright (C) Dialogic Corporation 1998-2016. All Rights Reserved.

   Name:         intu_sys.c

   Description:  Message sending and formatting procedures for the INAP
                 Test Utility - INTU

   Functions:

   INTU_send_open_rsp              INTU_send_delimit
   INTU_send_close                 INTU_send_u_abort
   INTU_send_error                 INTU_send_reject
   INTU_send_message

   -----   ---------  -----     ------------------------------------
   Issue    Date       By        Changes
   -----   ---------  -----     ------------------------------------
     A     22-Dec-98   JET      - Initial code.
     -     20-Jul-06   JTD      - Updates to support INAPAPI DLL
     1     13-Dec-06   ML       - Change to use of Dialogic Corporation copyright.
     2     15-Mar-16   CJM      - Add option for INTU to use API Extended Dialog ID functions
                                - Tests for IN_LMSGS defined removed (no longer used by API)
*/

#include "intu_def.h"

/*
 * Module variables, stored in intu.c updated by command line options in
 * intu_main.c
 */
extern u8  intu_mod_id;      /* The task id of this module */
extern u8  inap_mod_id;      /* The task id of the INAP binary module*/
extern u16 intu_options;     /* Defines which tracing options are configured */

/*
 * INTU_send_open_rsp()
 *
 * Returns 0 on success
 *         INTUE_CODING_FAILURE
 *         INTUE_MSG_SEND_FAILED
 *         INTUE_MSG_ALLOC_FAILED
 */
int INTU_send_open_rsp(ic_dlg_id)
  u32    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
{
  HDR *dlg_h;            /* The dialogue message being sent */
  int status;            /* Status of IN API library functions */
  u8  temp_dlg_param;    /* Stores the dialogue result parameter */

  u8 qos;   /*dialogue QOS param*/
  
   qos = 3; // bit 0 = 1 and bit 1 = 1

  temp_dlg_param = INAPRS_DLG_ACC;

  if ((dlg_h = IN_alloc_message(0)) != 0)
  {

    status = IN_set_dialogue_param(INDP_qos, 1, &qos, dlg_h);

    status = IN_set_dialogue_param(INDP_result, 1, &temp_dlg_param, dlg_h);

    if (status == IN_SUCCESS)
    {
      if (intu_options & INTU_OPT_EXT_DLG)
        status = IN_EXT_dialogue_open_rsp(ic_dlg_id, dlg_h);
      else
        status = IN_dialogue_open_rsp((u16)ic_dlg_id, dlg_h);
    }

    if (status != IN_SUCCESS)
    {
      IN_free_message(dlg_h);
      return(INTUE_CODING_FAILURE);
    }

    if (INTU_send_message(intu_mod_id, inap_mod_id, dlg_h) != IN_SUCCESS)
    {
      IN_free_message(dlg_h);
      return(INTUE_MSG_SEND_FAILED);
    }
  }
  else
  {
    return(INTUE_MSG_ALLOC_FAILED);
  }

  return(0);
}

/*
 * INTU_send_delimit()
 *
 * Returns 0 on success
 *         INTUE_CODING_FAILURE
 *         INTUE_MSG_SEND_FAILED
 *         INTUE_MSG_ALLOC_FAILED
 */
int INTU_send_delimit(ic_dlg_id)
  u32    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
{
  HDR *dlg_h;            /* The dialogue message being sent */
  int status;            /* Status of IN API library functions */

  if ((dlg_h = IN_alloc_message(0)) != 0)
  {
    if (intu_options & INTU_OPT_EXT_DLG)
      status = IN_EXT_dialogue_delimit(ic_dlg_id, dlg_h);
    else
      status = IN_dialogue_delimit((u16)ic_dlg_id, dlg_h);

    if (status != IN_SUCCESS)
    {
      IN_free_message(dlg_h);
      return(INTUE_CODING_FAILURE);
    }

    if (INTU_send_message(intu_mod_id, inap_mod_id, dlg_h) != IN_SUCCESS)
    {
      IN_free_message(dlg_h);
      return(INTUE_MSG_SEND_FAILED);
    }
  }
  else
  {
    fprintf(stderr,"INTU: *** Failed allocate MSG for dialogue ID 0x%08lx ***\n", ic_dlg_id);
    return(INTUE_MSG_ALLOC_FAILED);
  }

  return(0);
}

/*
 * INTU_send_close()
 *
 * Returns 0 on success
 *         INTUE_CODING_FAILURE
 *         INTUE_MSG_SEND_FAILED
 *         INTUE_MSG_ALLOC_FAILED
 */
int INTU_send_close(ic_dlg_id, release)
  u32    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
  u8     release;        /* The release method to be used by INAP */
{
  HDR *dlg_h;            /* The dialogue message being sent */
  int status;            /* Status of IN API library functions */

  //  u8 dlg_timeout;
  //dlg_timeout = 60;
  //u8   qos = 3; // bit 0 = 1 and bit 1 = 1


  if ((dlg_h = IN_alloc_message(0)) != 0)
  {


      //    status = IN_set_dialogue_param(INDP_dlg_idle_timeout, 1, &dlg_timeout, dlg_h);
      //status = IN_set_dialogue_param(INDP_qos, 1, &qos, dlg_h);
      //printf("status = %d\n", status);

    status = IN_set_dialogue_param(INDP_release_method, 1, &release, dlg_h);

    if (status == IN_SUCCESS)
    {
      if (intu_options & INTU_OPT_EXT_DLG)
        status = IN_EXT_dialogue_close(ic_dlg_id, dlg_h);
      else
        status = IN_dialogue_close((u16)ic_dlg_id, dlg_h);
    }

    if (status != IN_SUCCESS)
    {
      IN_free_message(dlg_h);
      return(INTUE_CODING_FAILURE);
    }

    if (INTU_send_message(intu_mod_id, inap_mod_id, dlg_h) != IN_SUCCESS)
    {
      IN_free_message(dlg_h);
      return(INTUE_MSG_SEND_FAILED);
    }
  }
  else
  {
    fprintf(stderr,"INTU: *** Failed allocate MSG for dialogue ID 0x%08lx ***\n", ic_dlg_id);
    return(INTUE_MSG_ALLOC_FAILED);
  }

  return(0);
}


/*
 * INTU_send_u_abort()
 *
 * Returns 0 on success
 *         INTUE_CODING_FAILURE
 *         INTUE_MSG_SEND_FAILED
 *         INTUE_MSG_ALLOC_FAILED
 */
int INTU_send_u_abort(ic_dlg_id)
  u32    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
{
  HDR *dlg_h;            /* The dialogue message being sent */
  int status;            /* Status of IN API library functions */
  u8  temp_dlg_param;    /* Stores the dialogue user reason parameter */

  temp_dlg_param = INAPUR_user_specific;

  if ((dlg_h = IN_alloc_message(0)) != 0)
  {
    status = IN_set_dialogue_param(INDP_user_rsn, 1, &temp_dlg_param, dlg_h);

    if (status == IN_SUCCESS)
    {
      if (intu_options & INTU_OPT_EXT_DLG)
        status = IN_EXT_dialogue_u_abort(ic_dlg_id, dlg_h);
      else
        status = IN_dialogue_u_abort((u16)ic_dlg_id, dlg_h);
    }

    if (status != IN_SUCCESS)
    {
      IN_free_message(dlg_h);
      return(INTUE_CODING_FAILURE);
    }

    if (INTU_send_message(intu_mod_id, inap_mod_id, dlg_h) != IN_SUCCESS)
    {
      IN_free_message(dlg_h);
      return(INTUE_MSG_SEND_FAILED);
    }
  }
  else
  {
    fprintf(stderr,"INTU: *** Failed allocate MSG for dialogue ID 0x%08lx ***\n", ic_dlg_id);
    return(INTUE_MSG_ALLOC_FAILED);
  }

  return(0);
}

/*
 * INTU_send_reject()
 *
 * Returns 0 on success
 *         INTUE_CODING_FAILURE
 *         INTUE_MSG_SEND_FAILED
 *         INTUE_MSG_ALLOC_FAILED
 */
int INTU_send_reject(ic_dlg_id, dlg_ptr)
  u32    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
  DLG_CB *dlg_ptr;       /* Pointer to the per-dialogue id control block */
{
  HDR *reject_h;         /* The reject service request being sent */
  int status;            /* Status of IN API library functions */

  if ((reject_h = IN_alloc_message(0)) != 0)
  {
    if (intu_options & INTU_OPT_EXT_DLG)
      status = IN_EXT_code_reject(ic_dlg_id, &(dlg_ptr->cpt), reject_h);
    else
      status = IN_code_reject((u16)ic_dlg_id, &(dlg_ptr->cpt), reject_h);

    if (status != IN_SUCCESS)
    {
      IN_free_message(reject_h);
      return(INTUE_CODING_FAILURE);
    }

    if (INTU_send_message(intu_mod_id, inap_mod_id, reject_h) != IN_SUCCESS)
    {
      IN_free_message(reject_h);
      return(INTUE_MSG_SEND_FAILED);
    }
  }
  else
  {
    fprintf(stderr,"INTU: *** Failed allocate MSG for dialogue ID 0x%08lx ***\n", ic_dlg_id);
    return(INTUE_MSG_ALLOC_FAILED);
  }

  return(0);
}

/*
 * INTU_send_error()
 *
 * Returns 0 on success
 *         INTUE_CODING_FAILURE
 *         INTUE_MSG_SEND_FAILED
 *         INTUE_MSG_ALLOC_FAILED
 */
int INTU_send_error(ic_dlg_id, dlg_ptr)
  u32    ic_dlg_id;      /* Dialogue id of the incoming dialogue indication */
  DLG_CB *dlg_ptr;       /* Pointer to the per-dialogue id control block */
{
  HDR *error_h;          /* The error service request message being sent */
  int status;            /* Status of IN API library functions */

  if ((error_h = IN_alloc_message(0)) != 0)
  {
    if (intu_options & INTU_OPT_EXT_DLG)
      status = IN_EXT_code_error(ic_dlg_id, &(dlg_ptr->cpt), error_h);
    else
      status = IN_code_error((u16)ic_dlg_id, &(dlg_ptr->cpt), error_h);

    if (status != IN_SUCCESS)
    {
      IN_free_message(error_h);
      return(INTUE_CODING_FAILURE);
    }

    if (INTU_send_message(intu_mod_id, inap_mod_id, error_h) != IN_SUCCESS)
    {
      IN_free_message(error_h);
      return(INTUE_MSG_SEND_FAILED);
    }
  }
  else
  {
    fprintf(stderr,"INTU: *** Failed allocate MSG for dialogue ID 0x%08lx ***\n", ic_dlg_id);
    return(INTUE_MSG_ALLOC_FAILED);
  }

  return(0);
}


/*
 * INTU_send_message()
 *
 * Calls IN_send_message but displays the message being sent if tracing is on
 * for that type of message.
 */
int INTU_send_message(sending_user_id, inap_module_id, h)
  u8     sending_user_id;   /* Source task id -INTU */
  u8     inap_module_id;    /* Dest task id - INAP */
  HDR    *h;                /* Message to send */
{
  DLG_CB *dlg_ptr;          /* Pointer to the per-dialogue id control block */
  u32    ic_dlg_id;         /* Dialogue id of the incoming dialogue indication */

  dlg_ptr = INTU_get_dlg_cb(&ic_dlg_id, h);

  if ((h->type == INAP_MSG_SRV_REQ) && (intu_options & INTU_OPT_TR_SRV_REQ))
  {
    if (dlg_ptr != 0)
      INTU_disp_srv_req(ic_dlg_id, dlg_ptr, h);
  }
  else
  {
    if ((h->type == INAP_MSG_DLG_REQ) && (intu_options & INTU_OPT_TR_DLG_REQ))
    {
      if (dlg_ptr != 0)
        INTU_disp_dlg_msg(ic_dlg_id, h);
    }
  }

  return(IN_send_message(sending_user_id, inap_module_id, h));
}
