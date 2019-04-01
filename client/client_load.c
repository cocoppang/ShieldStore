#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_IP_ADDR 12
#define MAX_FILE_LEN 70

//#define VALIDATION

struct param {
	int thread_id;
};
typedef struct param Param;

struct argument {
	char ipaddr[MAX_IP_ADDR];
	int port_num;
	int num_threads;
	int max_key_size;
	int max_val_size;
	int max_buf_size;
	int load_inst_size;
	char workload;
};
typedef struct argument Arg;

/** global variables **/
Arg arg;
pthread_mutex_t mutex;
pthread_barrier_t barrier;
static char ***inst = NULL;
static int *count = NULL;

void file_to_memory() {

	FILE *fp;
	char* buf;
	int thread_id = 0;
	char filename[MAX_FILE_LEN];
	int set_count = 0;

	/* For load workload */
	count = (int*)malloc(sizeof(int)*arg.num_threads);
	inst = (char***)malloc(sizeof(char**)*arg.num_threads);
	buf = (char*)malloc(sizeof(char)*arg.max_buf_size);

	for(int i = 0; i < arg.num_threads; i++) {
		count[i] = 0;
		inst[i] = (char**)malloc(sizeof(char*)*arg.load_inst_size/arg.num_threads);
		for(int j = 0; j < arg.load_inst_size/arg.num_threads; j++)
			inst[i][j] = (char*)malloc(sizeof(char)*arg.max_buf_size);
	}

	//sprintf(filename, "/home/workloads/workload_%dB_%dB_new/tracea_load_%c.txt", 
	//		arg.max_key_size, arg.max_val_size, arg.workload);
	sprintf(filename, "traces/trace_load.txt");

	fprintf(stdout, "trace file name: %s\n",filename);
	fp = fopen(filename, "r");
	while(set_count < arg.load_inst_size) {
		if(set_count % (arg.load_inst_size/arg.num_threads) == 0 && set_count != 0) {
			printf("set_count = %d thread_id = %d\n", set_count, thread_id);
			thread_id++;
		}

		memset(buf, 0, arg.max_buf_size);
		if(fgets(buf, arg.max_buf_size, fp) == NULL) {
			exit(0);
		}
		buf[strlen(buf)-1] = 0;
		memcpy(inst[thread_id][count[thread_id]++], buf, arg.max_buf_size);
		set_count++;
	}
	fclose(fp);
	free(buf);
}


void* client_worker(void *data_) {

	char* buf_in;
	char* buf_out;
	ssize_t errno = -1;

	pthread_mutex_lock(&mutex);

	Param* data = data_;
	int thread_id = data->thread_id;

	int client_len;
	int client_sockfd;
	struct sockaddr_in clientaddr;
	
	fprintf(stdout, "thread_id = %d\n", thread_id);

	client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	clientaddr.sin_family = AF_INET;
	//Local Server used
	clientaddr.sin_addr.s_addr = inet_addr(arg.ipaddr);
	clientaddr.sin_port = htons(arg.port_num);

	client_len = sizeof(clientaddr);
	pthread_mutex_unlock(&mutex);
	
	pthread_barrier_wait(&barrier);

	if(connect(client_sockfd, (struct sockaddr *)&clientaddr, client_len) < 0) {
		perror("Connect error: ");
		exit(0);
	}

	buf_in = (char*)malloc(sizeof(char)*arg.max_buf_size);
	buf_out = (char*)malloc(sizeof(char)*arg.max_buf_size);

	/** LOAD PHASE **/
	for(int i = 0; i < arg.load_inst_size/arg.num_threads;i++) {
		memset(buf_in, 0, arg.max_buf_size);
		memset(buf_out, 0, arg.max_buf_size);
		if(i % 100000 == 0)
			fprintf(stdout, "count = %d\n", i);
		strncpy(buf_in, inst[thread_id][i], arg.max_buf_size);
		errno = write(client_sockfd, buf_in, arg.max_buf_size);
		if(errno == -1) {
			fprintf(stderr, "Error errno: %ld\n", errno);
		}
		errno = read(client_sockfd, buf_out, arg.max_buf_size);
		if(errno == -1) {
			fprintf(stderr, "Error errno: %ld\n", errno);
		}
#ifdef VALIDATION
		fprintf(stdout, "Response: %s\n", buf_out);
#endif
	}

	printf("LOAD COMPLETE\n");

	errno = write(client_sockfd, "LOADDONE", 9);
	if(errno == -1) {
		fprintf(stderr, "Error errno: %ld\n", errno);
	}

	memset(buf_out, 0, arg.max_buf_size);
	errno = read(client_sockfd, buf_out, arg.max_buf_size);
	fprintf(stdout, "%s\n", buf_out);
	if(errno == -1) {
		fprintf(stderr, "Error errno: %ld\n", errno);
	}
	if(strncmp(buf_out,"EXIT",4) == 0) {
		fprintf(stdout, "Exit message: %s\n", buf_out);
	}

	free(buf_in);
	free(buf_out);

	close(client_sockfd);

}

