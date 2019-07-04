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

/* hash table */
extern hashtable *ht_enclave;
/* MAC buffer */
extern MACbuffer *MACbuf_enclave;

extern int ratio_root_per_buckets;

struct _bucketMAC{
	uint8_t mac[MAC_SIZE];
};
typedef _bucketMAC BucketMAC;

extern BucketMAC *MACTable;
extern Arg arg_enclave;

#endif
