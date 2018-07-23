#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sgx_trts.h"
#include "sgx_tkey_exchange.h"
#include "sgx_tcrypto.h"

#include "Enclave_t.h"

#include "climits"
#include "cassert"

#include "gperftools/malloc_hook.h"
#include "gperftools/malloc_extension.h"
#include "gperftools/tcmalloc.h"

/* Global Symmetric Key */
const sgx_ec_key_128bit_t gsk = {
	0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
	0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad
};

/** Authentication Tree root for each buckets **/
struct _bucketMAC{
	uint8_t mac[MAC_SIZE];
};
typedef _bucketMAC BucketMAC;

BucketMAC *MACTable;

/** Fixed the number of tree root
 *  The ratio of tree root per buckets can be configuralbe 
 **/
int num_tree_root = 1024*1024*4;
int ratio_root_per_buckets = 1;

/* hash table */
static hashtable *ht_enclave = NULL;

/* MAC buffer */
static MACbuffer *MACbuf_enclave = NULL;

/* Hash a string for a particular hash table. */
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

/** Hash function for making key_index **/
uint8_t key_hash_func(char *key){
	unsigned long int hashval = 7;
	int i = 0;
	/* Convert our string to an integer */
	while(hashval < ULONG_MAX && i < strlen(key)){
		hashval = hashval*11;
		hashval += key[i];
		i++;
	}

	return hashval % 256; //uint8_t can store 0 ~ 255
}

/** Decrypt and compare the key to select same-key entry in a bucket chain.
 *	When it finds the same-key entry, it returns plain key_value and updated counter.
 **/
char* decrypt_key_val_and_compare(char* key, char* cipher, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *updated_nac){
	uint8_t* temp_plain = NULL;
	uint8_t temp_nac[NAC_SIZE];
	sgx_status_t ret = SGX_SUCCESS;

	memcpy(temp_nac, nac, NAC_SIZE);

	temp_plain = (uint8_t*)malloc(sizeof(uint8_t)*(key_len+val_len));

	ret = sgx_aes_ctr_decrypt(&gsk, (uint8_t *)cipher, key_len+val_len, temp_nac, NAC_SIZE, temp_plain);
	
	if(ret != SGX_SUCCESS)
	{
		assert(0);
	}

	if(memcmp(temp_plain, key, key_len) == 0) {
		memcpy(updated_nac, temp_nac, NAC_SIZE);
	}
	else {
		free(temp_plain);
		temp_plain = NULL;
	}
	return (char*)temp_plain;
}

/* Retrieve a key-value pair from a hash table. */
entry * ht_get_o(char *key, int key_size, char **plain_key_val, int* kv_pos, uint8_t (&updated_nac)[NAC_SIZE]){
	int bin = 0;
	uint8_t key_hash = 0;
	entry *pair;

	bin = ht_hash(key);
	*kv_pos = 0;

	if(Enable_KEYOPT)
	{
		key_hash = key_hash_func(key);
	}

	/* Step through the bin, looking for our value. */
	pair = ht_enclave->table[bin];

	if(Enable_KEYOPT)
	{
		while(pair != NULL)
		{	
			//check key_size && key_hash value to find matching key
			if(pair->key_val != NULL && pair->key_size == key_size && key_hash == pair->key_hash)
			{
				if((*plain_key_val = decrypt_key_val_and_compare(key, pair->key_val, pair->key_size, pair->val_size, pair->nac, updated_nac)) != NULL)
				{
					assert(plain_key_val != NULL);
					return pair;
				}
			}
			pair = pair->next;
			(*kv_pos)++;
		}
	}
	else
	{
		while(pair != NULL)
		{	
			//check key_size && key_hash value to find matching key
			if(pair->key_val != NULL && pair->key_size == key_size)
			{
				if((*plain_key_val = decrypt_key_val_and_compare(key, pair->key_val, pair->key_size, pair->val_size, pair->nac, updated_nac)) != NULL)
				{
					assert(plain_key_val != NULL);
					return pair;
				}
			}
			pair = pair->next;
			(*kv_pos)++;
		}
	}
	return NULL;

}

