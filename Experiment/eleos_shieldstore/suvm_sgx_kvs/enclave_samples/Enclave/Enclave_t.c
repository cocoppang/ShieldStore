#include "Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */

#include <errno.h>
#include <string.h> /* for memcpy etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)


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


static sgx_status_t SGX_CDECL sgx_ht_create(void* pms)
{
	ms_ht_create_t* ms = SGX_CAST(ms_ht_create_t*, pms);
	sgx_status_t status = SGX_SUCCESS;

	CHECK_REF_POINTER(pms, sizeof(ms_ht_create_t));

	ht_create(ms->ms_size);


	return status;
}

static sgx_status_t SGX_CDECL sgx_enclave_process(void* pms)
{
	ms_enclave_process_t* ms = SGX_CAST(ms_enclave_process_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	int* _tmp_count = ms->ms_count;
	size_t _len_count = 8 * sizeof(*_tmp_count);
	int* _in_count = NULL;
	char*** _tmp_inst = ms->ms_inst;
	size_t _len_inst = 8 * sizeof(*_tmp_inst);
	char*** _in_inst = NULL;
	int* _tmp_count2 = ms->ms_count2;
	size_t _len_count2 = 8 * sizeof(*_tmp_count2);
	int* _in_count2 = NULL;
	char*** _tmp_inst2 = ms->ms_inst2;
	size_t _len_inst2 = 8 * sizeof(*_tmp_inst2);
	char*** _in_inst2 = NULL;

	CHECK_REF_POINTER(pms, sizeof(ms_enclave_process_t));
	CHECK_UNIQUE_POINTER(_tmp_count, _len_count);
	CHECK_UNIQUE_POINTER(_tmp_inst, _len_inst);
	CHECK_UNIQUE_POINTER(_tmp_count2, _len_count2);
	CHECK_UNIQUE_POINTER(_tmp_inst2, _len_inst2);

	if (_tmp_count != NULL) {
		_in_count = (int*)malloc(_len_count);
		if (_in_count == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_count, _tmp_count, _len_count);
	}
	if (_tmp_inst != NULL) {
		_in_inst = (char***)malloc(_len_inst);
		if (_in_inst == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_inst, _tmp_inst, _len_inst);
	}
	if (_tmp_count2 != NULL) {
		_in_count2 = (int*)malloc(_len_count2);
		if (_in_count2 == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_count2, _tmp_count2, _len_count2);
	}
	if (_tmp_inst2 != NULL) {
		_in_inst2 = (char***)malloc(_len_inst2);
		if (_in_inst2 == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memcpy(_in_inst2, _tmp_inst2, _len_inst2);
	}
	enclave_process(_in_count, _in_inst, _in_count2, _in_inst2);
err:
	if (_in_count) free(_in_count);
	if (_in_inst) free(_in_inst);
	if (_in_count2) free(_in_count2);
	if (_in_inst2) free(_in_inst2);

	return status;
}

static sgx_status_t SGX_CDECL sgx_ht_destroy(void* pms)
{
	ms_ht_destroy_t* ms = SGX_CAST(ms_ht_destroy_t*, pms);
	sgx_status_t status = SGX_SUCCESS;

	CHECK_REF_POINTER(pms, sizeof(ms_ht_destroy_t));

	ht_destroy(ms->ms_size);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_lib_initialize(void* pms)
{
	ms_ecall_lib_initialize_t* ms = SGX_CAST(ms_ecall_lib_initialize_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	void* _tmp_pool_ptr = ms->ms_pool_ptr;
	void* _tmp_queue = ms->ms_queue;
	unsigned char** _tmp_ptr_to_pin = ms->ms_ptr_to_pin;
	unsigned long int* _tmp_size_to_pin = ms->ms_size_to_pin;
	unsigned long long* _tmp_untrusted_counters = ms->ms_untrusted_counters;

	CHECK_REF_POINTER(pms, sizeof(ms_ecall_lib_initialize_t));

	ms->ms_retval = ecall_lib_initialize(_tmp_pool_ptr, ms->ms_pool_size, _tmp_queue, _tmp_ptr_to_pin, _tmp_size_to_pin, _tmp_untrusted_counters);


	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_erase_aptr_pcache(void* pms)
{
	ms_ecall_erase_aptr_pcache_t* ms = SGX_CAST(ms_ecall_erase_aptr_pcache_t*, pms);
	sgx_status_t status = SGX_SUCCESS;

	CHECK_REF_POINTER(pms, sizeof(ms_ecall_erase_aptr_pcache_t));

	ecall_erase_aptr_pcache(ms->ms_num_entries);


	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv;} ecall_table[5];
} g_ecall_table = {
	5,
	{
		{(void*)(uintptr_t)sgx_ht_create, 0},
		{(void*)(uintptr_t)sgx_enclave_process, 0},
		{(void*)(uintptr_t)sgx_ht_destroy, 0},
		{(void*)(uintptr_t)sgx_ecall_lib_initialize, 0},
		{(void*)(uintptr_t)sgx_ecall_erase_aptr_pcache, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[10][5];
} g_dyn_entry_table = {
	10,
	{
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
		{0, 0, 0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL print(const char* string)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_string = string ? strlen(string) + 1 : 0;

	ms_print_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_print_t);
	void *__tmp = NULL;

	ocalloc_size += (string != NULL && sgx_is_within_enclave(string, _len_string)) ? _len_string : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_print_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_print_t));

	if (string != NULL && sgx_is_within_enclave(string, _len_string)) {
		ms->ms_string = (char*)__tmp;
		__tmp = (void *)((size_t)__tmp + _len_string);
		memcpy((void*)ms->ms_string, string, _len_string);
	} else if (string == NULL) {
		ms->ms_string = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	status = sgx_ocall(0, ms);


	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL print_int(int d)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_print_int_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_print_int_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_print_int_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_print_int_t));

	ms->ms_d = d;
	status = sgx_ocall(1, ms);


	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL dummy()
{
	sgx_status_t status = SGX_SUCCESS;
	status = sgx_ocall(2, NULL);

	return status;
}

sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_cpuinfo = 4 * sizeof(*cpuinfo);

	ms_sgx_oc_cpuidex_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_oc_cpuidex_t);
	void *__tmp = NULL;

	ocalloc_size += (cpuinfo != NULL && sgx_is_within_enclave(cpuinfo, _len_cpuinfo)) ? _len_cpuinfo : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_oc_cpuidex_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_oc_cpuidex_t));

	if (cpuinfo != NULL && sgx_is_within_enclave(cpuinfo, _len_cpuinfo)) {
		ms->ms_cpuinfo = (int*)__tmp;
		__tmp = (void *)((size_t)__tmp + _len_cpuinfo);
		memcpy(ms->ms_cpuinfo, cpuinfo, _len_cpuinfo);
	} else if (cpuinfo == NULL) {
		ms->ms_cpuinfo = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_leaf = leaf;
	ms->ms_subleaf = subleaf;
	status = sgx_ocall(3, ms);

	if (cpuinfo) memcpy((void*)cpuinfo, ms->ms_cpuinfo, _len_cpuinfo);

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_wait_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t));

	ms->ms_self = SGX_CAST(void*, self);
	status = sgx_ocall(4, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_set_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_untrusted_event_ocall_t));

	ms->ms_waiter = SGX_CAST(void*, waiter);
	status = sgx_ocall(5, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_setwait_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t));

	ms->ms_waiter = SGX_CAST(void*, waiter);
	ms->ms_self = SGX_CAST(void*, self);
	status = sgx_ocall(6, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_waiters = total * sizeof(*waiters);

	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);
	void *__tmp = NULL;

	ocalloc_size += (waiters != NULL && sgx_is_within_enclave(waiters, _len_waiters)) ? _len_waiters : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_multiple_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t));

	if (waiters != NULL && sgx_is_within_enclave(waiters, _len_waiters)) {
		ms->ms_waiters = (void**)__tmp;
		__tmp = (void *)((size_t)__tmp + _len_waiters);
		memcpy((void*)ms->ms_waiters, waiters, _len_waiters);
	} else if (waiters == NULL) {
		ms->ms_waiters = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	ms->ms_total = total;
	status = sgx_ocall(7, ms);

	if (retval) *retval = ms->ms_retval;

	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_debug(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_ocall_debug_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_debug_t);
	void *__tmp = NULL;

	ocalloc_size += (str != NULL && sgx_is_within_enclave(str, _len_str)) ? _len_str : 0;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_debug_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_debug_t));

	if (str != NULL && sgx_is_within_enclave(str, _len_str)) {
		ms->ms_str = (char*)__tmp;
		__tmp = (void *)((size_t)__tmp + _len_str);
		memcpy((void*)ms->ms_str, str, _len_str);
	} else if (str == NULL) {
		ms->ms_str = NULL;
	} else {
		sgx_ocfree();
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	status = sgx_ocall(8, ms);


	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL ocall_create_swap_thread()
{
	sgx_status_t status = SGX_SUCCESS;
	status = sgx_ocall(9, NULL);

	return status;
}

