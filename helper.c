#include "intu_def.h"
#include "time.h"
#include "sys/ipc.h"
#include "helper.h"
#include "globals.h"


//#include <lauxlib.h>
#include <string.h>
//#include <lualib.h>
#include <stdio.h>
//#include <time.h>
#include <curl.h>
//#include <oci.h>
//#include <lua.h>
//#include <intu_def.h>

#include "camelgw_conf.h"


#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

//size_t my_dummy_write(char *ptr, size_t size, size_t nmemb, void *userdata);

/*******************************************************************************
*                                                                             *
* Helper procedures                                                           *
*                                                                             *
*******************************************************************************/

/*
* INTU_get_dlg_cb()
*
* Return a ptr to the dialogue control block if the dialogue id was in the
* valid range. Returns 0 otherwise.
*
* фукнкция возвращает указатель на  данные типа DLG_CB 
*/
//DLG_CB *INTU_get_dlg_cb(dlg_id)
//u16 dlg_id;         /* Dialogue id to check */
//{
//	if ((dlg_id < base_dialogue_id)|| (dlg_id > base_dialogue_id + num_dialogues - 1))
//	{
//		fprintf(stderr,"INTU: *** Dialogue ID 0x%04x out of valid range [0x%04x to 0x%04x] ***\n",dlg_id, base_dialogue_id, base_dialogue_id + num_dialogues -1);
//		return 0;
//	}
//	else
//	{
//		return(&dlg_data[dlg_id - base_dialogue_id]);
//	}
//}

/*
 * INTU_get_dlg_cb()
 *
 * Returns recovered dialog id and pointer to the dialogue control block,
 * if the dialogue id was in the valid range.
 * Returns 0 otherwise.
 */
DLG_CB *INTU_get_dlg_cb(u32 *dlg_id, HDR *h)
//u32  *dlg_id;         /* Pointer to recovered Dialogue id */
//HDR  *h;              /* Received message */
{
    int  decode_status;   /* Return status of the dialogue type recovery */
    u16  did = 0;         /* Dialog ID used when not using Extended DIDs */

    /*
     * Find the dialogue ID from the received message
     */
    if (intu_options & INTU_OPT_EXT_DLG)
	decode_status = IN_EXT_get_dialogue_id(dlg_id, h);
    else
	{
	    decode_status = IN_get_dialogue_id(&did, h);
	    *dlg_id = (u32)did;
	}

    if (decode_status != IN_SUCCESS)
	{
	    fprintf(stderr,
		    "INTU: *** Error recovering Dialogue ID from received message ***\n");

	    INTU_disp_other_msg(h);
	    return(NULL);
	}

    /*
     * Check that the dialogue ID is in a valid range
     */
    if ((*dlg_id < base_dialogue_id) || (*dlg_id > last_dlg_id))
	{
	    fprintf(stderr,
		    "INTU: *** Dialogue ID 0x%08lx out of valid range [0x%08lx to 0x%08lx] ***\n",
		    *dlg_id, base_dialogue_id, last_dlg_id);

	    return(NULL);
	}

    /*
     * Return the dialogue record for this dialogue ID
     */
    return (&dlg_data[*dlg_id - base_dialogue_id]);
}