/* Create a key-value pair. */
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

/* Insert/Update a key-value pair into a hash table. */
void ht_set_o(entry* updated_entry, char *key, char *key_val, uint8_t *nac, uint8_t *mac, uint32_t key_len, uint32_t val_len, int kv_pos){
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

		if(Enable_MACBUFFER == 1)
		{
			//mac buffer increase
			MACbuf_enclave->entry[bin].size++;
		}
	}
	if(Enable_MACBUFFER == 1)
	{
		/** The order of MAC bucket is reversed to hash chain order **/
		memcpy(MACbuf_enclave->entry[bin].mac+(MAC_SIZE*kv_pos), mac, MAC_SIZE);
	}
}

/* Append the value of key-value pair */
void ht_append_o(entry* updated_entry, char *key, char *key_val, uint8_t *nac, uint8_t *mac, uint32_t key_len, uint32_t val_len, int kv_pos){
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

	if(Enable_MACBUFFER == 1)
	{
		/** The order of MAC bucket is reversed to hash chain order **/
		memcpy(MACbuf_enclave->entry[bin].mac+(MAC_SIZE*kv_pos), mac, MAC_SIZE);
	}
}

/** For checking the integrity of the entry, the func get the MAC values of the specific bucket chain **/
void get_chain_mac(int hash_val,  uint8_t *mac){

	uint8_t temp_mac[MAC_SIZE];
	uint8_t* aggregate_mac;
	entry *pair;

	int count = 0;
	int i;
	int aggregate_mac_idx = 0;
	int index;
	sgx_status_t ret = SGX_SUCCESS;

	/* Initialize temp_mac to zero */
	memset(temp_mac, 0, MAC_SIZE);

	/** bucket start index for verifying integrity **/
	int start_index = (int)(hash_val/ratio_root_per_buckets)*ratio_root_per_buckets;

	if(Enable_MACBUFFER == 1)
	{
		for(index = start_index; index < start_index + ratio_root_per_buckets; index++) {
			count += MACbuf_enclave->entry[index].size;
		}
		aggregate_mac = (uint8_t*)malloc(MAC_SIZE*count);
		memset(aggregate_mac, 0, MAC_SIZE*count);

		for(index = start_index; index < start_index + ratio_root_per_buckets; index++) {
			memcpy(aggregate_mac + aggregate_mac_idx, MACbuf_enclave->entry[index].mac, MAC_SIZE*MACbuf_enclave->entry[index].size);
			aggregate_mac_idx += (MAC_SIZE*MACbuf_enclave->entry[index].size);
		}
	}
	else
	{
		//TODO Make this code optimize
		/* Check chaining size */
		for(index = start_index; index < start_index + ratio_root_per_buckets; index++) {
			pair = ht_enclave->table[index];
			while(pair != NULL){
				count++;
				pair = pair->next;
			}
		}

		//verify
		aggregate_mac = (uint8_t*)malloc(MAC_SIZE*count);
		memset(aggregate_mac, 0 , MAC_SIZE*count);

		i = 0;
		for(index = start_index; index < start_index + ratio_root_per_buckets; index++) {
			pair = ht_enclave->table[index];
			while(pair != NULL){
				memcpy(aggregate_mac+(MAC_SIZE*i), pair->mac, MAC_SIZE);
				i++;
				pair = pair->next;
			}
		}
	}

	ret = sgx_rijndael128_cmac_msg(&gsk, aggregate_mac, MAC_SIZE * count, &temp_mac);

	if(ret != SGX_SUCCESS)
	{
		assert(0);
	}
	free(aggregate_mac);

	/* Copy generated MAC to enclave */
	memcpy(mac, (char*)temp_mac, MAC_SIZE);
}


