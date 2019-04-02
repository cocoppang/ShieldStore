#include "App.h"
#include "Sealing/ReplayProtectedDRM.h"

#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define SEAL_NAME "SEAL.txt"
#define DATA_FILE_NAME "KVentry.txt"

/**
 * Initialize the sealing mechanism of Intel SGX
 **/
uint32_t sealing_initialization() {
	printf("Sealing Init Start\n");

	uint32_t result = 0;

	result = DRM->init();
 
	if(result)
    {
		std::cerr<<"Initialization the DRM failed."<< std::endl;
        return result;
    }
    else 
        std::cout<<"Successfully initialized the DRM."<< std::endl;
	
	return result;
}

/**
 * Put the sealed meta data inside enclave on storage
 **/
void put_sealed_log_in_file() {
	uint8_t* sealed_log;
	ssize_t err;

	sealed_log = (uint8_t*)malloc(sizeof(uint8_t)*SEALED_REPLAY_PROTECTED_PAY_LOAD_SIZE);
	memcpy(sealed_log, DRM->sealed_activity_log, SEALED_REPLAY_PROTECTED_PAY_LOAD_SIZE);

	int f = open(SEAL_NAME, O_RDWR | O_CREAT, 0644);
	
	if(f == -1) 
		std::cout << "Warning: Failed to save the sealed log to \"" <<SEAL_NAME<<"\""<< std::endl;
	else {
		err = write(f, (char*)sealed_log, SEALED_REPLAY_PROTECTED_PAY_LOAD_SIZE);
		if(err == -1) {
			printf("Error errno:%ld\n", err);
		}
		fsync(f);
	}
	close(f);

	return;
}


/**
 * Persistent process
 * Fork running process and make a file to keep encrypted data
 *
 * Allocate a large chunk of the memory and keep write on the memory chunk unless the chuck size is limited.
 * For example, allocate 4KB memory and keep write of the entry. If writing an entry to chunk overflow the size of 
 * large chunk, write the large chunk in a file and allocate another large chunk.
 **/
void persistent_process() {
	pid_t pid;
	uint8_t* entry_data;
	ssize_t err;

	//Fork the process
	pid = fork();

	if(pid < 0) {
		printf("Error for fork process\n");
	}
	//child process
	else if(pid == 0) {
		//This process store the encrypted key-value entries to the file
		uint8_t* kv_entry = NULL;
		uint8_t* chunk = NULL;
		uint32_t entry_size;
		uint32_t offset = 0;
		int counter = 1;
		entry* pair = NULL;

		int f = open(DATA_FILE_NAME, O_RDWR| O_CREAT, 0644);
		if(f == -1) 
			std::cout << "Warning: Failed to save the KV entires to \"" <<DATA_FILE_NAME<<"\""<< std::endl;
		else {
			lseek(f, 0, SEEK_SET);
			for(int i = 0 ; i < ht->size; i++) {
				pair = ht->table[i];
				while(pair != NULL) {
					entry_size = pair->key_size + pair->val_size + 1 + NAC_SIZE + MAC_SIZE + sizeof(uint32_t)*2;

					if(chunk == NULL || offset + entry_size > PAGE_SIZE) {
						if(chunk != NULL) {
							err = write(f, (char*)chunk, PAGE_SIZE);
							if(err == -1) {
								printf("Error errno:%ld\n", err);
							}
							munmap(chunk, PAGE_SIZE);
							chunk = NULL;
						}

						offset = 0;
						chunk = (uint8_t*)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
					}
					kv_entry = chunk + offset;

					memcpy(kv_entry, &(pair->key_size), sizeof(uint32_t));
					memcpy(kv_entry + sizeof(uint32_t), &(pair->val_size), sizeof(uint32_t));
					memcpy(kv_entry + 2*sizeof(uint32_t), &(pair->key_hash), 1);
					memcpy(kv_entry + 2*sizeof(uint32_t) + 1, pair->key_val, pair->key_size + pair->val_size);
					memcpy(kv_entry + 2*sizeof(uint32_t) + 1 + pair->key_size + pair->val_size, pair->nac, NAC_SIZE);
					memcpy(kv_entry + 2*sizeof(uint32_t) + 1 + pair->key_size + pair->val_size + NAC_SIZE, pair->mac, MAC_SIZE);

					pair = pair->next;
					offset += entry_size;
				}
				
			}
			//remained chunk data should be written to the file
			if(chunk != NULL) {
				err = write(f, (char*)chunk, PAGE_SIZE);
				if(err == -1) {
					printf("Error errno:%ld\n", err);
				}
				munmap(chunk, PAGE_SIZE);
				chunk = NULL;
			}
			fsync(f);
		}

		close(f);
	
		//exit child process
		exit(CHILD_EXIT);
	}
	//parent process
	else { 
		put_sealed_log_in_file();

		printf("Stored sealed data in file\n");
		return;
	}

}