/*
* IN_get_protocol_definition()
*
* Returns a pointer to the protocol defintion to use for the given application
* context. Returns 0 if the lookup failed.
*/
void *INTU_get_protocol_definition(applic_context_index)
u16 applic_context_index; /* Applic Context Index of protocol to be used */
{
	switch (applic_context_index)
	{
	case INETS_AC_CS1_SSP_TO_SCP:
	case INETS_AC_CS1_ASSIST_HANDOFF_TO_SSP_TO_SCP:
	case INETS_AC_CS1_IP_TO_SCP:
	case INETS_AC_CS1_SCP_TO_SSP:
	case INETS_AC_CS1_SCP_TO_SSP_TRAFFIC_MANAGEMENT:
	case INETS_AC_CS1_SCP_TO_SSP_SERVICE_MANAGEMENT:
	case INETS_AC_CS1_SSP_TO_SCP_SERVICE_MANAGEMENT:
		return(IN_get_prot_spec(INETS_300_374_1_PROTOCOL));

	case INITU_CS1_SSF_TO_SCF_GENERIC_AC:
	case INITU_CS1_SSF_TO_SCF_DPSPECIFIC_AC:
	case INITU_CS1_ASSIST_HANDOFF_SSF_TO_SCF_AC:
	case INITU_CS1_SRF_TO_SCF_AC:
	case INITU_CS1_SCF_TO_SSF_AC:
	case INITU_CS1_DP_SPECIFIC_SCF_TO_SSF_AC:
	case INITU_CS1_SCF_TO_SSF_TRAFFIC_MANAGEMENT_AC:
	case INITU_CS1_SCF_TO_SSF_SERVICE_MANAGEMENT_AC:
	case INITU_CS1_SSF_TO_SCF_SERVICE_MANAGEMENT_AC:
	case INITU_CS1_SCF_TO_SSF_STATUS_REPORTING_AC:
		return(IN_get_prot_spec(INITU_Q1218_PROTOCOL));

	case INCAP_V1_GSMSSF_TO_GSMSCF:
		return(IN_get_prot_spec(INCAP_V1_PROTOCOL));

	case INCAP_V2_GSMSSF_TO_GSMSCF:
	case INCAP_V2_ASSIST_HANDOFF_GSMSSF_TO_GSMSCF:
	case INCAP_V2_GSMSRF_TO_GSMSCF:
		return(IN_get_prot_spec(INCAP_V2_PROTOCOL));

	case INETS_AC_CS2_SSF_SCF_GENERIC:
	case INETS_AC_CS2_SSF_SCF_ASSIST_HANDOFF:
	case INETS_AC_CS2_SSF_SCF_SERVICE_MANAGEMENT:
	case INETS_AC_CS2_SCF_SSF_GENERIC:
	case INETS_AC_CS2_SCF_SSF_TRAFFIC_MANAGEMENT:
	case INETS_AC_CS2_SCF_SSF_SERVICE_MANAGEMENT:
	case INETS_AC_CS2_SCF_SSF_TRIGGER_MANAGEMENT:
	case INETS_AC_CS2_SRF_SCF:
	case INETS_AC_CS2_SCF_SCF_OPERATIONS:
	case INETS_AC_CS2_DISTRIBUTED_SCF_SYSTEM:
	case INETS_AC_CS2_SCF_SCF_OPERATIONS_WITH_3SE:
	case INETS_AC_CS2_DISTRIBUTED_SCF_SYSTEM_WITH_3SE:
	case INETS_AC_CS2_SCF_CUSF:
	case INETS_AC_CS2_CUSF_SCF:
		return(IN_get_prot_spec(INEN_301_140_1_PROTOCOL));

	case INCAP_V3_GSMSSF_TO_GSMSCF_GENERIC:
	case INCAP_V3_ASSIST_HANDOFF_GSMSSF_TO_GSMSCF:
	case INCAP_V3_GSMSRF_TO_GSMSCF:
	case INCAP_V3_GPRSSSF_TO_GSMSCF:
	case INCAP_V3_GSMSCF_TO_GPRSSSF:
	case INCAP_V3_SMS:
		return(IN_get_prot_spec(INCAP_V3_PROTOCOL));

	case INCAP_V4_GSMSSF_TO_GSMSCF_GENERIC:
	case INCAP_V4_ASSIST_HANDOFF_GSMSSF_TO_GSMSCF:
	case INCAP_V4_SCF_TO_GSMSSF_GENERIC:
	case INCAP_V4_GSMSRF_TO_GSMSCF:
	case INCAP_V4_GPRSSSF_TO_GSMSCF:
	case INCAP_V4_GSMSCF_TO_GPRSSSF:
	case INCAP_V4_SMS:
		return(IN_get_prot_spec(INCAP_V4_PROTOCOL));

	case INCAP_V4_IM_SSF_TO_GSMSCF_GENERIC:
		return(IN_get_prot_spec(INCAP_IMS_PROTOCOL));

	default:
		return(0);
	}
	return(0);
}

