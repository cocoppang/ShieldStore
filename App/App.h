#ifndef APP_H_
#define APP_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>

#include "Enclave_u.h"
#include "sgx_urts.h"

#include "Sealing/ReplayProtectedDRM.h"

#define CHILD_EXIT 222

void help();
void parse_option();
int setNonblocking(int fd);
void configuration_init();
MACbuffer * macbuffer_create(int size);
hashtable * ht_create(int size);
sgx_status_t load_and_initialize_enclave(sgx_enclave_id_t *eid);
void *load_and_initialize_threads(void *temp);
void* EnclaveResponderThread(void* hotEcallAsVoidP);

/** OCALLS **/
void message_return(char* ret, size_t ret_size, int client_sock);
void* sbrk_o(size_t size);
void print(const char *str);

/** Persistent **/
uint32_t sealing_initialization();
void put_sealed_log_in_file();
void persistent_process();

extern hashtable *ht;
extern ReplayProtectedDRM *DRM;
#endif
