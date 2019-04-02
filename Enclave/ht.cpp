#include "Enclave.h"

/**
 * Hash function for key-hint 
 **/
uint8_t key_hash_func(char *key){
	unsigned long int hashval = 7;
	int i = 0;
	/* Convert our string to an integer */
	while(hashval < ULONG_MAX && i < strlen(key)){
		hashval = hashval*11;
		hashval += key[i];
		i++;
	}

	return hashval % 256; //int8_t can store 0 ~ 255
}


/** 
 * Hash a string for a particular hash table.
 **/
int ht_hash(char *key){
	unsigned long int hashval = 7;
	int i = 0;
	/* Convert our string to an integer */
	while(hashval < ULONG_MAX && i < strlen(key)){
		hashval = hashval*61;
		hashval += key[i];
		i++;
	}

	return hashval % ht_enclave->size;
}

/**
 * Retrieve a key-value pair from a hash table. 
 **/
entry * ht_get_o(char *key, int32_t key_size, char **plain_key_val, int* kv_pos, uint8_t (&updated_nac)[NAC_SIZE]){
	int bin = 0;
	uint8_t key_hash = 0;
	entry *pair;

	bin = ht_hash(key);
	*kv_pos = 0;

	if(arg_enclave.key_opt)
	{
		key_hash = key_hash_func(key);
	}

	/* Step through the bin, looking for our value. */
	pair = ht_enclave->table[bin];

	if(arg_enclave.key_opt)
	{
		while(pair != NULL)
		{	
			//check key_size && key_hash value to find matching key
			if(pair->key_val != NULL && pair->key_size == key_size && key_hash == pair->key_hash)
			{
				if((*plain_key_val = decrypt_key_val_and_compare(key, pair->key_val, pair->key_size, pair->val_size, pair->nac, updated_nac)) != NULL)
				{
					assert(plain_key_val != NULL);
					
					/** range check operations **/
					assert(sgx_is_outside_enclave(pair, sizeof(*pair)));
					assert(sgx_is_outside_enclave(pair->key_val, pair->key_size + pair->val_size));

					return pair;
				}
			}
			pair = pair->next;
			(*kv_pos)++;
		}
	}

	/** When the key optimization is on, ShieldStore runs two-step search **/
	*kv_pos = 0;
	pair = ht_enclave->table[bin];

	while(pair != NULL)
	{	
		//check key_size && key_hash value to find matching key
		if(pair->key_val != NULL && pair->key_size == key_size)
		{
			if((*plain_key_val = decrypt_key_val_and_compare(key, pair->key_val, pair->key_size, pair->val_size, pair->nac, updated_nac)) != NULL)
			{
				assert(plain_key_val != NULL);

				/** range check operations **/
				assert(sgx_is_outside_enclave(pair, sizeof(*pair)));
				assert(sgx_is_outside_enclave(pair->key_val, pair->key_size + pair->val_size));

				return pair;
			}
		}
		pair = pair->next;
		(*kv_pos)++;
	}

	return NULL;

}

/**
 * Create a key-value pair. 
 **/
entry * ht_newpair(int key_size, int val_size, uint8_t key_hash, char *key_val, uint8_t *nac, uint8_t *mac){
	entry *newpair;

	if((newpair = (entry *)ocall_tc_malloc(sizeof(entry))) == NULL) return NULL;

	if((newpair->key_val = (char*)ocall_tc_malloc(sizeof(char)*(key_size+val_size))) == NULL) return NULL;

	if(memcpy(newpair->key_val, key_val, key_size+val_size) == NULL) return NULL;

	if(memcpy(newpair->nac, nac, NAC_SIZE) == NULL) return NULL;

	if(memcpy(newpair->mac, mac, MAC_SIZE) == NULL) return NULL;

	newpair->key_hash = key_hash;
	newpair->key_size = key_size;
	newpair->val_size = val_size;
	newpair->next = NULL;

	return newpair;
}

/** Insert a key-value pair into a hash table. 
 **/
void ht_set_o(entry* updated_entry, char *key, char *key_val, uint8_t *nac, uint8_t *mac, uint32_t key_len, uint32_t val_len, int kv_pos) {
	int bin = 0;
	uint8_t key_hash = 0;

	entry *newpair = NULL;

	bin = ht_hash(key);
	key_hash = key_hash_func(key);

	//Update
	if(updated_entry != NULL) {
		if(updated_entry->val_size != val_len)
			updated_entry->key_val = (char*)ocall_tc_realloc(updated_entry->key_val, sizeof(char)*(key_len+val_len));
		memcpy(updated_entry->key_val, key_val, key_len+val_len);
		memcpy(updated_entry->nac, nac, NAC_SIZE);
		memcpy(updated_entry->mac, mac, MAC_SIZE);
		updated_entry->val_size = val_len;
	}
	//Insert
	else {
		newpair = ht_newpair(key_len, val_len, key_hash, key_val, nac, mac);

		if(newpair == NULL)
			print("new pair problem");

		/** New-pair should be inserted into the first entry of the bucket chain **/
		if(updated_entry == ht_enclave->table[bin]){
			newpair->next = NULL;
			ht_enclave->table[bin] = newpair;
		}
		else if(updated_entry == NULL) {
			newpair->next = ht_enclave->table[bin];
			ht_enclave->table[bin] = newpair;
		}
		else {
			print("do not enter here");
		}

		if(arg_enclave.mac_opt)
		{
			//mac buffer increase
			MACbuf_enclave->entry[bin].size++;
		}
	}
	if(arg_enclave.mac_opt)
	{
		/** The order of MAC bucket is reversed to hash chain order **/
		memcpy(MACbuf_enclave->entry[bin].mac+(MAC_SIZE*kv_pos), mac, MAC_SIZE);
	}
}

/** 
 * append the value of key-value pair 
 **/
void ht_append_o(entry* updated_entry, char *key, char *key_val, uint8_t *nac, uint8_t *mac, uint32_t key_len, uint32_t val_len, int kv_pos) {
	int bin = 0;

	bin = ht_hash(key);

	//Update
	if(updated_entry != NULL) {
		updated_entry->key_val = (char*)ocall_tc_realloc(updated_entry->key_val, sizeof(char)*(key_len+val_len));
		memcpy(updated_entry->key_val, key_val, key_len+val_len);
		memcpy(updated_entry->nac, nac, NAC_SIZE);
		memcpy(updated_entry->mac, mac, MAC_SIZE);
		updated_entry->val_size = val_len;
	}
	else {
		print("There's no data in database");
		return;
	}

	if(arg_enclave.mac_opt)
	{
		/** The order of MAC bucket is reversed to hash chain order **/
		memcpy(MACbuf_enclave->entry[bin].mac+(MAC_SIZE*kv_pos), mac, MAC_SIZE);
	}
}

