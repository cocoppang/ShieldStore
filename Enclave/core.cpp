#include "Enclave.h"

/* Global Symmetric Key */
const sgx_ec_key_128bit_t gsk = {
	0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
	0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad
};

/**
 * decrypt key on the hash structure and compare to matching key
 */
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

/**
 * Decrypt the encrypted key_val and get the key
 * TODO: it doesn't update the IV/counter value(nac)
 **/
char* decrypt_and_get_key(char* cipher, uint32_t key_len, uint32_t val_len, uint8_t *nac) {
	uint8_t* temp_plain = NULL;
	uint8_t* temp_key = NULL;
	
	//Do not update the IV/counter
	uint8_t temp_nac[NAC_SIZE];
	sgx_status_t ret = SGX_SUCCESS;

	memcpy(temp_nac, nac, NAC_SIZE);

	temp_plain = (uint8_t*)malloc(sizeof(uint8_t)*(key_len+val_len));
	temp_key = (uint8_t*)malloc(sizeof(uint8_t)*(key_len));

	ret = sgx_aes_ctr_decrypt(&gsk, (uint8_t*)cipher, key_len+val_len, temp_nac, NAC_SIZE, temp_plain);
	
	if(ret != SGX_SUCCESS)
	{
		assert(0);
	}

	memcpy(temp_key, temp_plain, key_len);
	free(temp_plain);

	return (char*)temp_key;
}

/**
 * get all the mac entry for a same hash bucket
 * for integrity verification
 **/
