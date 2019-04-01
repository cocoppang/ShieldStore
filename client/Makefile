
all: load run

load: client_load.c
	gcc -O2 -g -o $@ $^ -lpthread

run: client_run.c
	gcc -O2 -g -o $@ $^ -lpthread

clean:
	rm -rf run load
