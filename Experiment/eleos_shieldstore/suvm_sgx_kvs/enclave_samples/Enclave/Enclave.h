#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

//Eleos support
#include "mem.h"
#include "Aptr.h"

/* DATA STRUCTURE DECLARATION */

/* Decrease the working set size of shieldstore to match eleos */
typedef struct entry {
	char* key;
  char* val;
	//int key_size;
	//int val_size;
	struct entry* next;
} entry;

typedef struct hashtable {
	int size;
	struct entry **table;
} hashtable;

hashtable *ht;

#endif /* !_ENCLAVE_H_ */