/*
* INTU_translate_number()
*
* Returns 0 on successful number translation
*         INTUE_NUM_TRANSLATE_FAILED otherwise
*/
int INTU_translate_number(PTY_ADDR *old_num, PTY_ADDR *new_num, int regime)
//PTY_ADDR *old_num;	// Old called party number 
//PTY_ADDR *new_num;		// New destination routing number = новые преобразованные цифры
//int regime;
{
	u8  called_pty_dgt_str[INTU_NUMBER_OF_DIGITS];// Called party digits 
	u8  num_digits; // number of called pty digits
	int i;          // loop counter 

	num_digits = old_num->naddr;

	/*
	* Copy the whole orignal called party address onto the destination 
	* routing address before looking at the actual digits.
	*/
	memcpy((void *) new_num, (void *) old_num, sizeof(PTY_ADDR));
	if ((num_digits > 0)&&(regime==3))
	{
		unpack_digits(called_pty_dgt_str, old_num->addr, 0, num_digits);
		if (intu_options & INTU_OPT_TR_NUM_TRANS)
		{
			printf("INTU: Called Party [\n");
			for (i=0;i<num_digits;i++)
				printf("%x\n",called_pty_dgt_str[i]);
		}

		/*
		* Does the recovered called party number 
		* match our example freephone number
		*/
		if ((memcmp(called_pty_dgt_str,
			example_freephone_num,num_digits) == 0) &&
			(num_digits == sizeof(example_freephone_num)))
		{
			num_digits = sizeof(example_dest_routing_num);
			pack_digits(new_num->addr, 0, example_dest_routing_num, num_digits);
			new_num->naddr = num_digits;

			if (intu_options & INTU_OPT_TR_NUM_TRANS)
			{
				printf("], Dest Routing Addr [");
				for (i=0;i<num_digits;i++)
					printf("%1.1x",example_dest_routing_num[i]);
				printf("]\n");
			}
			return(0);
		}
		else
		{
			if (intu_options & INTU_OPT_TR_NUM_TRANS)
				printf("], Releasing Call\n");
			return(INTUE_NUM_TRANSLATE_FAILED);
		}
	}

	if ((num_digits > 0)&&(regime!=3)){
		unpack_digits(called_pty_dgt_str, old_num->addr, 0, num_digits);

		if (intu_options & INTU_OPT_TR_NUM_TRANS)
		{ 
			printf("INTU: Called Party [\n");
			for (i=0;i<num_digits;i++)
				printf("%1.1x\n",called_pty_dgt_str[i]);
		} 
	} 

	else
	{
		return(INTUE_NUM_TRANSLATE_FAILED);
	}
}

