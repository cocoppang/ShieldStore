#include "Enclave.h"

static uint8_t* secret_object = NULL;
static staging_hashtable* ht_staging_enclave = NULL;

/**
 * Hash a string for a staging hash table. 
 **/
int ht_staging_hash(char *key){
	return ht_hash(key) % ht_staging_enclave->size;
}

/** 
 * Free the kv entries in unprotected region
 **/
void delete_entry(entry* pair) {
	ocall_tc_free(pair->key_val);
	ocall_tc_free(pair);
}

/** 
 * initialize the staging hashtable when the staging flag is on
 **/
void init_staging_hashtable(int size) {

	if((ht_staging_enclave = (staging_hashtable*)ocall_tc_malloc(sizeof(staging_hashtable))) == NULL) return;
	if((ht_staging_enclave->table = (staging_entry**)ocall_tc_malloc(sizeof(staging_entry*)*size)) == NULL) return;

	for(int i = 0 ; i < size; i++) ht_staging_enclave->table[i] = NULL;
	ht_staging_enclave->size = size;
}

/** 
 * Get the entry with specific key on a staging hashtable
 **/
staging_entry* ht_get_staging_buffer_o(char *key, int key_size, int* kv_pos) {
	staging_entry* staging_pair = NULL;
	entry* kv_entry = NULL;
	int bin = 0;
	int key_hash = 0;
	uint8_t updated_nac[NAC_SIZE];
	char* plain_key_val = NULL;

	bin = ht_staging_hash(key);
	staging_pair = ht_staging_enclave->table[bin];

	if(arg_enclave.key_opt) {
		key_hash = key_hash_func(key);
	}

	if(arg_enclave.key_opt) {
		while(staging_pair != NULL) {
			kv_entry = staging_pair->kv_entry;
			if(kv_entry->key_val != NULL && kv_entry->key_size == key_size && key_hash == kv_entry->key_hash) {
				if((plain_key_val = decrypt_key_val_and_compare(key, kv_entry->key_val, kv_entry->key_size, kv_entry->val_size, kv_entry->nac, updated_nac)) != NULL) {
					*kv_pos = staging_pair->kv_pos;
					return staging_pair;
				}
			}
			staging_pair = staging_pair->next;
		}
	}
	else { 
		while(staging_pair != NULL) {
			kv_entry = staging_pair->kv_entry;
			if(kv_entry->key_val != NULL && kv_entry->key_size == key_size) {
				if((plain_key_val = decrypt_key_val_and_compare(key, kv_entry->key_val, kv_entry->key_size, kv_entry->val_size, kv_entry->nac, updated_nac)) != NULL) {
					*kv_pos = staging_pair->kv_pos;
					return staging_pair;
				}
			}
			staging_pair = staging_pair->next;
		}
	}
	
	return NULL;
}

/**
 * Insert a key-value pair into a staging buffer. 
 **/
void ht_set_staging_buffer_o(staging_entry* ret_entry, char *key, char *key_val, uint8_t *nac, uint8_t *mac, int key_len, int val_len, int kv_pos){
	staging_entry *newpair = NULL;

	int bin = ht_hash(key);
	int staging_bin = ht_staging_hash(key);
	uint8_t key_hash = key_hash_func(key);

	//if the key is already in the key table, replace the staging buffer entry
	if(ret_entry != NULL)
	{
		if(ret_entry->kv_entry->val_size != val_len) 
			ret_entry->kv_entry->key_val = (char*)ocall_tc_realloc(ret_entry->kv_entry->key_val, sizeof(char)*(key_len+val_len));
		memcpy(ret_entry->kv_entry->key_val, key_val, key_len+val_len);
		memcpy(ret_entry->kv_entry->nac, nac, NAC_SIZE);
		memcpy(ret_entry->kv_entry->mac, mac, MAC_SIZE);
		ret_entry->kv_entry->val_size = val_len;
	}
	else{
		//Insert
		newpair = (staging_entry*)ocall_tc_malloc(sizeof(staging_entry));
		newpair->kv_entry = ht_newpair(key_len, val_len, key_hash, key_val, nac, mac);
		newpair->kv_pos = kv_pos;
		newpair->next = NULL;

		if(newpair == NULL || newpair->kv_entry == NULL)
			print("new pair problem");

		//Add newpair at the first entry of the chain
		if(ht_staging_enclave->table[staging_bin] == NULL) { 
			newpair->next = NULL;
			ht_staging_enclave->table[staging_bin] = newpair;
		}
		else {
			newpair->next = ht_staging_enclave->table[staging_bin];
			ht_staging_enclave->table[staging_bin] = newpair;
		}
	}

	if(arg_enclave.mac_opt)
	{
		/** The order of MAC bucket is reversed to hash chain order **/
		memcpy(MACbuf_enclave->entry[bin].mac+(MAC_SIZE*kv_pos), mac, MAC_SIZE);
	}
}

