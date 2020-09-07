#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_satus_t etc. */

#include "sgx_thread.h"
#include "common.h"

#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

void SGX_UBRIDGE(SGX_NOCONVENTION, print, (const char* string));
void SGX_UBRIDGE(SGX_NOCONVENTION, print_int, (int d));
void SGX_UBRIDGE(SGX_NOCONVENTION, dummy, ());
void SGX_UBRIDGE(SGX_CDECL, sgx_oc_cpuidex, (int cpuinfo[4], int leaf, int subleaf));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_wait_untrusted_event_ocall, (const void* self));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_untrusted_event_ocall, (const void* waiter));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_setwait_untrusted_events_ocall, (const void* waiter, const void* self));
int SGX_UBRIDGE(SGX_CDECL, sgx_thread_set_multiple_untrusted_events_ocall, (const void** waiters, size_t total));
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_debug, (const char* str));
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_create_swap_thread, ());

sgx_status_t ht_create(sgx_enclave_id_t eid, int size);
sgx_status_t enclave_process(sgx_enclave_id_t eid, int count[8], char** inst[8], int count2[8], char** inst2[8]);
sgx_status_t ht_destroy(sgx_enclave_id_t eid, int size);
sgx_status_t ecall_lib_initialize(sgx_enclave_id_t eid, int* retval, void* pool_ptr, size_t pool_size, void* queue, unsigned char** ptr_to_pin, unsigned long int* size_to_pin, unsigned long long* untrusted_counters);
sgx_status_t ecall_erase_aptr_pcache(sgx_enclave_id_t eid, int num_entries);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
