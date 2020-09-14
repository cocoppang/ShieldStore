#include <string.h>
#include <assert.h>
#include <limits.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>

#include "Enclave_u.h"
#include "sgx_urts.h"
#include "ErrorSupport.h"

#define ENCLAVE_NAME "libenclave.signed.so"
#define TOKEN_NAME "Enclave.token"

//For mmap
#include <sys/mman.h>
#include <fcntl.h>

using namespace std;

// Global data
sgx_enclave_id_t global_eid = 0;
sgx_launch_token_t token = {0};

int g_size;

/* OCALL Function */
void print(const char *str){
	printf("%s\n", str);
}

void print_int(int num) {
	printf("%d\n", num);
}

static char **inst = NULL;
static int count;

static char **inst2 = NULL;
static int count2;

time_t start1, end1, mid1;

void dummy() {
	mid1 = time(0);
}

// To load and initialize the enclave     
sgx_status_t load_and_initialize_enclave(sgx_enclave_id_t *eid){
    sgx_status_t ret = SGX_SUCCESS;
    int retval = 0;
    int updated = 0;

	// Step 1: check whether the loading and initialization operations are caused
	if(*eid != 0)
		sgx_destroy_enclave(*eid);

	// Step 2: load the enclave
	ret = sgx_create_enclave(ENCLAVE_NAME, SGX_DEBUG_FLAG, &token, &updated, eid, NULL);
	if(ret != SGX_SUCCESS)
		return ret;

	// Save the launch token if updated
	if(updated == 1){
		ofstream ofs(TOKEN_NAME, std::ios::binary|std::ios::out);
		if(!ofs.good())
			cout<< "Warning: Failed to save the launch token to \"" <<TOKEN_NAME <<"\""<<endl;
		else
			ofs << token;
	}
	return ret;
}

int _hash(char *key){
	unsigned long int hashval = 7;
	int i = 0;

	/* Convert our string to an integer */
	while(hashval < ULONG_MAX && i < strlen(key)){
		hashval = hashval * 61;
		hashval += key[i];
		i++;
	}

	return hashval % g_size;
}

void file_to_memory(char* workload){
	/* for mmap */
	FILE *f;
	int j;

	/* Listening commands */
	char buf[BUF_SIZE];
	char filename[30];

	int set_size = 16384*atoi(workload);
	print_int(set_size);
	int set_count = 1;
	int set_size2 = 10000000;
	int set_count2 = 1;

	/* For load workload */
	count = 0;
	inst = (char**)malloc(sizeof(char*)*set_size);
	for(j = 0; j < set_size; j++)
		inst[j] = (char*)malloc(sizeof(char)*BUF_SIZE);
	
	/* For run workload */
	count2 = 0;
	inst2 = (char**)malloc(sizeof(char*)*set_size2);
	for(j = 0; j < set_size2; j++)
		inst2[j] = (char*)malloc(sizeof(char)*BUF_SIZE);

	sprintf(filename, "../workloads/load_%s.txt", workload);	
	printf("%s\n",filename);
	f = fopen(filename, "r");
	while(set_count <= set_size)
	{
		fgets(buf, BUF_SIZE, f);
		memcpy(inst[count++], buf, BUF_SIZE);

		set_count++;
	}
	fclose(f);

	sprintf(filename, "../workloads/run_%s.txt", workload);	
	printf("%s\n",filename);
	f = fopen(filename, "r");	
	while(set_count2 <= set_size2)
	{
		fgets(buf, BUF_SIZE, f);
		memcpy(inst2[count2++], buf, BUF_SIZE);

		set_count2++;
	}
	fclose(f);
}

int main(int argc, char **argv){
	g_size = 1048576;

	char buf[BUF_SIZE];
	FILE *f;

	// Load and initialize the signed enclave
	sgx_status_t ret = load_and_initialize_enclave(&global_eid);

	if(ret != SGX_SUCCESS){
		ret_error_support(ret);
		return -1;
	}

	/* Make main hash table */
	if((ret = ht_create(global_eid, g_size)) != SGX_SUCCESS) {
		ret_error_support(ret);
		return -1;
	}
	printf("HT is created!\n");

	if(argc != 2) {
		printf("Usage: ./app [workload] : workload <-- 16, 32, 48, ... , 4096\n");
		exit(0);
	}

	/* Load files into memory */
	file_to_memory(argv[1]);

	start1 = time(0);
	enclave_process(global_eid, count*8, inst, count2*8, inst2);
	end1 = time(0);

	f = fopen("result.txt", "a");
	fprintf(f,"Thread: 1 Workload: %sMB Execution time for Running : %f\n", argv[1], difftime(end1,mid1));
	fclose(f);

	if((ret = ht_destroy(global_eid, g_size)) != SGX_SUCCESS) {
		ret_error_support(ret);
		return -1;
	}

	// Destroy the enclave
	sgx_destroy_enclave(global_eid);

	return 0;
}