/**
 * Replace the staging buffer entry to kv table 
 **/
void ht_replace_o(entry* replace_entry, char *key, int key_size)  {
	
	entry* prior_entry = NULL;
	entry* free_entry = NULL;
	entry* next_entry = NULL;
	entry* pair = NULL;

	uint8_t updated_nac[NAC_SIZE];
	char* plain_key_val = NULL;

	bool found = false;

	int bin = 0;
	uint8_t key_hash = 0;

	if(arg_enclave.key_opt) {
		key_hash = key_hash_func(key);
	}

	bin = ht_hash(key);

	pair = ht_enclave->table[bin];

	if(arg_enclave.key_opt)
	{
		while(pair != NULL)
		{	
			//check key_size && key_hash value to find matching key
			if(pair->key_val != NULL && pair->key_size == key_size && key_hash == pair->key_hash)
			{
				if((plain_key_val = decrypt_key_val_and_compare(key, pair->key_val, pair->key_size, pair->val_size, pair->nac, updated_nac)) != NULL)
				{
					assert(plain_key_val != NULL);
					found = true;
					break;
				}
			}
			prior_entry = pair;
			pair = pair->next;
		}
	}
	else
	{
		while(pair != NULL)
		{	
			//check key_size && key_hash value to find matching key
			if(pair->key_val != NULL && pair->key_size == key_size)
			{
				if((plain_key_val = decrypt_key_val_and_compare(key, pair->key_val, pair->key_size, pair->val_size, pair->nac, updated_nac)) != NULL)
				{
					assert(plain_key_val != NULL);
					found = true;
					break;
				}
			}
			prior_entry = pair;
			pair = pair->next;
		}
	}

	if(found == true) { 
		next_entry = pair->next;
		free_entry = pair;
		pair = replace_entry;
		if(free_entry != ht_enclave->table[bin])
			prior_entry->next = pair;
		else
			ht_enclave->table[bin] = pair;
		pair->next = next_entry;
		delete_entry(free_entry);
	}
	else {
		if(ht_enclave->table[bin] == NULL) {
			replace_entry->next = NULL;
			ht_enclave->table[bin] = replace_entry;
		}
		else {
			replace_entry->next = ht_enclave->table[bin];
			ht_enclave->table[bin] = replace_entry;
		}
	}

	free(plain_key_val);
}

/**
 * zeroing the sealed object which will store sealed meta-data
 **/
void init_secret_object() {
	secret_object = (uint8_t*)malloc(sizeof(uint8_t)*REPLAY_PROTECTED_SECRET_SIZE);
	memset(secret_object, 0, REPLAY_PROTECTED_SECRET_SIZE);
}

/**
 * update the sealed object with updated meta-data
 **/
