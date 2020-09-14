#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

/* PREDEFINED DATA */
#define BUF_SIZE 64

/* DATA STRUCTURE DECLARATION */
struct entry{
	int key_size;
	int val_size;
	char *key;
	char *val;
	struct entry *next;
};
typedef struct entry entry;

struct hashtable{
	int size;
	struct entry **table;
};
typedef struct hashtable hashtable;

#endif /* !_SERVER_H_ */