/*
* INTU_fmt_called_num()
* pack bits from structure to array
* array used then in INAP API to construct generic 
* number buffer like array of bytes
* Returns formated length, 0 on failure
*/
u8 INTU_fmt_called_num(buf, siz, called_num,eventype)
u8     	*buf;         // pointer into message buffer 
u8		siz;          // size of buffer 
PTY_ADDR      *called_num;  // called party number to format 
int eventype;
{
	u8		oddn, adlen, *addr, iCt;
	u8 elvina_1[]={29,6,144};
	/*
	* Verify that there is sufficient buffer space
	* for fixed fields and 'naddr' addres digits.
	*/
	adlen = (called_num->naddr + 1) >> 1;
	if (siz < (adlen + 2))
		return(0);
	else
	{
		int i;
		/*
		* Pack fixed fields
		*/
		addr = called_num->addr;
		for (i=0;i<=6;i++){
			printf("addr[%i] %x\n", i, addr[i]);
		}

		if(eventype == 12){
			adlen = (called_num->naddr + 2) >> 1;
			oddn = bit_from_byte(called_num->naddr, 1); // o/e
			bit_to_byte(buf, oddn, 7);    
			bits_to_byte(buf++, 3, 0, 7); // noai
			bits_to_byte(buf++, 4, 0, 7); // npi

			for (iCt=0; iCt<3; iCt++)
				*buf++=elvina_1[iCt];

			for (iCt=2; iCt<7; iCt++)
				*buf++=addr[iCt];
		}

		if(eventype == 2){
			/* 
			* если младший бит naddr равен 0 то количество цифр четное
			*/
			oddn = bit_from_byte(called_num->naddr, 0);  
			/* 
			* в седьмой бит по указателю buf устанавливаем з
			* начение младшего бита длины номера
			*/
			bit_to_byte(buf, oddn, 7);  
			bits_to_byte(buf++, 4, 0, 7);
			bits_to_byte(buf, 0, 0, 4);
			bits_to_byte(buf, called_num->npi, 4, 3);
			bit_to_byte(buf, 0, 7);
			for(iCt = adlen; iCt > 0; iCt--)
				*++buf = *addr++;
		}

		/*
		* Add filler if naddr odd
		*/
		if (oddn)
			*buf &= 0x0f;
	}
	return(2 + adlen);
}
/*
* INTU_rec_called_num()
* function used to fill called_num strucutre from received 
* CalledPartyBCDnum digits from incoming IDP
* CalledPartyBCD num transferred to this functioan as array with pointer = pptr
* 
* Returns 0 on success
*         INTUE_NUM_RECOVERY_FAILED otherwise
*/
int INTU_rec_called_num(called_num, pptr, plen)
PTY_ADDR      *called_num;  	// called party number recovered into here, goes as result of function 	
u8            *pptr;        	// pointer into parameter buffer, goes as input parameter for function 
PLEN           plen;         	// length of parameter being recovered , goes as input parameter for function
{
	u8        oddn, *addr;
	if ((plen < INTU_min_called_party_num_len) 
		||(plen > INTU_max_called_party_num_len))
	{
		/*
		* Parameter length is outside supported range.
		*/
		return(INTUE_NUM_RECOVERY_FAILED);
	}
	/*
	* recover fixed length fields
	*/
	plen -= 1; // старший октет нам не нужен, там хранится служебная инфа - extension,ton and npi 
	oddn = bits_from_byte(*(pptr+plen),4,4); // нужны четыре последних бита последнего октета CalledBCD для проверка на четность/нечетность цифр 
	if (oddn == 0xf) // if last 4 bits = 1111 that means odd number of digits in BCD 
	{
		fprintf(stderr,ANSI_COLOR_YELLOW "oddn(dec) %i" ANSI_COLOR_RESET "\n",oddn);
		called_num->naddr = (u8)(plen << 1) - 1; //количество цифр в номере, сдвиг влево на 1 позицию = умножение на два и минус 1
	}

	else
	{  
		called_num->naddr = (u8)(plen << 1); //количество цифр в номере, сдвиг влево на 1 позицию = умножение на два
	}

	/* 
	* CALLED PARTY BCD
	* noai - type of number - bits from bit 4 to bit 7 in first octet
	* npi - numbering plan identification - bits from bit 0 to bit 4 in firts octet
	* *pptr - pointer to the first element of the array of bytes
	*/
	called_num->noai = bits_from_byte(*pptr, 4, 3); // type of number, start bit = 4, num of bits =3, bits goes from 0 
	called_num->npi = bits_from_byte(*pptr, 0, 4);
	fprintf(stderr,ANSI_COLOR_YELLOW "type num(hex) %x" 
		ANSI_COLOR_RESET "\n",called_num->noai);
	fprintf(stderr,ANSI_COLOR_YELLOW "numb plan indentification(hex) %x" 
		ANSI_COLOR_RESET "\n",called_num->npi);
	/*
	* Now recover (variable length) address digits
	*/
	addr = called_num->addr;
	while(plen--)
		*addr++ = *++pptr;

	/*
	* Force filler to zero (if naddr odd)
	*/
	fprintf(stderr,ANSI_COLOR_YELLOW "naddr(dec) %i" 
		ANSI_COLOR_RESET ,called_num->addr[0]);
	fprintf(stderr,ANSI_COLOR_YELLOW "naddr(dec) %i" 
		ANSI_COLOR_RESET "\n",called_num->addr[1]);
	return 0;
}

