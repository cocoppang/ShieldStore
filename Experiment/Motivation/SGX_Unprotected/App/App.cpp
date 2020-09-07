#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>

#include "sgx_urts.h"
#include "Enclave_u.h"
#include "ErrorSupport.h"
#include "userdef.h"

#define ENCLAVE_NAME "libenclave.signed.so"
#define TOKEN_NAME "Enclave.token"

using namespace std;

// Global data
sgx_enclave_id_t global_eid = 0;
sgx_launch_token_t token = {0};

struct region *page;
int value = 1;

/* OCALL Function */
void print(const char *string) {
	printf("%s\n", string);
}
void print_int(int d) {
	printf("%d\n", d);
}

// load_and_initialize_enclave():
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

void write_o(int i, int size) {
	memcpy(page->addr+i, &value, size);
}

int main(int argc, char **argv){
    // Load and initialize the signed enclave
    sgx_status_t ret = load_and_initialize_enclave(&global_eid);

    if(ret != SGX_SUCCESS){
        ret_error_support(ret);
        return -1;
    }

	struct timeval start, end;
	double diff = 0;

	page= (struct region *) malloc(sizeof(struct region));
	if(!page) print("Region is not created!\n");

	int i;
	for (i = 0; i < FOOTPRINT; i++) {
		page->addr[i] = 1;
	}

	gettimeofday(&start, NULL);
	region_read(global_eid,&page,16);
	gettimeofday(&end, NULL);

	free(page);

	diff = (double)(end.tv_usec - start.tv_usec) / 1000000 + (double)(end.tv_sec - start.tv_sec);

	FILE *f = fopen("result.txt", "a");
	fprintf(f, "%lf sec\n", diff);
	fclose(f);

	// Destroy the enclave
    sgx_destroy_enclave(global_eid);

    return 0;
}

