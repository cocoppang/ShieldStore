#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include "time.h"
#include "string.h"
#include "App.h"

void print_int(int d) {
	printf("%d\n", d);
}

void region_read(int size) {
	int j;
	int r = FOOTPRINT / (4*1024*1024);
	int temp[size];

	for(j = 0; j < FOOTPRINT - 20480*r; j = j + 20480*r) {
		memcpy(temp, page->addr+j, size);
		memcpy(temp, page->addr+j + 1024*r+2, size);
		memcpy(temp, page->addr+j + 2048*r+4, size);
		memcpy(temp, page->addr+j + 3072*r+8, size);
		memcpy(temp, page->addr+j + 4096*r+2, size);
		memcpy(temp, page->addr+j + 5120*r+4, size);
		memcpy(temp, page->addr+j + 6144*r+8, size);
		memcpy(temp, page->addr+j + 7168*r+8, size);
		memcpy(temp, page->addr+j + 8192*r+4, size);
		memcpy(temp, page->addr+j + 9216*r+2, size);
		memcpy(temp, page->addr+j + 10240*r+2, size);
		memcpy(temp, page->addr+j + 11264*r+4, size);
		memcpy(temp, page->addr+j + 12288*r+8, size);
		memcpy(temp, page->addr+j + 13312*r+4, size);
		memcpy(temp, page->addr+j + 14336*r+2, size);
		memcpy(temp, page->addr+j + 15360*r+4, size);
		memcpy(temp, page->addr+j + 16384*r+2, size);
		memcpy(temp, page->addr+j + 17408*r+4, size);
		memcpy(temp, page->addr+j + 18432*r+4, size);
		memcpy(temp, page->addr+j + 19456*r+2, size);
	}
}

void region_write(int size) {
	int i;
	int r = FOOTPRINT / (4*1024*1024);
	int value[size];
	memset(value, 1, size);

	for(i = 0; i < FOOTPRINT - 20480*r; i = i + 20480*r) {
		memcpy(page->addr+i, value, size);
		memcpy(page->addr+i+1024*r+4, value, size);
		memcpy(page->addr+i+2048*r+8, value, size);
		memcpy(page->addr+i+3072*r+2, value, size);
		memcpy(page->addr+i+4096*r+4, value, size);
		memcpy(page->addr+i+5120*r+8, value, size);
		memcpy(page->addr+i+6144*r+2, value, size);
		memcpy(page->addr+i+7168*r+4, value, size);
		memcpy(page->addr+i+8192*r+2, value, size);
		memcpy(page->addr+i+9216*r+4, value, size);
		memcpy(page->addr+i+10240*r+8, value, size);
		memcpy(page->addr+i+11264*r+2, value, size);
		memcpy(page->addr+i+12288*r+2, value, size);
		memcpy(page->addr+i+13312*r+4, value, size);
		memcpy(page->addr+i+14336*r+8, value, size);
		memcpy(page->addr+i+15360*r+2, value, size);
		memcpy(page->addr+i+16384*r+4, value, size);
		memcpy(page->addr+i+17408*r+2, value, size);
		memcpy(page->addr+i+18432*r+8, value, size);
		memcpy(page->addr+i+19456*r+4, value, size);
	}
}

int main(int argc, char **argv) {
	page= (struct region *) malloc(sizeof(struct region));
	if(!page) printf("Region is not created!\n");

	int t;

	struct timeval start, end;
	double diff = 0;

	// Touch
	for(t = 0; t < FOOTPRINT; t++) {
		page->addr[t] = 1;
	}

	gettimeofday(&start, NULL);
	region_write(16);
	gettimeofday(&end, NULL);

	free(page);

	diff = (double)(end.tv_usec - start.tv_usec) / 1000000 + (double)(end.tv_sec - start.tv_sec);

	FILE *f = fopen("result.txt", "a");
	fprintf(f, "%lf sec\n", diff);
	fclose(f);

	return 0;
}