/* send message to sip_connect_logger module */
int sendMessage(LOG_MSG *message, int message_length)
{
	key_t ipckey;
	int mq_id;
	//char *str = "connect";
	//       struct { long type;
	//               char text[100] ; } mymsg;
	ipckey = ftok("/opt/DSI", 69);
	mq_id = msgget(ipckey, IPC_CREAT | 0666);
	// mymsg.type = 1;
	//strcpy(mymsg.text, buffer);
	//length = sizeof(message) - sizeof(long);
	msgsnd(mq_id, message, message_length, IPC_NOWAIT);
	return 0;
	}
int getCurrentTime(char *time_buffer) {
	struct tm m_time;
	long int s_time;
	s_time = time(NULL);
	localtime_r(&s_time, &m_time);
	asctime_r(&m_time, time_buffer);
	return 0;
}
/* TODO - this function should be redone ! */
/* param_data[0] - length */
int CAMELGW_get_callresult_param(char *param_buffer /*IN*/, u16 param_buffer_length /*IN*/, char param_TAG /*IN*/, char *param_data) {
    //int i,j=1, flag ;
    int i,j;
 int size, cursor;
 char param_length;
 // i=0;
 // while (i<=param_buffer_length,*(param_buffer +i) != param_TAG) {
 //i++;
 //}
 //flag=i;
 //*mess_size = *(param_buffer+flag+1);
 
 //param_length=*(param_buffer+i+1);
 //cursor=i+2;
 //param_data[0]=*mess_size;
 //for (i=cursor; i<(cursor+param_length); i++) {
 //param_data[j]=param_buffer[i];
 //j++;
 //}
 for (i=0;i<param_buffer_length;i++) {
     if( *(param_buffer +i ) == param_TAG)
	 {
	     *param_data =  *(param_buffer + i + 1);
	     for (j=0; j < *param_data; j++ )
		 {
		     *(param_data+1+j) = *(param_buffer + i + 1 + 1 +j);
		 }
	     return 0;
	 }

 }
 return 1;
}

/* insert node into list, in head position */

void CAMELGW_push(DLG_CPT_NODE **head, u8 data1, u16 data2, u8 *data3, u8 data4) {
    DLG_CPT_NODE *tmp = (DLG_CPT_NODE *) malloc(sizeof(DLG_CPT_NODE));
    tmp->operation = data1;
    tmp->databuf_offset =  data2;
    int i;
 for(i=0;i<data2; i++) {
     tmp->databuf[i] = *(data3+data4+i);
     //   printf ("databuf pushing = %x\n", tmp->databuf[i]);  //uncomment for debug
 }

    tmp->next = (*head);
    (*head) = tmp;
}
 
/* pop data from node into buffer cpt structure */


int CAMELGW_pop(DLG_CPT_NODE **head, IN_CPT *data) {
    DLG_CPT_NODE* prev = NULL;
     int i;

    if (head == NULL) {
        exit(-1);
    }
    prev = (*head);
   data->operation = prev->operation;
   data->prot_spec = prev->prot_spec;
   data->databuf_offset = prev->databuf_offset;
   //printf("pointer in pop  ===== %p\n", data);       
   //printf("databuf offset in pop function 1  ===== %i\n", data->databuf_offset);       
    for(i=0;i<(data->databuf_offset); i++) {
     data->databuf[i] = prev->databuf[i];
     }
    (*head) = (*head)->next;
    free(prev);
    // printf("databuf offset in pop function 2  ===== %i\n", data->databuf_offset);    
        return 0;
}

/* calculate number of nodes in component list */
int CAMELGW_list_length(DLG_CPT_NODE **head) {
    DLG_CPT_NODE *current = (*head);
    int count = 0;
    while (current != NULL)
	{
	    count++;
	    current = current->next;
	}
    return count;
}
void CAMELGW_del_list(DLG_CPT_NODE **head) {
    //DLG_CPT_NODE *tmp = (DLG_CPT_NODE *) malloc(sizeof(DLG_CPT_NODE));
    //tmp->operation = data1;
    //tmp->cpt_data_length = data2;
    //tmp->next = (*head);
    free(*head);
}

