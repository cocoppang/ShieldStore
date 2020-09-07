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

//For mmap and multi-threading
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

//For integrate on eleos
#include "init_services.h"

using namespace std;

// Global data
sgx_enclave_id_t global_eid = 0;
sgx_launch_token_t token = {0};
int g_size;

/* OCALL Function */
void print(const char *string){
	printf("%s\n", string);
}

void print_int(int d) {
	printf("%d\n", d);
}

static char ***inst = NULL;
static int *count = NULL;

static char ***inst2 = NULL;
static int *count2 = NULL;

time_t start1, end1, mid1;

void dummy() {
	mid1 = time(0);
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

void *load_and_initialize_threads(void *tmp){
	enclave_process(global_eid, count, inst, count2, inst2);
}

void file_to_memory(char workload){
	/* for mmap */ char *tok;
	FILE *fp;
	char *temp_;
	char key[KEY_SIZE];
	int i, j;

	/* Listening commands */
	char buf[BUF_SIZE];
	char buf2[BUF_SIZE];

	int thread_id;

	char filename[70];

	
	int set_size = 125000;

	if(VAL_SIZE == 513)
		set_size = 700000;
	else if(VAL_SIZE == 1025)
		set_size = 400000;
	else {
		set_size = 50000;
	}

	printf("value size : %d set_size = %d\n", VAL_SIZE, set_size);

	//For test
	//int set_size = 1000000;
	int set_count = 1;
	//int set_size2 = 30000000;
	int set_size2 = 10000000;
	int set_count2 = 1;

//	if(VAL_SIZE == 513)
//		set_size2 = 10000000;
//	else if(VAL_SIZE == 1025)
//		set_size2 = 1000000;
//	else {
//		set_size2 = 30000000;
		//For test
		//set_size2 = 2000000;
	//}

	/* For load workload */
	count = (int*)malloc(sizeof(int)*THREAD_NUM);
	inst = (char***)malloc(sizeof(char**)*THREAD_NUM);
	for(i = 0; i < THREAD_NUM; i++)
	{
		count[i] = 0;
		inst[i] = (char**)malloc(sizeof(char*)*set_size);
		for(j = 0; j < set_size; j++)
			inst[i][j] = (char*)malloc(sizeof(char)*BUF_SIZE);
	}

	/* For run workload */
	count2 = (int*)malloc(sizeof(int)*THREAD_NUM);
	inst2 = (char***)malloc(sizeof(char**)*THREAD_NUM);
	for(i = 0; i < THREAD_NUM; i++)
	{
		count2[i] = 0;
		inst2[i] = (char**)malloc(sizeof(char*)*set_size2);
		for(j = 0; j < set_size2; j++)
			inst2[i][j] = (char*)malloc(sizeof(char)*BUF_SIZE);
	}


	//sprintf(filename, "/home/workloads/workload_%dB_%dB/tracea_load_%c_m.txt",KEY_SIZE-1, VAL_SIZE-1,workload);
	sprintf(filename, "/home/workloads/eleos_traces/1.5GB/%d/trace%c_load.txt",VAL_SIZE-1,workload);
	//For test
	//sprintf(filename, "/home/workloads/tracea_load_medium.txt");
	printf("%s\n",filename);
	fp = fopen(filename, "r");
	while(set_count <= set_size)
	{
		fgets(buf, BUF_SIZE, fp);
		buf[strlen(buf)-1] = 0;
		memcpy(buf2, buf, BUF_SIZE);
		tok = strtok_r(buf+4," ",&temp_);

		memset(key, 0, KEY_SIZE);
		memcpy(key, tok, strlen(tok));

		thread_id = _hash(key) / (g_size/THREAD_NUM);
		memcpy(inst[thread_id][count[thread_id]++], buf2, BUF_SIZE);

		set_count++;
	}
	fclose(fp);
	//sprintf(filename, "/home/workloads/workload_%dB_%dB/tracea_run_%c_m.txt",KEY_SIZE-1, VAL_SIZE-1,workload);	
	sprintf(filename, "/home/workloads/eleos_traces/1.5GB/%d/trace%c_run.txt",VAL_SIZE-1,workload);
	//For test
	//sprintf(filename, "/home/workloads/tracea_run_medium.txt");	
	printf("%s\n",filename);
	fp = fopen(filename, "r");
	while(set_count2 <= set_size2)
	{
		fgets(buf, BUF_SIZE, fp);
		buf[strlen(buf)-1] = 0;
		memcpy(buf2, buf, BUF_SIZE);
		tok = strtok_r(buf+4," ",&temp_);

		memset(key, 0, KEY_SIZE);
		memcpy(key, tok, strlen(tok));

		thread_id = _hash(key) / (g_size/THREAD_NUM);
		memcpy(inst2[thread_id][count2[thread_id]++], buf2, BUF_SIZE);

		set_count2++;
	}
	fclose(fp);
}

int main(int argc, char **argv){

	//g_size = 1048576;
  //	g_size = 32*1024;
	if(VAL_SIZE == 513) {
  	g_size = 64*1024;
	}else if(VAL_SIZE == 1025) {
  	g_size = 128*1024;
	}else if(VAL_SIZE == 4097) {
  	g_size = 32*1024;
	}

	int i;
	pthread_t threads[THREAD_NUM];
	int status;

	// Load and initialize the signed enclave
	sgx_status_t ret = load_and_initialize_enclave(&global_eid);

	if(ret != SGX_SUCCESS){
		ret_error_support(ret);
		return -1;
	}

	// Eleos initialize
	if(initialize_lib(global_eid, /*rpc*/true) != 0) {
	//if(initialize_lib(global_eid, /*rpc*/false) != 0) {
		printf("initialization failed for enclave: %lu\n", global_eid);
		return -1;
	}

	struct timeval start, end;
	double diff = 0;
	FILE *f = fopen("result.txt", "a");

	/* Make main hash table */
	if((ret = ht_create(global_eid, g_size)) != SGX_SUCCESS) {
		ret_error_support(ret);
		return -1;
	}
	printf("HT is created!\n");

	if(argc != 2)
	{
		printf("Usage: ./app [workload] : workload <-- a,b,c,d,f,g,h,i\n");
	}

	/* Load files into memory */
	file_to_memory(argv[1][0]);
	printf("Files are loaded into memory!\n");

	for(i = 0; i < THREAD_NUM; i++){
		pthread_create(&threads[i], NULL, &load_and_initialize_threads, (void *) NULL);
	}
	start1 = time(0);
	for(i = 0; i < THREAD_NUM; i++)
	{
		pthread_join(threads[i], (void **) &status);
	}
	end1 = time(0);

	fprintf(f,"\n Thread: %d Workload: %c KeySize: %d ValSize: %d Execution time for loading : %f", THREAD_NUM, argv[1][0], KEY_SIZE-1, VAL_SIZE-1, difftime(mid1,start1));
	fprintf(f,"\n Thread: %d Workload: %c KeySize: %d ValSize: %d Execution time for Running : %f", THREAD_NUM, argv[1][0], KEY_SIZE-1, VAL_SIZE-1, difftime(end1,mid1));
	fclose(f);

	if((ret = ht_destroy(global_eid, g_size)) != SGX_SUCCESS) {
		ret_error_support(ret);
		return -1;
	}

	// Destroy the enclave
	sgx_destroy_enclave(global_eid);

	//cleanup eleos
	if(cleanup_lib(global_eid) != 0) {
		printf("cleanup failed for enclave: %lu\n", global_eid);
	}

	return 0;
}
