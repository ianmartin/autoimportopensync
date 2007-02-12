GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0)
GLIB_LDFLAGS := $(shell pkg-config --libs glib-2.0)
OUT=client
INCLUDE=$(GLIB_CFLAGS)

XXFLAGS=-Wall -g $(INCLUDE) -L/usr/lib 
LDFLAGS=$(GLIB_LDFLAGS)
CXX=gcc
OBJ=gpesync_client.o

all: test_client.c $(OBJ)
	$(CXX) $(OBJ) test_client.c -o $(OUT) $(XXFLAGS) $(LDFLAGS) 

gpesync_client.o: ../gpesync_client.c ../gpesync_client.h
	$(CXX) ../gpesync_client.c -c $(XXFLAGS)

clean:
	rm *.o
	rm $(OUT)