/*
*******************************************************************************
*                                                                             *
*  script_get_int: встраивает конфигурационный файл .lua в другие программы   *
*                                                                             *
*******************************************************************************
*/ // move into camelgw_conf.c and camelgw_conf.h -- for read configuration from lua config file
/*
* Содержимое файла обрабатывается в виде порции и компилируется 
* Далее помещая порцию в виртуальный стек
* Функция возвращает из конфигурационного файла значение таблицы int
*/ 
//read mode from config file - which service bypassed
//int script_get_int (int param)
//int read_lua_config(int param_type)
//{

//	lua_State	*L;

	//	char	ip;

//	int	status, result;
//	int i;
//	int	res, key;
//	int	type;

//	L = luaL_newstate(); //Создаем стэк
//	luaL_openlibs(L); //подключить стандартные библиотеки  
//	status = luaL_loadfile(L, "/opt/DSI/UPD/SRC/INTU/config.lua");

//	if (status)
//	    {
//		(void)fprintf(stderr, "file not found\n");
//	lua_close(L);//удаляем стэк и интерпретатор
//		return 1;
//	}

	//lua_newtable(L); //создать таблицу, поместить ее на вершину стэка
	//lua_setglobal(L, "foo");//добавить глобальную переменную "foo" в скрипт 
//	res = lua_pcall(L, 0, LUA_MULTRET, 0);//Выполняет функцию, функция должна находится
//	if (res) {
//		fprintf(stdout, "runtime error\n");
//		return 1;
//	}

//	result = lua_tonumber(L, -1); //only one return from lua config on the top of the stack
//
//	lua_pop(L, 1);//удаляем число которое вернул скрипт из стэка 
//	lua_close(L);//удаляем стэк и интерпретатор
//
//	return result;
//}

/*
*******************************************************************************
*                                                                             *
*     script_get_string: возвращает из .lua файла значение таблицы string     *
*                                                                             *
*******************************************************************************
*/
//moved to camelgw_conf.c
//void script_get_string (char string[], int param) {

//	lua_State	*L;

//	int status;
//	int i;
//	int	res, key;
//	int	type;

//	const	char	*str;

//	L = luaL_newstate(); //Создаем стэк
//	luaL_openlibs(L); //подключить стандартные библиотеки  
//	status = luaL_loadfile(L, "/opt/DSI/UPD/SRC/INTU/my.lua");

//	lua_newtable(L); //создать таблицу, поместить ее на вершину стэка
//	lua_setglobal(L, "foo");//добавить глобальню переменную "foo" в скрипт 
	
//	res = lua_pcall(L, 0, LUA_MULTRET, 0);//Выполняет функцию, функция должна находится

//	str = lua_tostring(L, param);
//	sprintf(string,"%s",str);
//	lua_pop(L, 1);//удаляем число которое вернул скрипт из стэка 
//	lua_close(L);//удаляем стэк и интерпретатор
//}


extern struct config camelgw_conf;

extern CURL *curl;// = curl_easy_init();
/*
*******************************************************************************
*                                                                             *
*       get: отправляет GET - запрос                                          *
*                                                                             *
*******************************************************************************
*/
/*
* Данные серевера прописаны в конфигурационном файле .lua
* Возращает http статус(200, 400, 412, 512)
*/
int get(char pCALLED_PARTY_NUMBER_B[], char pCALLING_PARTY_NUMBER_A[]) {

    //	CURL	*curl;
	CURLcode	res;

	int http_code;

	char	url[100] = {0}; 
	char	ip[15];

	int status, result;
	int	i;

	//	script_get_string(ip, IP);
	//	printf("host = %s\n", &camelgw_conf.letaika_ip[0]);
	sprintf(url, "%s/check?msisdn=%s&anum=%s", &camelgw_conf.letaika_ip[0], pCALLED_PARTY_NUMBER_B, pCALLING_PARTY_NUMBER_A);
	printf("get - %s\n", url);
	//curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	//	curl_easy_setopt(curl, CURLOPT_HEADER, 1);
	//curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2); /*timeout = 4 seconds is too much!*/
	//curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &my_dummy_write); /*we need this callback to switch of printing to STDOUT */
	res = curl_easy_perform(curl);
	curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	if(res != CURLE_OK)
		printf("ERROR GET: %s\n",curl_easy_strerror(res));
	//curl_easy_cleanup(curl);

	return http_code;
}
/*
*******************************************************************************
*                                                                             *
*     script_get_string: возвращает из .lua файла значение таблицы string     *
*                                                                             *
*******************************************************************************
*/
/*void script_get_string (char string[], int param) {

	lua_State	*L;

	int status;
	int i;
	int	res, key;
	int	type;

	const	char	*str;

	L = luaL_newstate(); //Создаем стэк
	luaL_openlibs(L); //подключить стандартные библиотеки  
	status = luaL_loadfile(L, "/opt/DSI/UPD/SRC/INTU/my.lua");

	lua_newtable(L); //создать таблицу, поместить ее на вершину стэка
	lua_setglobal(L, "foo");//добавить глобальню переменную "foo" в скрипт 
	res = lua_pcall(L, 0, LUA_MULTRET, 0);//Выполняет функцию, функция должна находится

	str = lua_tostring(L, param);
	sprintf(string,"%s",str);
	lua_pop(L, 1);//удаляем число которое вернул скрипт из стэка 
	lua_close(L);//удаляем стэк и интерпретатор
	} */