void update_secret_object() {
	uint8_t** internal_root_l1;
	uint8_t** internal_root_l2;
	uint8_t* aggregated_hash;
	uint8_t temp_mac[MAC_SIZE];
	sgx_status_t ret;

	int l1_size = arg_enclave.tree_root_size*MAC_SIZE/PAGE_SIZE/PAGE_SIZE;
	int l2_size = arg_enclave.tree_root_size*MAC_SIZE/PAGE_SIZE;
	
	internal_root_l1 = (uint8_t**)malloc(sizeof(uint8_t*)*l1_size);
	internal_root_l2 = (uint8_t**)malloc(sizeof(uint8_t*)*l2_size);

	for(int i = 0 ; i < l2_size; i++)
	{
		if(i < l1_size) {
			internal_root_l1[i] = (uint8_t*)malloc(sizeof(uint8_t)*MAC_SIZE);
		}

		internal_root_l2[i] = (uint8_t*)malloc(sizeof(uint8_t)*MAC_SIZE);
	}

	aggregated_hash = (uint8_t*)malloc(sizeof(uint8_t)*PAGE_SIZE);
	memset(aggregated_hash, 0, PAGE_SIZE);
	/** Naive approach : Hash Tree consturction with page granularity **/
	//L2 level hash construction
	for(int i = 0, j = 0, k = 0 ; i < arg_enclave.tree_root_size && k < l2_size; i++, j++) {
		memcpy(aggregated_hash+j*MAC_SIZE, MACTable[i].mac, MAC_SIZE);

		if(j == (PAGE_SIZE/MAC_SIZE - 1)) { 
			ret = sgx_rijndael128_cmac_msg(&gsk, aggregated_hash, PAGE_SIZE , &temp_mac);
			if(ret == SGX_SUCCESS) {
				memcpy(internal_root_l2[k], temp_mac, MAC_SIZE);
			}
			else{
				print("Error on updating secret");
			}
			memset(aggregated_hash, 0, PAGE_SIZE);
			k++;
			j = 0;
		}
	}
	print("L2 construction done");

	memset(aggregated_hash, 0, PAGE_SIZE);
	//L1 level hash construction
	for(int i = 0, j = 0, k = 0; i < l2_size && k < l1_size; i++, j++) {
		memcpy(aggregated_hash+j*MAC_SIZE, internal_root_l2[i], MAC_SIZE);

		if(j == (PAGE_SIZE/MAC_SIZE - 1)) { 
			ret = sgx_rijndael128_cmac_msg(&gsk, aggregated_hash, PAGE_SIZE , &temp_mac);
			if(ret == SGX_SUCCESS) {
				memcpy(internal_root_l1[k], temp_mac, MAC_SIZE);
			}
			else{
				print("Error on updating secret");
			}
			memset(aggregated_hash, 0, PAGE_SIZE);
			k++;
			j = 0;
		}
	}
	print("L1 construction done");

	/** update secret **/
	assert(REPLAY_PROTECTED_SECRET_SIZE > l1_size*MAC_SIZE);
	memset(secret_object, 0, REPLAY_PROTECTED_SECRET_SIZE);

	for(int i = 0; i < l1_size; i++) {
		memcpy(secret_object+i*MAC_SIZE, internal_root_l1[i], MAC_SIZE);
	}

	for(int i = 0 ; i < l2_size; i++) {
		if(i < l1_size) 
			free(internal_root_l1[i]);
		free(internal_root_l2[i]);
	}
	free(internal_root_l2);
	free(internal_root_l1);
	free(aggregated_hash);
}

/**
 * return the secret object
 **/
uint8_t* get_secret_object() {
	return secret_object;
}

/**
 * update the staging buffer to original kv hash table 
 * when the child process finish to store kv entries on a file.
 * It checks all the entries of the staging buffer to replace the original kv entry to staging entries.
 * When it finishes to replace all the entries of staging buffer, it clears the staging buffer and keytable entries.
 */
void update_staging_buffer_in_kvs() {
	staging_entry* pair = NULL;
	staging_entry* cur_pair = NULL;
	entry* kv_entry = NULL;

	char* key = NULL;
	sgx_status_t ret = SGX_SUCCESS;

	for(int i = 0 ; i < ht_staging_enclave->size; i++) {
		pair = ht_staging_enclave->table[i];
		while(pair != NULL) {
			cur_pair = pair;
			kv_entry = cur_pair->kv_entry;
			pair = pair->next;
			
			//Update the kv entries using staging buffer
			ret = enclave_verification(kv_entry->key_val, kv_entry->key_hash, 
					kv_entry->key_size, kv_entry->val_size, kv_entry->nac, kv_entry->mac);

			if(ret != SGX_SUCCESS)
			{
				print("MAC verification failed");
			}
			key = decrypt_and_get_key(kv_entry->key_val, kv_entry->key_size, kv_entry->val_size, kv_entry->nac);

			ht_replace_o(kv_entry, key, kv_entry->key_size);
			
			free(key);
			ocall_tc_free(cur_pair);

		}
	}
	ocall_tc_free(ht_staging_enclave->table);
	ocall_tc_free(ht_staging_enclave);
	
	print("Staging buffer update DONE");
}


