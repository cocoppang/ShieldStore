###### SET VALUE SIZE FOR EXPERIMENTS ######

VAL_SIZE ?= SMALL
ifeq ($(VAL_SIZE), SMALL)
MODE = -DSMALL
endif
ifeq ($(VAL_SIZE), MEDIUM)
MODE = -DMEDIUM
endif
ifeq ($(VAL_SIZE), LARGE)
MODE = -DLARGE
endif

############################################

all: client

client: mydb_client.c
	gcc -O2 $(MODE) -o mydb_client mydb_client.c -lpthread
clean:
	rm -rf mydb_client