/* Never writes anything, just returns the size presented */
//size_t my_dummy_write(char *ptr, size_t size, size_t nmemb, void *userdata)
//{
//  return size * nmemb;
//}



//int CAMELGW_rec_called_num_test(struct called_pty_addr *p_called_num, unsigned char *pptr, unsigned short plen, unsigned char num_type)
//     PTY_ADDR  *called_num;  /* called party number recovered into here */
//     u8        *pptr;        /* pointer into parameter buffer */
//     PLEN       plen;         /* length of parameter being recovered */
// num_type = 2 for recovering called party bcd number and num_type = 12 for recovering called party number
//{
//    unsigned char         *addr;

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
//    p_called_num->num_type = num_type;

//    if ( num_type  == 2 )
//	{
	    //     plen -= 1; //one  octets in called bcd num digit string is service octets


	    //    p_called_num->bcd_exti = bit_from_byte(*pptr, 7);
	    //    p_called_num->bcd_toni = bits_from_byte(*pptr, 4, 3);
	    //    p_called_num->both_npi =  bits_from_byte(*pptr++, 0, 4);

	//    p_calling_num->ni = bit_from_byte(*pptr, 7);
	//p_calling_num->npi = bits_from_byte(*pptr, 4, 3);
	//p_calling_num->apri = bits_from_byte(*pptr, 2, 2);
	//p_calling_num->si = bits_from_byte(*pptr++, 0, 2);


	    //    p_called_num->naddr = (unsigned char)(plen << 1); /*equal to multiple to 2 operation */
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

	    //    unpack_digits(p_called_num->addr, pptr, 0, p_called_num->naddr);

	    //    if (p_called_num->addr[ p_called_num->naddr - 1] == 0x0f)
	    //	p_called_num->naddr = p_called_num->naddr -1;
	    //	}

	    //    if ( num_type == 12 )
	    //	{

	    //     plen -= 2; //two octets in calling num digit string is service octets
     //	unsigned char    both_npi;            /* Numbering plan indicator 0001b ISDN E164 */
	    //    p_called_num->oddi = bit_from_byte(*pptr, 7);; //odd-even indicator in calledpartynumber
	    //    p_called_num->noai = bits_from_byte(*pptr++, 0, 7);
	    //    p_called_num->inni = bit_from_byte(*pptr, 7);; //odd-even indicator in calledpartynumber;
    //   unsigned char    naddr;          /* number of address digits , this is our element not exist in standart calledBCDnumber*/
	    // p_called_num->both_npi =  bits_from_byte(*pptr++, 4, 3);


	    //    p_called_num->naddr = (unsigned char)(plen << 1) - p_called_num->oddi;
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

	    //    unpack_digits(p_called_num->addr, pptr, 0, p_called_num->naddr);

	    //	}

	    //    return 0;
	    //}

 /*
 * CAMELGW_rec_calling_num()
 *
 * Returns 0 on success
 *         INTUE_NUM_RECOVERY_FAILED otherwise
 */