/** Configure the ratio of tree root per buckets and initialize tree roots **/
void enclave_init_tree_root(hashtable* ht_, MACbuffer* MACbuf_){

	int i;
	ht_enclave = ht_;	
	MACbuf_enclave = MACbuf_;

	ratio_root_per_buckets = ht_enclave->size/num_tree_root;

	MACTable = (BucketMAC*)malloc(sizeof(BucketMAC)* num_tree_root);
	for(i = 0; i < num_tree_root; i++)
	{
		memset(MACTable[i].mac ,0 ,MAC_SIZE);
	}
}

/** Update tree root **/
void enclave_rebuild_tree_root(int hash_val){

	uint8_t temp_mac[MAC_SIZE];

	memset(temp_mac, 0, MAC_SIZE);

	get_chain_mac(hash_val, temp_mac);

	memcpy(MACTable[hash_val/ratio_root_per_buckets].mac , temp_mac, MAC_SIZE);
}

/** Compare tree root and current tree root **/
sgx_status_t enclave_verify_tree_root(int hash_val){

	uint8_t cur_mac[MAC_SIZE];

	get_chain_mac(hash_val, cur_mac);

	if(memcmp(cur_mac, MACTable[hash_val/ratio_root_per_buckets].mac , MAC_SIZE) != 0)
		return SGX_ERROR_UNEXPECTED;

	return SGX_SUCCESS;
}

/** Encrypt key and value and generate MAC for the entry **/
void enclave_encrypt(char* key_val, char *cipher, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac){
	uint8_t* temp_cipher;          // Temp area to store cipher text
	uint8_t* temp_all;  // Temp area to store all data
	uint8_t temp_nac[NAC_SIZE];
	uint8_t temp_mac[MAC_SIZE];             // Temp area to store MAC

	memcpy(temp_nac, nac, NAC_SIZE);

	temp_cipher = (uint8_t*)cipher;
	temp_all = (uint8_t*)malloc(sizeof(uint8_t)*(key_len+val_len+NAC_SIZE+1)+sizeof(uint32_t)*2);

	/* Encrypt plain text first */
	sgx_status_t ret = sgx_aes_ctr_encrypt(&gsk, (uint8_t *)key_val, key_len+val_len, temp_nac, NAC_SIZE, temp_cipher);

	if(ret != SGX_SUCCESS)
	{
		assert(0);
	}

	/* Generate MAC */
	memcpy(temp_all, temp_cipher, key_len+val_len);
	memcpy(temp_all + key_len + val_len, nac, NAC_SIZE);
	memcpy(temp_all + key_len + val_len + NAC_SIZE, &key_idx, sizeof(uint8_t));
	memcpy(temp_all + key_len + val_len + NAC_SIZE + sizeof(uint8_t), &key_len, sizeof(uint32_t));
	memcpy(temp_all + key_len + val_len + NAC_SIZE + sizeof(uint8_t) + sizeof(uint32_t), &val_len, sizeof(uint32_t));
	sgx_rijndael128_cmac_msg(&gsk, temp_all, key_len + val_len + NAC_SIZE+sizeof(uint8_t)+sizeof(uint32_t)*2, &temp_mac);

	/* Copy results to outside of enclave */
	cipher = (char*)temp_cipher;
	memcpy(mac, temp_mac, MAC_SIZE);

	free(temp_all);
}