void get_chain_mac(int hash_val,  uint8_t *mac){

	uint8_t temp_mac[MAC_SIZE];
	uint8_t* aggregate_mac;
	entry *pair;

	int count = 0;
	int i;
	int aggregate_mac_idx = 0;
	int index;
	sgx_status_t ret = SGX_SUCCESS;

	memset(temp_mac, 0, MAC_SIZE);

	/** bucket start index for verifying integrity **/
	int start_index = (int)(hash_val/ratio_root_per_buckets)*ratio_root_per_buckets;

	if(arg_enclave.mac_opt)
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

/**
 * verify integrity tree &
 * update integrity tree with updated hash values
 **/
sgx_status_t enclave_rebuild_tree_root(int hash_val, int kv_pos, bool is_insert, uint8_t* prev_mac)
{

	uint8_t temp_mac[MAC_SIZE];
	uint8_t* aggregate_mac;
	uint8_t* prev_aggregate_mac;
	entry *pair;

	int total_mac_count = 0, prev_mac_count = 0, cur_mac_count = 0;
	int aggregate_mac_idx = 0;
	int i, index, updated_idx = -1;
	bool is_cur_idx = false;

	sgx_status_t ret = SGX_SUCCESS;

	memset(temp_mac, 0, MAC_SIZE);

	/** bucket start index for verifying integrity **/
	int start_index = (int)(hash_val / ratio_root_per_buckets) * ratio_root_per_buckets;

	if (arg_enclave.mac_opt) {
		for (index = start_index; index < start_index + ratio_root_per_buckets; index++) {
			if (index == hash_val) {
				prev_mac_count = total_mac_count;
				/** The location of MAC is the reverse order over the location of key value **/
				if (is_insert)
					updated_idx = prev_mac_count + kv_pos;
				else
					updated_idx = prev_mac_count + (MACbuf_enclave->entry[index].size - kv_pos - 1);
			}
			total_mac_count += MACbuf_enclave->entry[index].size;
		}

		aggregate_mac = (uint8_t *)malloc(MAC_SIZE * total_mac_count);
		memset(aggregate_mac, 0, MAC_SIZE * total_mac_count);
		for (index = start_index; index < start_index + ratio_root_per_buckets; index++) {
			memcpy(aggregate_mac + aggregate_mac_idx, MACbuf_enclave->entry[index].mac,
					MAC_SIZE * MACbuf_enclave->entry[index].size);
			aggregate_mac_idx += (MAC_SIZE * MACbuf_enclave->entry[index].size);
		}
	}
	else {
		/* Check chaining size */
		for (index = start_index; index < start_index + ratio_root_per_buckets; index++) {
			pair = ht_enclave->table[index];
			is_cur_idx = false;
			if (index == hash_val) {
				prev_mac_count = total_mac_count;
				is_cur_idx = true;
			}
			while (pair != NULL) {
				if (is_cur_idx)
					cur_mac_count++;
				total_mac_count++;
				pair = pair->next;
			}
		}
		if (is_insert)
			updated_idx = prev_mac_count + (cur_mac_count - kv_pos - 1);
		else 
			updated_idx = prev_mac_count + kv_pos;

		//verify
		aggregate_mac = (uint8_t *)malloc(MAC_SIZE * total_mac_count);
		memset(aggregate_mac, 0 , MAC_SIZE * total_mac_count);

		i = 0;
		for (index = start_index; index < start_index + ratio_root_per_buckets; index++) {
			pair = ht_enclave->table[index];
			while (pair != NULL) {
				memcpy(aggregate_mac+(MAC_SIZE * i), pair->mac, MAC_SIZE);
				i++;
				pair = pair->next;
			}
		}

	}

	//generate previous mac
	if (is_insert) {
		assert(kv_pos == 0);

		if (total_mac_count > 1) {
			prev_aggregate_mac = (uint8_t *)malloc(MAC_SIZE * (total_mac_count - 1));
			memset(prev_aggregate_mac, 0, MAC_SIZE * (total_mac_count - 1));

			memcpy(prev_aggregate_mac, aggregate_mac, updated_idx * MAC_SIZE);
			memcpy(prev_aggregate_mac + (updated_idx * MAC_SIZE),
					aggregate_mac + (updated_idx + 1) * MAC_SIZE,
					(total_mac_count - updated_idx - 1) * MAC_SIZE);

			//verify tree root using previous mac
			ret = sgx_rijndael128_cmac_msg(&gsk, prev_aggregate_mac,
					MAC_SIZE * (total_mac_count - 1), &temp_mac);
			free(prev_aggregate_mac);

			if (ret != SGX_SUCCESS)
				return SGX_ERROR_UNEXPECTED;
		}
		else {
			//no need to verify
		}
	}
	else {
		prev_aggregate_mac = (uint8_t *)malloc(MAC_SIZE * total_mac_count);
		memset(prev_aggregate_mac, 0, MAC_SIZE * total_mac_count);

		memcpy(prev_aggregate_mac, aggregate_mac, MAC_SIZE * total_mac_count);
		memcpy(prev_aggregate_mac + updated_idx * MAC_SIZE, prev_mac, MAC_SIZE);

		//verify tree root using previous mac
		ret = sgx_rijndael128_cmac_msg(&gsk, prev_aggregate_mac,
				MAC_SIZE * total_mac_count, &temp_mac);
		free(prev_aggregate_mac);

		if (ret != SGX_SUCCESS)
			return SGX_ERROR_UNEXPECTED;
	}

	//If ShieldStore stores at least one entry, we can verify tree root
	if (!is_insert || (is_insert && total_mac_count > 1)) {
		if (memcmp(temp_mac, MACTable[hash_val / ratio_root_per_buckets].mac, MAC_SIZE) != 0) {
			print("Previous MAC compare failed");
			return SGX_ERROR_UNEXPECTED;
		}
	}

	//updated tree root
	ret = sgx_rijndael128_cmac_msg(&gsk, aggregate_mac, MAC_SIZE * total_mac_count, &temp_mac);
	free(aggregate_mac);
	memcpy(MACTable[hash_val/ratio_root_per_buckets].mac , temp_mac, MAC_SIZE);

	return ret;
}

/**
 * verify integrity tree
 **/
sgx_status_t enclave_verify_tree_root(int hash_val){

	uint8_t cur_mac[MAC_SIZE];

	get_chain_mac(hash_val, cur_mac);

	if(memcmp(cur_mac, MACTable[hash_val/ratio_root_per_buckets].mac , MAC_SIZE) != 0)
		return SGX_ERROR_UNEXPECTED;

	return SGX_SUCCESS;
}

/**
 * encrypt and generate MAC value of entry
 **/
void enclave_encrypt(char* key_val, char *cipher, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac){
	uint8_t* temp_cipher;
	uint8_t* temp_all;
	uint8_t temp_nac[NAC_SIZE];
	uint8_t temp_mac[MAC_SIZE];

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

/**
 * verify integrity with MAC value
 **/
sgx_status_t enclave_verification(char *cipher, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac) {
	uint8_t* temp_all;
	uint8_t temp_nac[NAC_SIZE];
	uint8_t temp_mac[MAC_SIZE];

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

/**
 * decrypt entry and check MAC value and entry
 **/
void enclave_decrypt(char *cipher, char *plain, uint8_t key_idx, uint32_t key_len, uint32_t val_len, uint8_t *nac, uint8_t *mac){
	uint8_t* temp_plain;
	uint8_t* temp_all;
	uint8_t temp_nac[NAC_SIZE];
	uint8_t temp_mac[MAC_SIZE];

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
