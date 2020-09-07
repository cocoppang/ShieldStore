#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_ht_create_t {
	int ms_size;
} ms_ht_create_t;

typedef struct ms_enclave_process_t {
	int* ms_count;
	char*** ms_inst;
	int* ms_count2;
	char*** ms_inst2;
} ms_enclave_process_t;

typedef struct ms_ht_destroy_t {
	int ms_size;
} ms_ht_destroy_t;

typedef struct ms_ecall_lib_initialize_t {
	int ms_retval;
	void* ms_pool_ptr;
	size_t ms_pool_size;
	void* ms_queue;
	unsigned char** ms_ptr_to_pin;
	unsigned long int* ms_size_to_pin;
	unsigned long long* ms_untrusted_counters;
} ms_ecall_lib_initialize_t;

typedef struct ms_ecall_erase_aptr_pcache_t {
	int ms_num_entries;
} ms_ecall_erase_aptr_pcache_t;

typedef struct ms_print_t {
	char* ms_string;
} ms_print_t;

typedef struct ms_print_int_t {
	int ms_d;
} ms_print_int_t;


typedef struct ms_sgx_oc_cpuidex_t {
	int* ms_cpuinfo;
	int ms_leaf;
	int ms_subleaf;
} ms_sgx_oc_cpuidex_t;

typedef struct ms_sgx_thread_wait_untrusted_event_ocall_t {
	int ms_retval;
	void* ms_self;
} ms_sgx_thread_wait_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_set_untrusted_event_ocall_t {
	int ms_retval;
	void* ms_waiter;
} ms_sgx_thread_set_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_setwait_untrusted_events_ocall_t {
	int ms_retval;
	void* ms_waiter;
	void* ms_self;
} ms_sgx_thread_setwait_untrusted_events_ocall_t;

typedef struct ms_sgx_thread_set_multiple_untrusted_events_ocall_t {
	int ms_retval;
	void** ms_waiters;
	size_t ms_total;
} ms_sgx_thread_set_multiple_untrusted_events_ocall_t;

typedef struct ms_ocall_debug_t {
	char* ms_str;
} ms_ocall_debug_t;


static sgx_status_t SGX_CDECL Enclave_print(void* pms)
{
	ms_print_t* ms = SGX_CAST(ms_print_t*, pms);
	print((const char*)ms->ms_string);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_print_int(void* pms)
{
	ms_print_int_t* ms = SGX_CAST(ms_print_int_t*, pms);
	print_int(ms->ms_d);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_dummy(void* pms)
{
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	dummy();
	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_oc_cpuidex(void* pms)
{
	ms_sgx_oc_cpuidex_t* ms = SGX_CAST(ms_sgx_oc_cpuidex_t*, pms);
	sgx_oc_cpuidex(ms->ms_cpuinfo, ms->ms_leaf, ms->ms_subleaf);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_wait_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_wait_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_wait_untrusted_event_ocall((const void*)ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_set_untrusted_event_ocall(void* pms)
{
	ms_sgx_thread_set_untrusted_event_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_untrusted_event_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_untrusted_event_ocall((const void*)ms->ms_waiter);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_setwait_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_setwait_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_setwait_untrusted_events_ocall((const void*)ms->ms_waiter, (const void*)ms->ms_self);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_sgx_thread_set_multiple_untrusted_events_ocall(void* pms)
{
	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = SGX_CAST(ms_sgx_thread_set_multiple_untrusted_events_ocall_t*, pms);
	ms->ms_retval = sgx_thread_set_multiple_untrusted_events_ocall((const void**)ms->ms_waiters, ms->ms_total);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_debug(void* pms)
{
	ms_ocall_debug_t* ms = SGX_CAST(ms_ocall_debug_t*, pms);
	ocall_debug((const char*)ms->ms_str);

	return SGX_SUCCESS;
}

static sgx_status_t SGX_CDECL Enclave_ocall_create_swap_thread(void* pms)
{
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	ocall_create_swap_thread();
	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[10];
} ocall_table_Enclave = {
	10,
	{
		(void*)Enclave_print,
		(void*)Enclave_print_int,
		(void*)Enclave_dummy,
		(void*)Enclave_sgx_oc_cpuidex,
		(void*)Enclave_sgx_thread_wait_untrusted_event_ocall,
		(void*)Enclave_sgx_thread_set_untrusted_event_ocall,
		(void*)Enclave_sgx_thread_setwait_untrusted_events_ocall,
		(void*)Enclave_sgx_thread_set_multiple_untrusted_events_ocall,
		(void*)Enclave_ocall_debug,
		(void*)Enclave_ocall_create_swap_thread,
	}
};
sgx_status_t ht_create(sgx_enclave_id_t eid, int size)
{
	sgx_status_t status;
	ms_ht_create_t ms;
	ms.ms_size = size;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t enclave_process(sgx_enclave_id_t eid, int count[8], char** inst[8], int count2[8], char** inst2[8])
{
	sgx_status_t status;
	ms_enclave_process_t ms;
	ms.ms_count = (int*)count;
	ms.ms_inst = (char***)inst;
	ms.ms_count2 = (int*)count2;
	ms.ms_inst2 = (char***)inst2;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ht_destroy(sgx_enclave_id_t eid, int size)
{
	sgx_status_t status;
	ms_ht_destroy_t ms;
	ms.ms_size = size;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_lib_initialize(sgx_enclave_id_t eid, int* retval, void* pool_ptr, size_t pool_size, void* queue, unsigned char** ptr_to_pin, unsigned long int* size_to_pin, unsigned long long* untrusted_counters)
{
	sgx_status_t status;
	ms_ecall_lib_initialize_t ms;
	ms.ms_pool_ptr = pool_ptr;
	ms.ms_pool_size = pool_size;
	ms.ms_queue = queue;
	ms.ms_ptr_to_pin = ptr_to_pin;
	ms.ms_size_to_pin = size_to_pin;
	ms.ms_untrusted_counters = untrusted_counters;
	status = sgx_ecall(eid, 3, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

sgx_status_t ecall_erase_aptr_pcache(sgx_enclave_id_t eid, int num_entries)
{
	sgx_status_t status;
	ms_ecall_erase_aptr_pcache_t ms;
	ms.ms_num_entries = num_entries;
	status = sgx_ecall(eid, 4, &ocall_table_Enclave, &ms);
	return status;
}

