
#pragma once
/*header file for helper.c functions*/
/* CAMELGW additional functions */
//int sendMessage(LOG_MSG *message, int message_length);

extern u32 last_dlg_id;      /* Last Dialog ID of configured range */


int sendMessage(LOG_MSG *message, int message_length);
int getCurrentTime(char *time_buffer);
//this function should be in handle_acr.c file
//int CAMELGW_get_callresult_param(char *param_buffer /*IN*/, u16 param_buffer_length /*IN*/, char param_TAG /*IN*/, char *param_data);
void CAMELGW_push(DLG_CPT_NODE **head, u8 data1, u16 data2, u8 *data3, u8 data4);
int CAMELGW_pop(DLG_CPT_NODE **head, IN_CPT *data);
int CAMELGW_list_length(DLG_CPT_NODE **head);
void CAMELGW_del_list(DLG_CPT_NODE **head);

int INTU_translate_number(PTY_ADDR *old_num, PTY_ADDR *new_num, int regime);
u8 INTU_fmt_called_num(u8 *buf, u8 siz, PTY_ADDR *called_num, int eventype);
//u8     	*buf;         // pointer into message buffer 
//u8		siz;          // size of buffer 
//PTY_ADDR      *called_num;  // called party number to format 
//int eventype;