/** Generate MAC for the entry and verify integrity of the entry **/
sgx_status_t enclave_verification(char *cipher, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac) {
	uint8_t* temp_all;  // Temp area to store all data
	uint8_t temp_nac[NAC_SIZE];             // Temp area to store MAC
	uint8_t temp_mac[MAC_SIZE];             // Temp area to store MAC

	/* Generate nac */
	memcpy(temp_nac, nac, NAC_SIZE);

	temp_all = (uint8_t*)malloc(sizeof(uint8_t)*(key_len+val_len+NAC_SIZE+1)+sizeof(uint32_t)*2);

	memcpy(temp_all, cipher, key_len+val_len);
	memcpy(temp_all + key_len + val_len, temp_nac, NAC_SIZE);
	memcpy(temp_all + key_len + val_len + NAC_SIZE, &key_idx, sizeof(uint8_t));
	memcpy(temp_all + key_len + val_len + NAC_SIZE + sizeof(uint8_t), &key_len, sizeof(uint32_t));
	memcpy(temp_all + key_len + val_len + NAC_SIZE + sizeof(uint8_t) + sizeof(uint32_t), &val_len, sizeof(uint32_t));
	sgx_status_t ret = sgx_rijndael128_cmac_msg(&gsk, temp_all, key_len+val_len+NAC_SIZE+sizeof(uint8_t)+sizeof(uint32_t)*2, &temp_mac);

	/* When MAC is same */
	if(memcmp(temp_mac, mac, MAC_SIZE) == 0){
		;
	}
	/* When MAC is not same */
	else{
		print("MAC matching failed");
	}

	free(temp_all);

	return ret;
}

/** Deprecated function
 * 	Decrypt key and value of the entry and verify integirty of the entry **/
void enclave_decrypt(char *cipher, char *plain, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac){
	uint8_t* temp_plain;           // Temp area to store cipher text
	uint8_t* temp_all;  // Temp area to store all data
	uint8_t temp_nac[NAC_SIZE];             // Temp area to store MAC
	uint8_t temp_mac[MAC_SIZE];             // Temp area to store MAC

	/* Generate nac */
	memcpy(temp_nac, nac, NAC_SIZE);

	/* Generate MAC & compare it */
	temp_plain = (uint8_t*)plain;
	temp_all = (uint8_t*)malloc(sizeof(uint8_t)*(key_len+val_len+NAC_SIZE+1)+sizeof(uint32_t)*2);

	memcpy(temp_all, cipher, key_len+val_len);
	memcpy(temp_all + key_len + val_len, temp_nac, NAC_SIZE);
	memcpy(temp_all + key_len + val_len + NAC_SIZE, &key_idx, sizeof(uint8_t));
	memcpy(temp_all + key_len + val_len + NAC_SIZE + sizeof(uint8_t), &key_len, sizeof(uint32_t));
	memcpy(temp_all + key_len + val_len + NAC_SIZE + sizeof(uint8_t) + sizeof(uint32_t), &val_len, sizeof(uint32_t));
	sgx_status_t ret = sgx_rijndael128_cmac_msg(&gsk, temp_all, key_len+val_len+NAC_SIZE+sizeof(uint8_t)+sizeof(uint32_t)*2, &temp_mac);

	if(ret != SGX_SUCCESS)
	{
		assert(0);
	}

	/* When MAC is same */
	if(memcmp(temp_mac, mac, MAC_SIZE) == 0){
		/* Decrypt cipher text */
		sgx_aes_ctr_decrypt(&gsk, (uint8_t *)cipher, key_len+val_len, temp_nac, NAC_SIZE, temp_plain);
		/* Copy results to outside of enclave */
		plain = (char*)temp_plain;
	}
	/* When MAC is not same */
	else{
		print("MAC matching failed");
	}

	free(temp_all);

}

