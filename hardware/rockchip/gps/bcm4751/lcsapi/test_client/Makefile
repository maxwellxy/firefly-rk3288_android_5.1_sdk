INCFILES=-I../include
LDFLAGS=-lpthread
CFLAGS=-g -Wall ${INCFILES}

common_OBJS = $(addprefix ../src/, brcm_marshall.o gps_api.o lbs.o brcmipc_unixsocket.o supl_api.o)
test_client_OBJS = test_client.o $(common_OBJS) 
test_reconnect_OBJS = test_reconnect.o $(common_OBJS)

all: test_client  test_reconnect

test_client: $(test_client_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ 


test_reconnect: $(test_reconnect_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ 


clean:
	rm -f $(test_client_OBJS)
	rm -f test_client 
	rm -f $(test_reconnect_OBJS)
	rm -f test_reconnect 
