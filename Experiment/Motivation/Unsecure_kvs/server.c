#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "server.h"
#include <sys/time.h>

//For mmap
#include <sys/mman.h>
#include <fcntl.h>

static char **inst = NULL;
static int count;

static char **inst2 = NULL;
static int count2;

time_t start, end, mid;

/* Hash table variable */
static hashtable *ht = NULL;

int g_size;

void print(const char* str) {
	printf("%s\n", str);
}
void print_int(int num) {
	printf("%d\n", num);
}

/* Create a new hashtable. */
hashtable * ht_create(int size){
	int i;

	if(size < 1) {
		printf("Size is negative!");
		exit(0);
	}

	/* Allocate the table itself. */
	if((ht = (hashtable *)malloc(sizeof(hashtable))) == NULL) {
		printf("HT is not created!");
		exit(0);
	}

	/* Allocate pointers to the head nodes. */
	if((ht->table = (entry **)malloc(sizeof(entry *) * size)) == NULL) {
		printf("Pointer of entry is not created!");
		exit(0);
	}

	for(i = 0; i < size; i++) {
		ht->table[i] = NULL;
	}
	ht->size = size;

	return ht;
}

void ht_destroy(hashtable *ht) {
	int i;
	for(i = 0; i < ht->size; i++) {
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

	return hashval % g_size;
}

/* Create a key-value pair. */
entry * ht_newpair(char *key, char *val, int key_size, int val_size){
	entry *newpair;

	if((newpair = (entry *)malloc(sizeof(entry))) == NULL) {
		printf("Newpair is not created!");
		exit(0);
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

	while(next != NULL && next->key != NULL && (strcmp(key, next->key) != 0)){
		last = next;
		next = next->next;
	}

	/* There's already a pair.  Let's replace that string. */
	if(next != NULL && next->key != NULL && (strcmp(key, next->key) == 0)){
		//if(next->key_size != key_size)
		//	next->key = (char*)realloc(next->key, sizeof(char)*key_size);
		if(next->val_size != val_size)
			next->val = (char*)realloc(next->val, sizeof(char)*val_size);
		//memcpy(next->key, key, key_size);
		memcpy(next->val, val, val_size);
		//next->key_size = key_size;
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

void file_to_memory(char* workload){
	/* for mmap */
	FILE *f;
	int j;

	/* Listening commands */
	char buf[BUF_SIZE];
	char filename[30];

	int set_size = 16384*atoi(workload);
	print_int(set_size);
	int set_count = 1;
	int set_size2 = 10000000;
	int set_count2 = 1;

	/* For load workload */
	count = 0;
	inst = (char**)malloc(sizeof(char*)*set_size);
	for(j = 0; j < set_size; j++)
		inst[j] = (char*)malloc(sizeof(char)*BUF_SIZE);
	
	/* For run workload */
	count2 = 0;
	inst2 = (char**)malloc(sizeof(char*)*set_size2);
	for(j = 0; j < set_size2; j++)
		inst2[j] = (char*)malloc(sizeof(char)*BUF_SIZE);

	sprintf(filename, "../workloads/load_%s.txt", workload);	
	printf("%s\n",filename);
	f = fopen(filename, "r");
	while(set_count <= set_size)
	{
		fgets(buf, BUF_SIZE, f);
		memcpy(inst[count++], buf, BUF_SIZE);

		set_count++;
	}
	fclose(f);

	sprintf(filename, "../workloads/run_%s.txt", workload);	
	printf("%s\n",filename);
	f = fopen(filename, "r");	
	while(set_count2 <= set_size2)
	{
		fgets(buf, BUF_SIZE, f);
		memcpy(inst2[count2++], buf, BUF_SIZE);

		set_count2++;
	}
	fclose(f);
}


int main(int argc, char **argv){
	g_size = 1048576;

	/* Listening commands */
	char buf[BUF_SIZE];
	FILE *f;
	int i;

	/* Command handling variable */
	char *tok;
	char *temp;
	char *key;
	char *val;
	int key_size;
	int val_size;
	entry *ret_entry;

	/* Make main hash table */
	ht = ht_create(g_size);
	printf("HT is created!\n");

	if(argc != 2)
	{
		printf("Usage: ./app [workload] : workload <-- 16, 32, 48, ... , 4096\n");
	}

	/* Load files into memory */
	file_to_memory(argv[1]);

	start = time(0);
	/* Process load */
	for(i = 0; i < count; i++){
		memset(buf, 0, BUF_SIZE);
		memcpy(buf, inst[i], BUF_SIZE);
		//printf("%s",buf);
		if(strncmp(buf, "SET", 3) == 0 || strncmp(buf, "set", 3) == 0)
		{
			tok = strtok_r(buf + 4, " ", &temp);

			key_size = strlen(tok)+1;
			key = (char*)malloc(sizeof(char)*key_size);
			memset(key, 0, key_size);
			memcpy(key, tok, key_size-1);

			val_size = strlen(temp)+1;
			val = (char*)malloc(sizeof(char)*val_size);
			memset(val, 0, val_size);
			memcpy(val, temp, val_size-1);

			ht_set(key, val, key_size, val_size);

			free(key);
			free(val);
		}
		else {
			printf("Invalid load command\n");
		}
	}

	mid = time(0); 

	printf("Load Process has done\n");

	/* Process run */
	for(i = 0; i < count2; i++){
		memset(buf, 0, BUF_SIZE);
		memcpy(buf, inst2[i], BUF_SIZE);
		//printf("%s",buf);
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
				printf("No data\n");
			}

			free(key);
		}
		else {
			printf("Invalid run command\n");
		}
	}

	printf("Run Process has done\n");
	end = time(0); 

	f = fopen("result.txt", "a");
	//fprintf(f,"Thread: 1 Workload: %sMB Execution time for loading : %f\n", argv[1], difftime(mid,start));
	fprintf(f,"Thread: 1 Workload: %sMB Execution time for Running : %f\n", argv[1], difftime(end,mid));
	fclose(f);

	ht_destroy(ht);

	return 0;
}
