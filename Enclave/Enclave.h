#ifndef ENCLAVE_H_
#define ENCLAVE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Enclave_t.h"

#include "sgx_trts.h"
#include "sgx_tkey_exchange.h"
#include "sgx_tcrypto.h"
#include "sgx_tseal.h"
#include "sgx_tae_service.h"

#include "gperftools/malloc_hook.h"
#include "gperftools/malloc_extension.h"
#include "gperftools/tcmalloc.h"

#include <climits>
#include <cassert>

#include "sealed_data_defines.h"
#define REPLAY_PROTECTED_SECRET_SIZE  1024

typedef struct _bucketMAC
{
	uint8_t mac[MAC_SIZE];
} BucketMAC;

typedef struct _staging_entry 
{ 
	entry* kv_entry;
	int kv_pos;
	struct _staging_entry* next;
} staging_entry;

typedef struct _staging_hashtable 
{
	int size;
	staging_entry** table;
} staging_hashtable;

typedef struct _activity_log
{
    uint32_t release_version;
    uint32_t max_release_version;
} activity_log;

typedef struct _replay_protected_pay_load
{
    sgx_mc_uuid_t mc;
    uint32_t mc_value;
    uint8_t secret[REPLAY_PROTECTED_SECRET_SIZE];
    activity_log log;
} replay_protected_pay_load;

/** Hash related functions **/
int ht_hash(char *key);
uint8_t key_hash_func(char *key);
char* decrypt_key_val_and_compare(char* key, char* cipher, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *updated_nac);
entry * ht_get_o(char *key, int32_t key_size, char **plain_key_val, int* kv_pos, uint8_t (&updated_nac)[NAC_SIZE]);
entry * ht_newpair(int key_size, int val_size, uint8_t key_hash, char *key_val, uint8_t *nac, uint8_t *mac);
void ht_set_o(entry* updated_entry, char *key, char *key_val, uint8_t *nac, uint8_t *mac, uint32_t key_len, uint32_t val_len, int kv_pos);
void ht_append_o(entry* updated_entry, char *key, char *key_val, uint8_t *nac, uint8_t *mac, uint32_t key_len, uint32_t val_len, int kv_pos);

/** Security core functions **/
void get_chain_mac(int hash_val,  uint8_t *mac);
//void enclave_rebuild_tree_root(int hash_val);
sgx_status_t enclave_rebuild_tree_root(int hash_val, int kv_pos, bool is_insert, uint8_t* mac);
sgx_status_t enclave_verify_tree_root(int hash_val);
void enclave_encrypt(char* key_val, char *cipher, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac);
sgx_status_t enclave_verification(char *cipher, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac);
void enclave_decrypt(char *cipher, char *plain, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac);

/** ShieldStore operations **/
void enclave_set(char *cipher); 
void enclave_get(char *cipher);
void enclave_append(char *cipher);

/** Interface with front-end **/
void enclave_message_pass(void* data);
void EcallStartResponder( HotCall* hotEcall );
void enclave_init_values(hashtable* ht_, MACbuffer* MACbuf_, Arg arg);
void enclave_worker_thread(hashtable *ht_, MACbuffer *MACbuf_);

/** For sealing **/
int ht_staging_hash(char* key);
char* decrypt_and_get_key(char* cipher, uint32_t key_len, uint32_t val_len, uint8_t *nac);
void delete_entry(entry* pair);
void init_staging_hashtable(int size);
void ht_replace_o(entry* replace_entry, char *key, int key_size);
staging_entry* ht_get_staging_buffer_o(char *key, int key_size, int* kv_pos);
void ht_set_staging_buffer_o(staging_entry* ret_entry, char *key, char *key_val, uint8_t *nac, uint8_t *mac, int key_len, int val_len, int kv_pos);
void enclave_persist_done();
void update_staging_buffer_in_kvs();

void init_secret_object();
void update_secret_object();
uint8_t* get_secret_object();

uint32_t create_sealed_policy(uint8_t* sealed_log, uint32_t sealed_log_size );
uint32_t perform_sealed_policy(const uint8_t* sealed_log, uint32_t sealed_log_size);
uint32_t update_sealed_policy(uint8_t* sealed_log, uint32_t sealed_log_size);
uint32_t delete_sealed_policy(const uint8_t* sealed_log, uint32_t sealed_log_size);

/* hash table */
extern hashtable *ht_enclave;
/* MAC buffer */
extern MACbuffer *MACbuf_enclave;
/* secret key */
extern const sgx_ec_key_128bit_t gsk;

extern uint8_t* sealed_secret_object;

extern int ratio_root_per_buckets;
extern BucketMAC *MACTable;
extern Arg arg_enclave;

#endif