void configuration_init() {
	char LOCAL_IP[MAX_IP_ADDR] = "127.0.0.1";

	memcpy(arg.ipaddr, LOCAL_IP, MAX_IP_ADDR);
	arg.port_num = 1234;
	arg.max_key_size = 16;
	arg.max_val_size = 16;
	arg.max_buf_size = 64;
	//arg.num_threads = 16;
	arg.num_threads = 4;
	arg.workload = 'a';
/* For loading the number of key-value data
 * For 16-16		the working set size is 160MB
 * For 16-128		the working set size is 1280MB(1.28GB)
 * For 16-512		the working set size is 5120MB(5.12GB)
 * For 16-1024	the working set size is 10240MB(10GB)
 */
	//arg.load_inst_size = 10000000;
	arg.load_inst_size = 500;
	
}

void help() {
	printf("-h : help\n");
	printf("-i <server ip address>\n");
	printf("-p <server port number>\n");
	printf("-k <maximum key size>\n");
	printf("-v <maximum value size>\n");
	printf("-b <maximum buffer size>\n");
	printf("-t <number of threads>\n");
	printf("-w <workload name>\n");
	printf("-n <number of operations>\n");
}

int main(int argc, char **argv) {

	int status;
	int opt;

	configuration_init();

	while((opt = getopt(argc, argv, "hi:p:k:v:b:t:w:n:")) != -1) {
		switch(opt) {
			case 'h':
				help();
				break;

			case 'i':
				memcpy(arg.ipaddr, optarg, strlen(optarg));
				break;

			case 'p':
				arg.port_num = atoi(optarg);
				break;

			case 'k':
				arg.max_key_size = atoi(optarg);
				break;

			case 'v':
				arg.max_val_size = atoi(optarg);
				if(arg.max_val_size == 16) 
					arg.max_buf_size = 64;
				else if(arg.max_val_size == 128)
					arg.max_buf_size = 192;
				else if(arg.max_val_size == 512) 
					arg.max_buf_size = 576;
				else
					arg.max_buf_size = arg.max_key_size + arg.max_val_size + 10;
				break;

			case 'b':
				arg.max_buf_size = atoi(optarg);
				assert(arg.max_buf_size >= arg.max_key_size + arg.max_val_size + 10);
				break;

			case 't':
				arg.num_threads = atoi(optarg);
				break;

			case 'w':
				arg.workload = optarg[0];
				break;

			case 'n':
				arg.load_inst_size = atoi(optarg);
				break;

			default:
				fprintf(stderr, "Illegal argument \"%c\"\n", opt);
				help();
				break;
		}
	}

	pthread_t* threads;
	Param* parameter;

	pthread_mutex_init(&mutex, NULL);
	pthread_barrier_init(&barrier, NULL, arg.num_threads);

	threads = (pthread_t*)malloc(sizeof(pthread_t)*arg.num_threads);
	parameter = (Param*)malloc(sizeof(Param)*arg.num_threads);

	file_to_memory();
	printf("Files are loaded into memory!\n");

	for(int i = 0; i < arg.num_threads; i++) {
		parameter[i].thread_id = i;
		pthread_create(&threads[i], NULL, &client_worker, (void *)&parameter[i]);
	}


	for(int i = 0 ; i < arg.num_threads; i++) {
		pthread_join(threads[i], (void**)&status);
	}

	for(int i = 0; i < arg.num_threads; i++) {
		for(int j = 0; j < arg.load_inst_size/arg.num_threads; j++)
			free(inst[i][j]);
		free(inst[i]);
	}
	free(count);
	free(inst);
	free(threads);
	free(parameter);

	pthread_mutex_destroy(&mutex);
	pthread_barrier_destroy(&barrier);

	return 1;
}
