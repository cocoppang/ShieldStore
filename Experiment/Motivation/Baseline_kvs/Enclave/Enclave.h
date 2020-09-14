#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

/* DATA STRUCTURE DECLARATION */
typedef struct entry {
	int key_size;
	int val_size;
	char *key;
	char *val;
	struct entry *next;
} entry;

typedef struct hashtable {
	int size;
	entry **table;
} hashtable;

#endif /* !_ENCLAVE_H_ */
