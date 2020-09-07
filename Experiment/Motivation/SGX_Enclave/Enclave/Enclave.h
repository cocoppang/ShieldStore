#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#define FOOTPRINT 1024*1024*1024

struct region {
	int addr[FOOTPRINT];
};

struct region *page;
int value = 1;

#endif /* !_ENCLAVE_H_ */
