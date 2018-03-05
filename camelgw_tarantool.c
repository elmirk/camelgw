
#include <stdlib.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>
//#include <curl/curl.h>

//#include "system.h"
//#include "msg.h"
//#include "intu_def.h"

//#define MP_SOURCE 1
#include "msgpuck.h"



int bootstrap_tnt_idphandlers_space(char **services, char **idp_handlers)
{
    int i = 0;

	struct tnt_stream *tnt = tnt_net(NULL);          /* See note = SETUP */
	tnt_set(tnt, TNT_OPT_URI, "localhost:3311");
	if (tnt_connect(tnt) < 0) {                      /* See note = CONNECT */
	    printf("Connection refused\n");
	    exit(-1);
	}

	struct tnt_stream *tuple;// = tnt_object(NULL);     /* See note = MAKE REQUEST */

    while ( services[i] != NULL)
	{
	    tuple = tnt_object(NULL);
    tnt_object_format(tuple, "[%s%d]", services[i], idp_handlers[i]);
	tnt_insert(tnt, 515, tuple);                     /* See note = SEND REQUEST */
	tnt_flush(tnt);
	struct tnt_reply reply;  tnt_reply_init(&reply); /* See note = GET REPLY */
	tnt->read_reply(tnt, &reply);
	if (reply.code != 0) 
{
	    printf("Insert failed %lu.\n", reply.code);
	}
	    i++;
	}
	

	tnt_close(tnt);                                  /* See below = TEARDOWN */
	tnt_stream_free(tuple);
	tnt_stream_free(tnt);
	return 0;
}
void *camelgw_tnt_select(const char *str)
{

// int (*tptr) (u32 , DLG_CB *, HDR *, u8 *, IN_CPT *, struct calldetails *);
void *tptr;

	struct tnt_stream *tnt = tnt_net(NULL);              /* SETUP */
	tnt_set(tnt, TNT_OPT_URI, "localhost:3311");
	if (tnt_connect(tnt) < 0) {                         /* CONNECT */
	    printf("Connection refused\n");
	    exit(-1);
	}
	struct tnt_stream *arg; arg = tnt_object(NULL);     /* MAKE REQUEST */
	tnt_object_add_array(arg, 0);
	struct tnt_request *req1 = tnt_request_call(NULL);  /* CALL function f() */
	tnt_request_set_funcz(req1, "camel_main");

	struct tnt_stream *tuple = tnt_object(NULL);     /* REQUEST */
	tnt_object_format(tuple, "[%s]", str);


	tnt_request_set_tuple(req1, tuple);
	uint64_t sync1 = tnt_request_compile(tnt, req1);
	tnt_flush(tnt);                                     /* SEND REQUEST */
	struct tnt_reply reply;  
	tnt_reply_init(&reply);    /* GET REPLY */
	tnt->read_reply(tnt, &reply);
	if (reply.code != 0) {
	    printf("Funciton call failed %lu.\n", reply.code);
	    exit(-1);
	}

	uint32_t field_count = mp_decode_array(&reply.data);

	printf("field count = %d\n", field_count);


	char field_type;
	field_type = mp_typeof(*reply.data);

printf("field_type = %d\n", field_type);

 uint64_t num_value;

 if (field_type == MP_INT) {
    num_value = mp_decode_int(&reply.data);
    printf("    value=%p\n", num_value);
}

tptr = (void *) num_value;

return tptr;

}

int camelgw_tnt_call_function(const char *fname, const char *arg1)
{

	struct tnt_stream *tnt = tnt_net(NULL);              /* SETUP */
	tnt_set(tnt, TNT_OPT_URI, "localhost:3311");
	if (tnt_connect(tnt) < 0) {                         /* CONNECT */
	    printf("Connection refused\n");
	    exit(-1);
	}
	struct tnt_stream *arg; arg = tnt_object(NULL);     /* MAKE REQUEST */
	tnt_object_add_array(arg, 0);
	struct tnt_request *req1 = tnt_request_call(NULL);  /* CALL function f() */
	tnt_request_set_funcz(req1, fname);

	struct tnt_stream *tuple = tnt_object(NULL);     /* REQUEST */
	tnt_object_format(tuple, "[%s]", arg1);


	tnt_request_set_tuple(req1, tuple);
	uint64_t sync1 = tnt_request_compile(tnt, req1);
	tnt_flush(tnt);                                     /* SEND REQUEST */
	struct tnt_reply reply;  
	tnt_reply_init(&reply);    /* GET REPLY */
	tnt->read_reply(tnt, &reply);
	if (reply.code != 0) {
	    printf("Funciton call failed %lu.\n", reply.code);
	    exit(-1);
	}

	uint32_t field_count = mp_decode_array(&reply.data);

	printf("field count = %d\n", field_count);


	char field_type;
	field_type = mp_typeof(*reply.data);

printf("field_type = %d\n", field_type);

 uint64_t num_value;

 if (field_type == MP_INT)
{
    num_value = mp_decode_int(&reply.data);
    printf("    value=%p\n", num_value);
}

//tptr = (void *) num_value;

//return 0;

return 0;
}