/** Handling Set operation **/
void enclave_set(char *cipher){ 
	char* key;
	char* val;
	char* key_val;
	char* plain_key_val = NULL;
	uint8_t nac[NAC_SIZE];
	uint8_t mac[MAC_SIZE];
	uint8_t updated_nac[NAC_SIZE];

	uint32_t key_size;
	uint32_t val_size;

	uint8_t key_idx;

	char *tok;
	char *temp_;
	entry * ret_entry;

	tok = strtok_r(cipher+4," ",&temp_);
	key_size = strlen(tok)+1;
	key = (char*)malloc(sizeof(char)*key_size);
	memset(key, 0, key_size);
	memcpy(key, tok, key_size-1);

	val_size = strlen(temp_)+1;
	val = (char*)malloc(sizeof(char)*val_size);
	memset(val, 0, val_size);
	memcpy(val, temp_, val_size-1);

	int kv_pos = 0;
	ret_entry = ht_get_o(key, key_size, &plain_key_val, &kv_pos, updated_nac);

	int hash_val = ht_hash(key);
	key_idx = key_hash_func(key);

	sgx_status_t ret = SGX_SUCCESS;

	/* update */
	if(ret_entry != NULL)
	{
		ret = enclave_verification(ret_entry->key_val, ret_entry->key_hash, ret_entry->key_size, ret_entry->val_size, ret_entry->nac, ret_entry->mac);

		if(ret != SGX_SUCCESS)
		{
			print("MAC verification failed");
		}

		ret = enclave_verify_tree_root(hash_val);

		if(ret != SGX_SUCCESS)
		{
			print("Tree verification failed");
		}

		memcpy(nac, updated_nac, NAC_SIZE);
		free(plain_key_val);
	}
	else
	{
		/* Make initial nac */
		sgx_read_rand(nac, NAC_SIZE);
		assert(plain_key_val == NULL);
	}

	/* We have to encrypt key and value together, so make key_val field */
	key_val = (char*)malloc(sizeof(char)*(key_size+val_size));
	memcpy(key_val, key, key_size);
	memcpy(key_val + key_size, val, val_size);


	/* At this point, key_val field will be plain text, so it should be encrypted */
	enclave_encrypt(key_val, key_val, key_idx, key_size, val_size, nac, mac);

	/* Store key, con and mac together on the hash table */
	ht_set_o(ret_entry, key, key_val, nac, mac, key_size, val_size, kv_pos);

	enclave_rebuild_tree_root(hash_val);

	memset(cipher, 0, BUF_SIZE);
	memcpy(cipher, key, key_size);

	free(key);
	free(val);
	free(key_val);
}

/** Handling Get operation **/
void enclave_get(char *cipher){ 

	char* key;
	char* plain_key_val = NULL;

	uint32_t key_size;

	char *tok;
	char *temp_;
	entry * ret_entry;

	uint8_t updated_nac[NAC_SIZE];

	//parsing key
	tok = strtok_r(cipher+4," ",&temp_);
	key_size = strlen(tok)+1;
	key = (char*)malloc(sizeof(char)*key_size);
	memset(key, 0, key_size);
	memcpy(key, tok, key_size-1);

	int kv_pos = 0;
	ret_entry = ht_get_o(key, key_size, &plain_key_val, &kv_pos, updated_nac);

	if(ret_entry == NULL){
		print("GET FAILED: No data in database");
		return;
	}

	/* Have to verify the MAC and tree */
	int hash_val = ht_hash(key);  
	sgx_status_t ret = SGX_SUCCESS;

	ret = enclave_verification(ret_entry->key_val, ret_entry->key_hash, ret_entry->key_size, ret_entry->val_size, ret_entry->nac, ret_entry->mac);

	if(ret != SGX_SUCCESS)
	{
		print("MAC verification failed");
	}

	ret = enclave_verify_tree_root(hash_val);

	if(ret != SGX_SUCCESS)
	{
		print("Tree verification failed");
	}
	
	//	memset(cipher, 0, BUF_SIZE);
	//	memcpy(cipher, plain_key_val+ret_entry->key_size, ret_entry->val_size);
	
	free(key);
	free(plain_key_val);

}

