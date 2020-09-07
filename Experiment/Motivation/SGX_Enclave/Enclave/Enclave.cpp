#include <stdio.h>
#include <stdlib.h>
#include "string.h"

#include "sgx_trts.h"
#include "Enclave_t.h"
#include "Enclave.h"

void region_create() {
	page= (struct region *) malloc(sizeof(struct region));
	if(!page) print("Region is not created!\n");
}

void region_touch() {
	int i;
	for (i = 0; i < FOOTPRINT; i++) {
		page->addr[i] = 1;
	}
}

void region_do_nothing(int size) {
	int i;
	int j;
	int temp = 0;

	int r = FOOTPRINT / (4*1024*1024);
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

	for(i = 0; i < FOOTPRINT - 20480*r; i = i + 20480*r) {
		memcpy(page->addr+i, &value, size);
		memcpy(page->addr+i+1024*r+4, &value, size);
		memcpy(page->addr+i+2048*r+8, &value, size);
		memcpy(page->addr+i+3072*r+2, &value, size);
		memcpy(page->addr+i+4096*r+4, &value, size);
		memcpy(page->addr+i+5120*r+8, &value, size);
		memcpy(page->addr+i+6144*r+2, &value, size);
		memcpy(page->addr+i+7168*r+4, &value, size);
		memcpy(page->addr+i+8192*r+2, &value, size);
		memcpy(page->addr+i+9216*r+4, &value, size);
		memcpy(page->addr+i+10240*r+8, &value, size);
		memcpy(page->addr+i+11264*r+2, &value, size);
		memcpy(page->addr+i+12288*r+2, &value, size);
		memcpy(page->addr+i+13312*r+4, &value, size);
		memcpy(page->addr+i+14336*r+8, &value, size);
		memcpy(page->addr+i+15360*r+2, &value, size);
		memcpy(page->addr+i+16384*r+4, &value, size);
		memcpy(page->addr+i+17408*r+2, &value, size);
		memcpy(page->addr+i+18432*r+8, &value, size);
		memcpy(page->addr+i+19456*r+4, &value, size);
	}
}

void region_destroy() {
	free(page);
}

