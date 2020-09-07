#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "sgx_trts.h"
#include "Enclave_t.h"
#include "Enclave.h"

/* Create a new hashtable. */
void ht_create(int size){
	if(size < 1) {
		print("Size of hashtable is negative");
	}

	ht = (hashtable *)malloc(sizeof(hashtable));
	if(!ht) print("Hashtable is not allocated");

	ht->table = (entry **)memsys5Malloc(sizeof(entry*)*size);
	if(!ht->table) print("Array of Pointers is not allocated");

	for(int i = 0 ; i < size; i++) {
	  Aptr<entry *> tmp_aptr = Aptr<entry *> (&(ht->table[i]), sizeof(entry *), 0);
	  tmp_aptr.m_aptr.hint_write = true;
	  entry **table_entry = (entry **)deref(&tmp_aptr.m_aptr, tmp_aptr.m_base_page_index);
		*table_entry = NULL;
	}

	ht->size = size;
}

void ht_destroy(int size) {
	memsys5Free(ht->table);
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
entry * ht_newpair(char *key, char *val, int key_len, int val_len){
	
	entry* newpair;
	
	ASSERT(key_len > 0 && val_len > 0);

	newpair = (entry*)memsys5Malloc(sizeof(entry));
	ASSERT(newpair);

	Aptr<entry> new_aptr(newpair, sizeof(entry), 0);
	new_aptr.m_aptr.hint_write = true;

	entry *new_entry = (entry *)deref(&new_aptr.m_aptr, new_aptr.m_base_page_index);

	new_entry->key = (char*)memsys5Malloc(sizeof(char)*key_len);
	ASSERT(new_entry->key);

	new_entry->val = (char*)memsys5Malloc(sizeof(char)*val_len);
	ASSERT(new_entry->val);

	Aptr<char> ptr_key = Aptr<char>(new_entry->key, sizeof(char)*key_len, 0);
	Aptr<char> ptr_val = Aptr<char>(new_entry->val, sizeof(char)*val_len, 0);

	memcpy_aptr_reg((char*)&(ptr_key), key, key_len);
	memcpy_aptr_reg((char*)&(ptr_val), val, val_len);

	//new_entry->key_size = key_len;
	//new_entry->val_size = val_len;
	new_entry->next = NULL;
	
	return newpair;
}

/* Insert a key-value pair into a hash table. */
void ht_set(char *key, char *val, int key_len, int val_len){
	int bin = 0;
	entry *newpair = NULL;
	Aptr<entry> last_aptr;
	Aptr<entry> next_aptr;
	entry* next_entry;
	entry* new_entry;

	char* key_value;
	Aptr<char> ptr_key;
	Aptr<char> ptr_val;

	bin = ht_hash(key);

	Aptr<entry *> tmp_aptr = Aptr<entry *> (&(ht->table[bin]), sizeof(entry *), 0);
	tmp_aptr.m_aptr.hint_write = true;

	entry **table_entry =  (entry**)deref(&tmp_aptr.m_aptr, tmp_aptr.m_base_page_index);

	if (*table_entry) {
	/** bucket first entry exists **/
	    next_aptr = Aptr<entry>(*table_entry, sizeof(entry), 0);
	} else {
	/** bucket first entry is NULL **/
      next_aptr.init();
	}
	
	while(next_aptr.is_not_null() ) {

	  next_aptr.m_aptr.hint_write = true;
		next_entry = (entry*)deref(&next_aptr.m_aptr, next_aptr.m_base_page_index);
	
		if(next_entry->key != NULL) {
			//ptr_key = Aptr<char>(next_entry->key, next_entry->key_size, 0);
			ptr_key = Aptr<char>(next_entry->key, KEY_SIZE, 0);

			key_value = (char*)deref(&ptr_key.m_aptr, ptr_key.m_base_page_index);
			if(strcmp(key, key_value) != 0) {
				last_aptr = next_aptr;
				if (next_entry->next) {
				    next_aptr = Aptr<entry>(next_entry->next, sizeof(entry), 0);
				} else {
						next_aptr.init();
				}
			}
			else {
				break;
			}
		}
	}
	
	if(next_aptr.is_not_null()) {

		next_entry = (entry*)deref(&next_aptr.m_aptr, next_aptr.m_base_page_index);
		if(next_entry->key != NULL) {
			//ptr_key = Aptr<char>(next_entry->key, next_entry->key_size, 0);
			ptr_key = Aptr<char>(next_entry->key, KEY_SIZE, 0);
			key_value = (char*)deref(&ptr_key.m_aptr, ptr_key.m_base_page_index);

			if(strcmp(key, key_value) == 0) {
				ptr_val = Aptr<char>(next_entry->val, sizeof(char)*val_len, 0);
				memcpy_aptr_reg((char*)&ptr_val, val, val_len);
				//next_entry->val_size = val_len;
			}
		 }
		}
	
	/* Nope, could't find it.  Time to grow a pair. */
	else{
		newpair = ht_newpair(key, val, key_len, val_len);
		Aptr<entry> newpair_ap(newpair, sizeof(entry), 0);
	  newpair_ap.m_aptr.hint_write = true;

		new_entry = (entry*)deref(&newpair_ap.m_aptr, newpair_ap.m_base_page_index);
		new_entry->next = NULL;
    
		/* We're at the start of the linked list in this bin. */
		if(!(*table_entry)){
			*table_entry = newpair;
		}
		else{
			new_entry->next = *table_entry;
			*table_entry = newpair;

		}
	}
}

/* Retrieve a key-value pair from a hash table. */
entry * ht_get(char *key){
	
	int bin = 0;
	entry* pair_entry;
	Aptr<entry> pair_aptr;
	Aptr<char> ptr_key;
	char* key_value;

	bin = ht_hash(key);

	Aptr<entry *> tmp_aptr = Aptr<entry *> (&(ht->table[bin]), sizeof(entry *), 0);
	entry **table_entry =  (entry**)deref(&tmp_aptr.m_aptr, tmp_aptr.m_base_page_index);

	if(*table_entry) {
		pair_aptr = Aptr<entry>(*table_entry, sizeof(entry), 0);
	}
	else {
		pair_aptr.init();
	}
	while(pair_aptr.is_not_null()) {

		pair_entry = (entry*)deref(&pair_aptr.m_aptr, pair_aptr.m_base_page_index);

		if(pair_entry->key != NULL) {
			//ptr_key = Aptr<char>(pair_entry->key, pair_entry->key_size, 0);
			ptr_key = Aptr<char>(pair_entry->key, KEY_SIZE, 0);

			key_value = (char*)deref(&ptr_key.m_aptr, ptr_key.m_base_page_index);
			if(strcmp(key, key_value) != 0) {
				if(pair_entry->next) {
					pair_aptr = Aptr<entry>(pair_entry->next, sizeof(entry), 0);
				}
				else {
					pair_aptr.init();
				}
			}
			else {
				break;
			}
		}
	}

	/* Did we actually find anything? */
	if(!pair_aptr) {	
		print(key);
		return NULL;
	}
	else{
		pair_entry = (entry*)deref(&pair_aptr.m_aptr, pair_aptr.m_base_page_index);
		//ptr_key = Aptr<char>(pair_entry->key, pair_entry->key_size, 0);
		ptr_key = Aptr<char>(pair_entry->key,KEY_SIZE, 0);

		key_value = (char*)deref(&ptr_key.m_aptr, ptr_key.m_base_page_index);
		if(strcmp(key, key_value) != 0) {
			print("GET ERROR");
		}
		return pair_entry;
	}

}

void enclave_set(char *cipher){
	char* key;
	char* val;

	char *tok;
	char *temp_;

	int key_size;
  int val_size;

	tok = strtok_r(cipher + 4, " ", &temp_);
	//key_size = strlen(tok)+1;
	key_size = KEY_SIZE;
	key = (char*)malloc(sizeof(char)*key_size);
	memset(key, 0, key_size);
	memcpy(key, tok, key_size-1);
	
	//val_size = strlen(temp_)+1;
	val_size = VAL_SIZE;
	val = (char*)malloc(sizeof(char)*val_size);
	memset(val, 0, val_size);
	memcpy(val, temp_, val_size-1);


	ht_set(key, val, key_size, val_size);

	free(key);
	free(val);
}

void enclave_get(char *cipher){
	char* key;

	int key_size;
	char *tok;
	char *temp_;
	entry * ret;

	tok = strtok_r(cipher + 4, " ", &temp_);
	//key_size = strlen(tok)+1;
	key_size = KEY_SIZE;
	key = (char*)malloc(sizeof(char)*key_size);
	memset(key, 0, key_size);
	memcpy(key, tok, key_size-1);

	ret = ht_get(key);
	
	if(ret == NULL)
		print("No value correspond to the key");
	else { 
	//	Aptr<entry> ret_aptr(ret, sizeof(entry), 0);
	//	entry* ret_entry = (entry*)deref(&ret_aptr.m_aptr, ret_aptr.m_base_page_index);
	//	Aptr<char> ptr_val = Aptr<char>(ret_entry->val, ret_entry->val_size, 0);
	//	memcpy_reg_aptr(cipher, (char*)&ptr_val, ret_entry->val_size);
	}
	free(key);
}

int bar1_num = 0;
int bar2_num = 0;
int bar3_num = 0;
int num = 0;
sgx_thread_mutex_t global_mutex;

void enclave_process(int count[THREAD_NUM], char **inst[THREAD_NUM], int count2[THREAD_NUM], char **inst2[THREAD_NUM]) {
	char buf[BUF_SIZE];
	int thread_id, i, my_counter;
	char **inst_thread;

	/* Get a thread number instead of sgx_self()'s result */
	sgx_thread_mutex_lock(&global_mutex);
	thread_id = num;
	num += 1;

	my_counter = count[thread_id];
	inst_thread = inst[thread_id];
	++bar1_num;
	sgx_thread_mutex_unlock(&global_mutex);

	/* Barrior */
	while(1) {
		int mybar;
	    sgx_thread_mutex_lock(&global_mutex);
		mybar = bar1_num;
	    sgx_thread_mutex_unlock(&global_mutex);
	    if (mybar == THREAD_NUM) break;
	}

	/* Process load */
	for(i = 0; i < my_counter; i++){
		if (!inst_thread[i]) {
			print("BIG ERROR");
		}
		memcpy(buf, inst_thread[i], BUF_SIZE);
		if(strncmp(buf, "SET", 3) == 0 || strncmp(buf, "set", 3) == 0)
		{
			enclave_set(buf);
		}
		else {
			print("Invalid command\n");
		}
	}

	sgx_thread_mutex_lock(&global_mutex);
	my_counter = count2[thread_id];
	inst_thread = inst2[thread_id];
	++bar2_num;
	sgx_thread_mutex_unlock(&global_mutex);

	/* Barrior */
	while(1) {
		int mybar;
		sgx_thread_mutex_lock(&global_mutex);
		mybar = bar2_num;
		sgx_thread_mutex_unlock(&global_mutex);
		if (mybar == THREAD_NUM) break;
	}

	dummy();

	if(thread_id == 0)
		print("Load Process has done");

	/* Process run */
	for(i = 0; i < my_counter; i++){
		if (!inst_thread[i]) {
			print("BIG ERROR");
		}
		memcpy(buf, inst_thread[i], BUF_SIZE);
		if(strncmp(buf, "GET", 3) == 0 || strncmp(buf, "get", 3) == 0)
		{
			enclave_get(buf);
		}
		else if(strncmp(buf, "SET", 3) == 0 || strncmp(buf, "set", 3) == 0)
		{
			enclave_set(buf);
		}
		else {
			print("Invalid command\n");
		}
	}

	sgx_thread_mutex_lock(&global_mutex);
	++bar3_num;
	sgx_thread_mutex_unlock(&global_mutex);

	/* Barrior */
	while(1) {
		int mybar;
		sgx_thread_mutex_lock(&global_mutex);
		mybar = bar3_num;
		sgx_thread_mutex_unlock(&global_mutex);
		if (mybar == THREAD_NUM) break;
	}

	if(thread_id == 0)
		print("Run Process has done");
}
