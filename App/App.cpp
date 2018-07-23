#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include <fstream>
#include <iostream>

#include "Enclave_u.h"
#include "sgx_urts.h"

#include "ErrorSupport.h"

#include <sys/time.h>

//For multi threading
#include <pthread.h>

//For socket
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//For mmap
#include <sys/mman.h>
#include <fcntl.h>

//for sbrk
#include <unistd.h>

#define ENCLAVE_NAME "libenclave.signed.so"
#define TOKEN_NAME "Enclave.token"

using namespace std;

// Global variables
sgx_enclave_id_t global_eid = 0;
sgx_launch_token_t token = {0};

int g_port;
int g_size;

/* Hash table variable */
static hashtable *ht = NULL;

static MACbuffer *MACbuf = NULL;

time_t start1, mid1, end1; 

int global_client_sockfd;
bool exit_flag = false;

void dummy()
{
	mid1 = time(0);
}

int setNonblocking(int fd)
{
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}

/* OCALL for debugging */
void PRINT_FOR_DEBUGING( void* mem, uint32_t len)
{
	if(!mem || !len)
	{   
		printf("\n( null )\n");
		return;
	}   
	uint8_t *array = (uint8_t *)mem;
	printf("%u bytes:\n{\n", len);
	uint32_t i = 0;
	for(i = 0; i < len - 1; i++)
	{   
		printf("0x%x, ", array[i]);
		if(i % 8 == 7) printf("\n");
	}   
	printf("0x%x ", array[i]);
	printf("\n}\n");
}


/* OCALL for debugging */
void print(const char *str){
	printf("--------------------\n");
	printf("%s\n", str);
	printf("--------------------\n");
}

/* Parsing command line options */
void parse_option(int argc, char **argv){
	int cur = 1;

	/** Default variables **/
	g_port = 1234;
	g_size = 1024*1024*8;		//8M entries

	printf("Server Configuration\n");
	printf("Port:         %d\n", g_port);
	printf("Table Size:   %d\n", g_size);
}

/* create mac buffer */
MACbuffer * macbuffer_create(int size){
	MACbuffer *Mbuf = NULL;
	int i;

	Mbuf = (MACbuffer*)malloc(sizeof(MACbuffer));
	Mbuf->entry = (MACentry*)malloc(sizeof(MACentry)*size);
	for(i = 0 ; i < size; i++)
	{
		Mbuf->entry[i].size = 0;
	}
	return Mbuf;
}

/* Create a new hashtable. */
hashtable * ht_create(int size){
	hashtable *ht = NULL;
	int i;

	if(size < 1) return NULL;

	/* Allocate the table itself. */
	if((ht = (hashtable *)malloc(sizeof(hashtable))) == NULL) return NULL;

	/* Allocate pointers to the head nodes. */
	if((ht->table = (entry **)malloc(sizeof(entry*) * size)) == NULL) return NULL;

	for(i = 0; i < size; i++) ht->table[i] = NULL;

	ht->size = size;

	return ht;
}

/** Allocation untrusted memory as really needed 
 *  For custom TCMALLOC.									
 **/
void* sbrk_o(size_t size)
{
	void* result = NULL;
	result = sbrk((intptr_t)size);
	return result;
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

void* EnclaveResponderThread( void* hotEcallAsVoidP )
{
	//To be started in a new thread
	HotCall *hotEcall = (HotCall*)hotEcallAsVoidP;
	EcallStartResponder( global_eid, hotEcall );

	return NULL;
}

int SGX_CDECL main(int argc, char **argv){
	/* Socket variable */
	int server_sockfd, client_sockfd;
	int state;
	socklen_t client_len;
	int pid;
	struct sockaddr_in clientaddr, serveraddr;
	client_len = sizeof(clientaddr);

	sgx_status_t ret;

	pthread_t threads[1];

	/* Benchmark handling variable */
	int i;
	int status;

	fd_set readfds;
	fd_set master;
	int fdmax;

	// Load and initialize the signed enclave
	ret = load_and_initialize_enclave(&global_eid);
	if(ret != SGX_SUCCESS){
		ret_error_support(ret);
		return -1;
	}
	/* Parsing command line options */
	parse_option(argc, argv);

	/* Make main hash table */
	ht = ht_create(g_size);

	/* MAC buffer Create */
	MACbuf = macbuffer_create(g_size);

	/* Initialize hash tree */
	enclave_init_tree_root(global_eid, ht, MACbuf);

	/* Make socket */
	if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket error: ");
		exit(0);
	}

	int enable = 1;
	if(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsocketopt(SO_REUSEADDR) failed");
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(g_port);

	FD_ZERO(&master);
	FD_ZERO(&readfds);

	//Set non-blocking
	setNonblocking(server_sockfd);

	state = bind(server_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if(state == -1){
		perror("bind error : ");
		exit(0);
	}

	state = listen(server_sockfd, 5);
	if(state == -1){
		perror("listen error : ");
		exit(0);
	}

	FD_SET(server_sockfd, &master);
	fdmax = server_sockfd;
	
	/** Set parameters for HotCalls **/
	EcallParams ecallParams;
	memset(ecallParams.buf, 0, BUF_SIZE);
	HotCall     hotEcall        = HOTCALL_INITIALIZER;
	hotEcall.data               = &ecallParams;

	/** Create one more thread for HotCalls **/
	pthread_create(&threads[0], NULL, &EnclaveResponderThread, (void *)&hotEcall); 

	while(1)
	{
		readfds = master;
		if(select(fdmax+1, &readfds, NULL, NULL, NULL) == -1)
			exit(0);

		for(int i = 0; i <= fdmax;i++)
		{
			if(FD_ISSET(i, &readfds))
			{
				if(i == server_sockfd)
				{
					client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr, &client_len);
					if(client_sockfd == -1){
						perror("Accept error : ");
						exit(0);
					}
					printf("Accept() is OK\n");
					FD_SET(client_sockfd, &master);
					if(client_sockfd > fdmax)
					{
						fdmax = client_sockfd;
					}
					global_client_sockfd = client_sockfd;
					printf("Time check start\n");
					start1 = time(0);
				}
				else
				{

					memset(ecallParams.buf, 0, BUF_SIZE);

					if(read(client_sockfd, ecallParams.buf, BUF_SIZE) <= 0){
						close(client_sockfd);
						break;
					}
					ecallParams.ht_ = ht;
					ecallParams.MACbuf_ = MACbuf;

					HotCall_requestCall(&hotEcall, 0, &ecallParams);

					if(write(client_sockfd, ecallParams.buf, BUF_SIZE) <= 0) {
						close(client_sockfd);
						break;
					}

					if(strncmp(ecallParams.buf, "EXIT", 4) == 0)
					{
						exit_flag = true;
						break;
					}
				}

			}
		}

		if(exit_flag)
		{
			close(i);
			FD_CLR(i, &master);
			break;
		}
	}

	StopResponder(&hotEcall);
	
	pthread_join(threads[0],(void **)&status);

	end1 = time(0); 

	FILE *f;
	f = fopen("result.txt", "a");
	fprintf(f,"\n Thread: %d Execution time for Loading : %f", THREAD_NUM, difftime(mid1,start1));
	fprintf(f,"\n Thread: %d Execution time for Running : %f", THREAD_NUM, difftime(end1,mid1));

	fclose(f);

	sgx_destroy_enclave(global_eid);

	return 0;
}