//int CAMELGW_rec_calling_num(struct calling_pty_addr *p_calling_num, u8 *pptr, u16 plen) {
//     struct calling_pty_addr  *p_calling_num;  /* called party number recovered into here */
//     u8        *pptr;        /* pointer into parameter buffer */
//     u16       plen;         /* length of parameter being recovered, number of octest in parameter buffer */

//    unsigned char         *addr;

//        if ((plen < INTU_min_calling_party_num_len) ||
//    	(plen > INTU_max_calling_party_num_len))
//    	{
    	    /*
    	     * Parameter length is outside supported range.
    	     */
//    	    return(INTUE_NUM_RECOVERY_FAILED);
//    	}

//      plen -= 2; //two octets in calling num digit string is service octets

//    p_calling_num->oddi = bit_from_byte(*pptr, 7);
//    p_calling_num->noai = bits_from_byte(*pptr++, 0, 7);
    
//    p_calling_num->ni = bit_from_byte(*pptr, 7);
//    p_calling_num->npi = bits_from_byte(*pptr, 4, 3);
//    p_calling_num->apri = bits_from_byte(*pptr, 2, 2);
//    p_calling_num->si = bits_from_byte(*pptr++, 0, 2);


//    p_calling_num->naddr = (unsigned char)(plen << 1) - p_calling_num->oddi;
  
//    unpack_digits(p_calling_num->addr, pptr, 0, p_calling_num->naddr);

//    return 0;
//}

/* ssize_t                         /\* send "n" bytes to a socket descriptor. *\/ */
/* sendn_test(int fd, const void *vptr, size_t n) { */
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
/* 		printf("nsent = %d\n", nsent); */
/* 		perror("send:"); */

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


/***************************************************************************************************************
arrtonum function:

convert array of int element into unsigned long long value
test in =   unsigned char test_data[]= {2,5,0,2,7,1,2,3,4,5,6,7,8,9,0};
test out = unsigned long long value = 250271234567890
lentgh - number of digitst converted to value
in case of IMSI length = 15

*****************************************************************************************************************/
/* unsigned long long arrtonum(unsigned char *data, int length) { */

/*     int i; */
/* 	    const unsigned long long PowersOf10[] = { // use lookup to reduce multiplications */
/* 		1, 10, */
/* 		100, 1000, */
/* 		10000, 100000, */
/* 		1000000, 10000000, */
/* 		100000000, 1000000000, // actually the table can end here since A[i] is of type int */
/* 		10000000000, 100000000000, */
/* 		1000000000000, 10000000000000, */
/* 		100000000000000 }; */

/* 	    unsigned long long *p10, n = 0; // or unsigned long long if uint64_t not available */
/* 	    int N = length -1; */
/* 	    //  for (i = 0; i < sizeof(A)/sizeof(A[0]); i++) */
/*   for (i = 0; i < length; i++) */
/* 		{ */
/* 		    //  p10 = PowersOf10; */
/* 		    // while (*p10 < A[i]) p10++; */
/* 		    //n *= *p10; */
/* 		    //n += A[i]; */
/* 		    n = n + (*(data+i)*PowersOf10[N--]); */
/* 		} */

/* 	return n; */
/* } */

/***************************************************************************************************************
reversearray function:

mirror-like reverse elements in unsigned char array 
test in =    unsigned char data[] = {0xab, 0xcd, 0xef, 0x08, 0xcf, 0xea, 0, 0}; and plen = 6
test out = unsigned char data[] = {0xea, 0xcf, 0x08, 0xef, 0xcd, 0xab, 0, 0}
plen - number of element which should be reversed

*****************************************************************************************************************/
 
//int reversearray(unsigned char *data, int plen) {

// unsigned char tmp;
// int i = 0;
 //plen--;
// unsigned char nIters = plen >> 1;
//plen--;
// while ( i < nIters)
//   {
//	 tmp = data[i];
//  data[i++] = data[plen];
//data[plen--] = tmp;
 //++i;
    //plen--;
//}



//return 0;
//}

