#
#                       Copyright (C) Dialogic Corporation 2001-2010.  All Rights Reserved.
#
#   File:               intu.mak
#
#   Makefile to build:  INTU example
#   for use with:       C compiler.
#   with libraries:     from the Development package
#   Output file:        intu (executable)
#
#   -------+---------+------+------------------------------------------
#   Issue     Date      By    Description
#   -------+---------+------+------------------------------------------
#     1     03-Apr-01   MH   - Initial makefile.
#     2     29-Mar-10   JLP  - Added comment

# Include common definitions:
include ../makdefs.mak

INC := /opt/DSI/INC/

TARGET = $(BINPATH)/intu

all :   $(TARGET)

clean:
	rm -rf *.o
	rm -rf $(TARGET)

# Ensure libin_api.so exists #
INTULIBS = $(LIBS) -L. -lin_api

#commerc version
#helper.o: helper.c
#	gcc -c helper.c -I$(OCCIPATH)sdk/include/ -I$(CURLPATH)include/curl/ -shared -fpic -I$(LUAPATH) -I$(INTU) -I$(INC) -I/usr/local/lib/erlang/lib/erl_interfa#ce-3.10/include

helper.o: helper.c
	gcc -c helper.c -I/usr/local/include/curl/ -I$(INC) -lcurl 


camelgw_backend.o: camelgw_backend.c
		   gcc -c camelgw_backend.c -I$(INC) -DLINT_ARGS -DIN_LMSGS -m32



OBJS = version.o localtime.o stringfields.o term.o buildinfo.o syslog.o intu.o intu_sys.o intu_trc.o intumain.o helper.o camelgw_handle_idp.o camelgw_handle_erb.o camelgw_handle_acr.o camelgw_utils.o camelgw_backend.o camelgw_drivers.o camelgw_conf.o camelgw_init.o camelgw_tarantool.o camelgw_parsers.o camelgw_dnc.o lock.o camelgw_strings.o camelgw_cli.o camelgw_logger.o

$(TARGET): $(OBJS)
	$(LINKER) -Wl,--dynamic-list=./exported.txt $(LFLAGS) -o $@ $(OBJS) $(INTULIBS) $(SYSLIBS) -lcurl -llua -lm -ldl -ledit -ltarantool -lmsgpuck  -lpthread

#-llua -lm -ldl  === this is for lua
