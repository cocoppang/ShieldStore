#include <fstream>
#include <iostream>

#include "App.h"
#include "ErrorSupport.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <chrono>
#include <sgx_uswitchless.h>

#define ENCLAVE_NAME "libenclave.signed.so"
#define TOKEN_NAME "Enclave.token"

// Global variables
sgx_enclave_id_t global_eid = 0;
sgx_launch_token_t token = {0};
Arg arg;
static hashtable *ht = NULL;
static MACbuffer *MACbuf = NULL;

std::chrono::time_point<std::chrono::high_resolution_clock> start1;
std::chrono::time_point<std::chrono::high_resolution_clock> end1;
std::chrono::time_point<std::chrono::high_resolution_clock> mid1;


/**
 * help function
 **/
void help() {
	printf("-h : help\n");
	printf("-p <port number>\n");
	printf("-b <maximum buffer size>\n");
	printf("-t <number of threads>\n");
	printf("-s <number of hash buckets>\n");
	printf("-r <number of root nodes in enclave>\n");
	printf("-y : key-hint optimization on\n");
	printf("-c : mac bucketing optimization on\n");
}

/**
 * making socket nonblocking
 **/
int setNonblocking(int fd) {
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}

/**
 * parsing command line options 
 **/
void parse_option(){
	printf("Server Configuration\n");
	printf("Port: %d\n", arg.port_num);
	printf("Table Size: %d\n", arg.bucket_size);
	printf("Max Buf Size: %d\n", arg.max_buf_size);
	printf("Num Threads: %d\n", arg.num_threads);
	printf("Key OPT: %s\n", arg.key_opt ? "true" : "false");
	printf("MAC OPT: %s\n", arg.mac_opt ? "true" : "false");
}

/**
 * For mac bucketing optimization
 * create mac buffer
 **/
MACbuffer * macbuffer_create(int size){
	MACbuffer *Mbuf = NULL;

	Mbuf = (MACbuffer*)malloc(sizeof(MACbuffer));
	Mbuf->entry = (MACentry*)malloc(sizeof(MACentry)*size);
	for(int i = 0 ; i < size; i++) {
		Mbuf->entry[i].size = 0;
	}
	return Mbuf;
}

/** 
 * create new hashtable
 **/
hashtable * ht_create(int size){
	hashtable *ht = NULL;

	if(size < 1) return NULL;

	/* Allocate the table itself. */
	if((ht = (hashtable *)malloc(sizeof(hashtable))) == NULL) return NULL;

	/* Allocate pointers to the head nodes. */
	if((ht->table = (entry **)malloc(sizeof(entry*) * size)) == NULL) return NULL;

	for(int i = 0; i < size; i++) ht->table[i] = NULL;

	ht->size = size;

	return ht;
}

/**
 * load and initialize the enclave     
 **/
sgx_status_t load_and_initialize_enclave(sgx_enclave_id_t *eid,const sgx_uswitchless_config_t* us_config){
	sgx_status_t ret = SGX_SUCCESS;
	int retval = 0;
	int updated = 0;

	// Step 1: check whether the loading and initialization operations are caused
	if(*eid != 0)
		sgx_destroy_enclave(*eid);

        void* enclave_ex_p[32] = { 0 };
    
        enclave_ex_p[SGX_CREATE_ENCLAVE_EX_SWITCHLESS_BIT_IDX] = (void*)us_config;

	// Step 2: load the enclave
	ret = sgx_create_enclave_ex(ENCLAVE_NAME, SGX_DEBUG_FLAG, &token, &updated, eid, NULL,SGX_CREATE_ENCLAVE_EX_SWITCHLESS, enclave_ex_p);

	if(ret != SGX_SUCCESS)
		return ret;

	// Save the launch token if updated
	if(updated == 1){
		std::ofstream ofs(TOKEN_NAME, std::ios::binary|std::ios::out);
		if(!ofs.good())
			std::cout<< "Warning: Failed to save the launch token to \"" <<TOKEN_NAME <<"\""<<std::endl;
		else
			ofs << token;
	}
	return ret;
}

/**
 * creating server threads working inside the enclave 
 **/
void *load_and_initialize_threads(void *temp){
	enclave_worker_thread(global_eid, (hashtable *)ht, (MACbuffer*)MACbuf);
}

/**
 * For hotcalls
 **/
void* EnclaveResponderThread( void* hotEcallAsVoidP )
{
	//To be started in a new thread
	HotCall *hotEcall = (HotCall*)hotEcallAsVoidP;
	//EcallStartResponder( global_eid, hotEcall );

	return NULL;
}

/**
 * init default configuration values
 **/
void configuration_init() {

	arg.port_num = 1234;
	arg.max_buf_size = 64;
	arg.num_threads = 1;

	arg.bucket_size = 8*1024*1024;
	arg.tree_root_size = 4*1024*1024;

	/** Optimization **/
	arg.key_opt = false;
	arg.mac_opt = false;

}