/** Handling Append operation **/
void enclave_append(char *cipher){ 
	char* key;
	char* val;
	char* key_val;
	char* plain_key_val;
	uint8_t nac[NAC_SIZE];
	uint8_t mac[MAC_SIZE];
	uint8_t updated_nac[NAC_SIZE];

	uint32_t key_size;
	uint32_t val_size;

	uint8_t key_idx;

	char *tok;
	char *temp_;
	entry * ret_entry;

	tok = strtok_r(cipher+4," ",&temp_);
	key_size = strlen(tok)+1;
	key = (char*)malloc(sizeof(char)*key_size);
	memset(key, 0, key_size);
	memcpy(key, tok, key_size-1);

	val_size = strlen(temp_)+1;
	val = (char*)malloc(sizeof(char)*val_size);
	memset(val, 0, val_size);
	memcpy(val, temp_, val_size-1);

	int kv_pos = 0;
	ret_entry = ht_get_o(key, key_size, &plain_key_val, &kv_pos, updated_nac);

	int hash_val = ht_hash(key);
	key_idx = key_hash_func(key);

	sgx_status_t ret = SGX_SUCCESS;

	/* update */
	if(ret_entry != NULL)
	{
	
		ret = enclave_verification(ret_entry->key_val, ret_entry->key_hash, ret_entry->key_size, ret_entry->val_size, ret_entry->nac, ret_entry->mac);

		if(ret != SGX_SUCCESS)
		{
			print("MAC verification failed");
		}

		ret = enclave_verify_tree_root(hash_val);

		if(ret != SGX_SUCCESS)
		{
			print("set");
			print("Tree verification failed");
		}

		memcpy(nac, updated_nac, NAC_SIZE);
	}
	else
	{
		print("There's no data in the database");
		return;
	}

	/** Make appended key-value **/
	key_val = (char*)malloc(sizeof(char)*(ret_entry->key_size + ret_entry->val_size + val_size - 1));
	memcpy(key_val, plain_key_val, ret_entry->key_size + ret_entry->val_size);
	memcpy(key_val + ret_entry->key_size + ret_entry->val_size - 1, val, val_size);

	/* At this point, key_val field will be plain text, so it should be encrypted */
	enclave_encrypt(key_val, key_val, key_idx, key_size, ret_entry->val_size + val_size - 1, nac, mac);

	/* Store key, con and mac together on the hash table */
	ht_append_o(ret_entry, key, key_val, nac, mac, key_size, ret_entry->val_size + val_size - 1, kv_pos);

	enclave_rebuild_tree_root(hash_val);

	free(key);
	free(val);
	free(plain_key_val);
	free(key_val);
}


void enclave_message_pass(void* data){
	EcallParams *ecallParams = (EcallParams *) data;

	ht_enclave = ecallParams->ht_;
	MACbuf_enclave = ecallParams->MACbuf_;

	/** Get operation **/
	if(strncmp(ecallParams->buf, "GET", 3) == 0 || strncmp(ecallParams->buf, "get", 3) == 0){
		enclave_get(ecallParams->buf);
	}
	/** Set operation **/
	else if(strncmp(ecallParams->buf, "SET", 3) == 0 || strncmp(ecallParams->buf, "set", 3) == 0){
		enclave_set(ecallParams->buf);
	}
	/** Append operation **/
	else if(strncmp(ecallParams->buf, "APP", 3) == 0 || strncmp(ecallParams->buf, "app", 3) == 0) {
		enclave_append(ecallParams->buf);
	}
	else if(strncmp(ecallParams->buf, "EXIT", 4) == 0 || strncmp(ecallParams->buf, "exit", 4) == 0){
		memset(ecallParams->buf, 0, BUF_SIZE);
		memcpy(ecallParams->buf, "EXIT", 5);
	}
	else if(strncmp(ecallParams->buf, "LOADDONE", 8) == 0) {
		dummy();
		print("Load process is done");
	}
	else{
		print("Invalid command");
		return;
	}
}

void EcallStartResponder( HotCall* hotEcall )
{
	void (*callbacks[1])(void*);
	callbacks[0] = enclave_message_pass;

	HotCallTable callTable;
	callTable.numEntries = 1;
	callTable.callbacks  = callbacks;

	HotCall_waitForCall( hotEcall, &callTable );
}
