#ifndef ENCLAVE_T_H__
#define ENCLAVE_T_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include "sgx_edger8r.h" /* for sgx_ocall etc. */

#include "sgx_thread.h"
#include "common.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif


void ht_create(int size);
void enclave_process(int count[8], char** inst[8], int count2[8], char** inst2[8]);
void ht_destroy(int size);
int ecall_lib_initialize(void* pool_ptr, size_t pool_size, void* queue, unsigned char** ptr_to_pin, unsigned long int* size_to_pin, unsigned long long* untrusted_counters);
void ecall_erase_aptr_pcache(int num_entries);

sgx_status_t SGX_CDECL print(const char* string);
sgx_status_t SGX_CDECL print_int(int d);
sgx_status_t SGX_CDECL dummy();
sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf);
sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter);
sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self);
sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total);
sgx_status_t SGX_CDECL ocall_debug(const char* str);
sgx_status_t SGX_CDECL ocall_create_swap_thread();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