int SGX_CDECL main(int argc, char **argv){

        /* Configuration for Switchless SGX */
        sgx_uswitchless_config_t us_config = SGX_USWITCHLESS_CONFIG_INITIALIZER;
        us_config.num_uworkers = 2;
        us_config.num_tworkers = 2;

	/* Socket variable */
	int server_sockfd, client_sockfd;
	int state;
	socklen_t client_len;
	struct sockaddr_in clientaddr, serveraddr;
	client_len = sizeof(clientaddr);

	sgx_status_t ret;

	int i;
	int opt;
	int status;

	fd_set readfds;
	fd_set master;
	int fdmax;

	pthread_t* threads;

	int num_connection = 0;
	int num_clients = 0;
	bool run_phase = false;

	configuration_init();

	while((opt = getopt(argc, argv, "hp:b:t:s:r:yc")) != -1) {
		switch(opt) {
			case 'h':
				help();
				exit(0);
				break;

			case 'p':
				arg.port_num = atoi(optarg);
				break;

			case 'b':
				arg.max_buf_size = atoi(optarg);
				break;

			case 't':
				arg.num_threads = atoi(optarg);
				break;

			case 's':
				arg.bucket_size = atoi(optarg);
				break;
			
			case 'r':
				arg.tree_root_size = atoi(optarg);
				break;

			case 'y':
				arg.key_opt = true;
				break;

			case 'c':
				arg.mac_opt = true;
				break;

			default:
				fprintf(stderr, "Illegal argument \"%c\"\n", opt);
				help();
				exit(0);
				break;
		}
	}

	/* Load and initialize the signed enclave */
	ret = load_and_initialize_enclave(&global_eid,&us_config);
	if(ret != SGX_SUCCESS){
		ret_error_support(ret);
		return -1;
	}

	/* Parsing command line options */
	parse_option();

	/* Make main hash table */
	ht = ht_create(arg.bucket_size);

	/* MAC buffer Create */
	MACbuf = macbuffer_create(arg.bucket_size);

	/* Initialize hash tree */
	enclave_init_values(global_eid, ht, MACbuf, arg);

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
	serveraddr.sin_port = htons(arg.port_num);

	FD_ZERO(&master);
	FD_ZERO(&readfds);

	/* Set non-blocking */
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

	/* Initalize hotcalls data structure */
	EcallParams ecallParams;
	ecallParams.buf = (char*)malloc(sizeof(char)*arg.max_buf_size);
	memset(ecallParams.buf, 0, arg.max_buf_size);
	HotCall     hotEcall        = HOTCALL_INITIALIZER;
	hotEcall.data               = &ecallParams;
	
	//create server threads and hotcalls thread
	threads = (pthread_t*)malloc(sizeof(pthread_t)*(arg.num_threads+1));

	/* For HotCall & Worker Thread */
	pthread_create(&threads[0], NULL, &EnclaveResponderThread, (void *)&hotEcall); 
	for(int i=1;i<arg.num_threads+1;i++){
		pthread_create(&threads[i], NULL, &load_and_initialize_threads, (void *)NULL); 
	}

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
					fprintf(stdout, "Accept() is OK\n");
					FD_SET(client_sockfd, &master);

					if(client_sockfd > fdmax) {
						fdmax = client_sockfd;
					}

					if(run_phase == true && num_connection == 0) {
						printf("Time check start\n");
						mid1 = std::chrono::high_resolution_clock::now();
					}
					num_connection++;

					//The maximum number of num_connection is the number of clients
					if(num_connection > num_clients) {
						num_clients = num_connection;
						printf("num_clients: %d\n", num_clients);

						start1 = std::chrono::high_resolution_clock::now();
					}
				}
				else {

					memset(ecallParams.buf, 0, arg.max_buf_size);

					if(read(i, ecallParams.buf, arg.max_buf_size) <= 0){
						close(i);
						break;
					}

					ecallParams.client_sock_ = i;
					ecallParams.num_clients_ = num_clients;


					//HotCall_requestCall(&hotEcall, 0, &ecallParams);
					enclave_message_pass(global_eid, &ecallParams);
	
					if(strncmp(ecallParams.buf, "LOADDONE", 9) == 0) {
						close(i);
						FD_CLR(i, &master);
						num_connection--;
						run_phase = true;
						if(num_connection == 0) {
							printf("Waiting for running clients\n");
						}
					}


					if(strncmp(ecallParams.buf, "EXIT", 4) == 0) {
						close(i);
						FD_CLR(i, &master);
						num_connection--;
						printf("num_connection: %d remained\n", num_connection);
						break;
					}
				}

			}
		}

		if(strncmp(ecallParams.buf, "EXIT", 4) == 0 && num_connection == 0) {
			printf("Client request is END\n");
			break;
		}
	}

	StopResponder(&hotEcall);

	for(int i = 0; i < arg.num_threads+1;i++) {
		pthread_join(threads[i],(void **)&status);
	}

	end1 = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed;
	FILE *f;
	f = fopen("result.txt", "a");
	elapsed = mid1 - start1;
	fprintf(f,"\n Thread: %d Execution time for Loading : %lf", arg.num_threads, elapsed.count());
	elapsed = end1 - mid1;
	fprintf(f,"\n Thread: %d Execution time for Running : %lf", arg.num_threads, elapsed.count());
	fclose(f);

	free(threads);
	sgx_destroy_enclave(global_eid);

	return 0;
}

