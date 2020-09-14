#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "sgx_trts.h"
#include "Enclave_t.h"
#include "Enclave.h"

static hashtable *ht = NULL;

/* Create a new hashtable. */
void ht_create(int size){
	if(size < 1) {
		print("Size of hashtable is negative");
	}

	/* Allocate the table itself. */
	ht = (hashtable *)malloc(sizeof(hashtable));
	if(!ht) print("Hashtable is not allocated");

	/* Allocate pointers to the head nodes. */
	ht->table = (entry **)malloc(sizeof(entry *) * size);
	if(!ht->table) print("Array of Pointers is not allocated");

	/* Allocate and initialize the entries */
	for(int i = 0; i < size; i++) {
		ht->table[i] = NULL;
	}
	ht->size = size;
}

void ht_destroy(int size) {
	for(int i = 0; i < size; i++) {
		if(!ht->table[i])
			free(ht->table[i]);
	}
	free(ht->table);
	free(ht);
}

/* Hash a string for a particular hash table. */
int ht_hash(char *key){
	unsigned long int hashval = 7;
	int i = 0;

	/* Convert our string to an integer */
	while(hashval < ULONG_MAX && i < strlen(key)){
		hashval = hashval * 61;
		hashval += key[i];
		i++;
	}

	return hashval % ht->size;
}

/* Create a key-value pair. */
entry * ht_newpair(char *key, char *val, int key_size, int val_size){
	entry *newpair;

	if((newpair = (entry *)malloc(sizeof(entry))) == NULL) {
		print("Newpair is not created!");
	}

	if((newpair->key = (char *)malloc(sizeof(char)*key_size)) == NULL) return NULL;
	if((newpair->val = (char *)malloc(sizeof(char)*val_size)) == NULL) return NULL;

	if(memcpy(newpair->key, key, key_size) == NULL) return NULL;
	if(memcpy(newpair->val, val, val_size) == NULL) return NULL;

	newpair->key_size = key_size;
	newpair->val_size = val_size;
	newpair->next = NULL;

	return newpair;
}

/* Insert a key-value pair into a hash table. */
void ht_set(char *key, char *val, int key_size, int val_size){
	int bin = 0;
	entry *newpair = NULL;
	entry *next = NULL;
	entry *last = NULL;

	bin = ht_hash(key);
	next = ht->table[bin];

	while(next != NULL && next->key != NULL	&& (strcmp(key, next->key) != 0)){
		last = next;
		next = next->next;
	}

	/* There's already a pair.  Let's replace that string. */
	if(next != NULL && next->key != NULL && (strcmp(key, next->key) == 0)){
		if(next->val_size != val_size)
			next->val = (char*)realloc(next->val, sizeof(char)*val_size);
		memcpy(next->val, val, val_size);
		next->val_size = val_size;
	}
	/* Nope, could't find it.  Time to grow a pair. */
	else{
		newpair = ht_newpair(key, val, key_size, val_size);

		/* We're at the start of the linked list in this bin. */
		if(next == ht->table[bin]){
			newpair->next = next;
			ht->table[bin] = newpair;
		}

		/* We're at the end of the linked list in this bin. */
		else if(next == NULL){
			last->next = newpair;
		}

		/* We're in the middle of the list. */
		else{
			newpair->next = next;
			last->next = newpair;
		}
	}
}

/* Retrieve a key-value pair from a hash table. */
entry * ht_get(char *key){
	int bin = 0;
	entry *pair;

	bin = ht_hash(key);

	/* Step through the bin, looking for our value. */
	pair = ht->table[bin];

	while(pair != NULL && pair->key != NULL && (strcmp(key, pair->key) != 0))
		pair = pair->next;

	/* Did we actually find anything? */
	if(pair == NULL || pair->key == NULL || strcmp(key, pair->key) != 0)
		return NULL;
	else
		return pair;
}

void enclave_process(int count, char **inst, int count2, char **inst2) {
	char buf[BUF_SIZE];
	int i;
	char *tok;
	char *temp;
	entry *ret_entry;

	int key_size;
	int val_size;
	char *key;
	char *val;

	/* Process load */
	for(i = 0; i < count/8; i++){
		memset(buf, 0, BUF_SIZE);
		memcpy(buf, inst[i], BUF_SIZE);
		//print(buf);
		if(strncmp(buf, "SET", 3) == 0 || strncmp(buf, "set", 3) == 0)
		{
			tok = strtok_r(buf + 4, " ", &temp);

			key_size = strlen(tok)+1;
			key = (char*)malloc(sizeof(char)*key_size);
			memset(key, 0, key_size);
			memcpy(key, tok, key_size-1);

			val_size = strlen(temp);
			val = (char*)malloc(sizeof(char)*val_size);
			memset(val, 0, val_size);
			memcpy(val, temp, val_size-1);

			ht_set(key, val, key_size, val_size);

			free(key);
			free(val);
		}
		else {
			print("Invalid load command");
		}
	}

	dummy();

	print("Load Process has done");

	/* Process run */
	for(i = 0; i < count2/8; i++){
		memset(buf, 0, BUF_SIZE);
		memcpy(buf, inst2[i], BUF_SIZE);
		//print(buf);
		if(strncmp(buf, "GET", 3) == 0 || strncmp(buf, "get", 3) == 0)
		{
			tok = strtok_r(buf + 4, " ", &temp);

			key_size = strlen(tok); // Remove -1 because \n
			key = (char*)malloc(sizeof(char)*key_size);
			memset(key, 0, key_size);
			memcpy(key, tok, key_size-1);

			ret_entry = ht_get(key);

			if(ret_entry == NULL)
			{
				print("No data");
			}

			free(key);
		}
		else {
			print("Invalid run command");
		}
	}

	print("Run Process has done");
}
