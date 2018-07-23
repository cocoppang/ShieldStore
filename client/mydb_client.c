#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define KEY_SIZE 17

#ifdef SMALL
#define VAL_SIZE 17
#define BUF_SIZE 64

#elif MEDIUM
#define VAL_SIZE 129
#define BUF_SIZE 192

#elif LARGE
#define VAL_SIZE 513
#define BUF_SIZE 576
#endif

#define THREAD_NUM 1

static char ***inst = NULL;
static int *count = NULL;

static char ***inst2 = NULL;
static int *count2 = NULL;

/** At the client side, it reads the trace file of the input and send the traces to the ShieldStore server.
 *  Read the traces of the input in this function.
 **/
void file_to_memory(char workload) {
	
	char *tok;
	FILE *fp;
	char *temp_;
	char key[KEY_SIZE];
	int i, j;

	/* Listening buffer*/
	char buf[BUF_SIZE];
	char buf2[BUF_SIZE];

	int thread_id;

	char filename[70];

	/* For loading the number of key-value data
	 * For 16-16		the working set size is 160MB
	 * For 16-128		the working set size is 1280MB(1.28GB)
	 * For 16-512		the working set size is 5120MB(5.12GB)
	 * For 16-1024	the working set size is 10240MB(10GB)
	 */
	//int set_size = 10000000;
	int set_size = 100000;
	int set_count = 1;

	/* For running the number of key-value data
	 * the number of running key-value data is to 10M
	 */
	//int set_size2 = 10000000;
	int set_size2 = 100000;
	int set_count2 = 1;
	
	/* For load workload */
	count = (int*)malloc(sizeof(int)*THREAD_NUM);
	inst = (char***)malloc(sizeof(char**)*THREAD_NUM);

	for(i = 0; i < THREAD_NUM; i++) {
		count[i] = 0;
		inst[i] = (char**)malloc(sizeof(char*)*set_size);
		for(j = 0; j < set_size; j++)
			inst[i][j] = (char*)malloc(sizeof(char)*BUF_SIZE);
	}

	/* For run workload */
	count2 = (int*)malloc(sizeof(int)*THREAD_NUM);
	inst2 = (char***)malloc(sizeof(char**)*THREAD_NUM);

	for(i = 0; i < THREAD_NUM; i++) {
		count2[i] = 0;
		inst2[i] = (char**)malloc(sizeof(char*)*set_size2);
		for(j = 0; j < set_size2; j++)
			inst2[i][j] = (char*)malloc(sizeof(char)*BUF_SIZE);
	}

	sprintf(filename, "./workloads/tracea_load_%c_m.txt",workload);
	printf("%s\n",filename);
	fp = fopen(filename, "r");
	while(set_count <= set_size) {
		if(fgets(buf, BUF_SIZE, fp) == NULL) {
			exit(0);
		}
		buf[strlen(buf)-1] = 0;
		memcpy(buf2, buf, BUF_SIZE);

		thread_id = 0;
		memcpy(inst[thread_id][count[thread_id]++], buf2, BUF_SIZE);

		set_count++;
	}
	fclose(fp);
	
	sprintf(filename, "./workloads/tracea_run_%c_m.txt",workload);	
	printf("%s\n",filename);
	fp = fopen(filename, "r");
	while(set_count2 <= set_size2) {
		if(fgets(buf, BUF_SIZE, fp) == NULL) {
			exit(0);
		}
		buf[strlen(buf)-1] = 0;
		memcpy(buf2, buf, BUF_SIZE);

		thread_id = 0;
		memcpy(inst2[thread_id][count2[thread_id]++], buf2, BUF_SIZE);

		set_count2++;
	}
	fclose(fp);
}

int main(int argc, char **argv) {

	int client_len;
	int client_sockfd;
	int i,j;
	int count1_ = 0;
	int count2_ = 0;
	
	char buf_in[BUF_SIZE];
	char buf_out[BUF_SIZE];
	
	int status;

	struct sockaddr_in clientaddr;
	ssize_t errno = -1;

	if(argc != 3) {
		printf("Usage : ./mydb_client [port] [workload_name]\n");
		exit(0);
	}

	client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	clientaddr.sin_family = AF_INET;
	//Local Server used
	clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	clientaddr.sin_port = htons(atoi(argv[1]));

	client_len = sizeof(clientaddr);

	file_to_memory(argv[2][0]);
	printf("Files are loaded into memory!\n");

	if(connect(client_sockfd, (struct sockaddr *)&clientaddr, client_len) < 0) {
		perror("Connect error: ");
		exit(0);
	}


	/** LOAD PHASE **/
	for(i = 0; i < count[0];i++) {
		memset(buf_out, 0, BUF_SIZE);
		strncpy(buf_in, inst[0][i], BUF_SIZE);
		errno = write(client_sockfd, buf_in, BUF_SIZE);
		if(errno == -1) {
			printf("Error errno: %ld\n", errno);
		}

		errno = read(client_sockfd, buf_out, BUF_SIZE);
		if(errno == -1) {
			printf("Error errno: %ld\n", errno);
		}
	}
	
	printf("LOAD COMPLETE\n");

	strncpy(buf_in, "LOADDONE", BUF_SIZE);
	errno = write(client_sockfd,buf_in, BUF_SIZE);
	if(errno == -1) {
		printf("Error errno: %ld\n", errno);
	}

	/** RUN PHASE **/
	printf("run count : %d\n", count2[0]);
	for(i = 0; i < count2[0];i++) {
		memset(buf_out, 0, BUF_SIZE);
		if(i % 10000000 == 0)
			printf("count = %d\n",i);
		strncpy(buf_in, inst2[0][i], BUF_SIZE);
		errno = write(client_sockfd, buf_in, BUF_SIZE);
		if(errno == -1) {
			printf("Error errno: %ld\n", errno);
		}
		errno = read(client_sockfd, buf_out, BUF_SIZE);
		if(errno == -1) {
			printf("Error errno: %ld\n", errno);
		}
	}

	printf("RUN COMPLETE\n");

	errno = write(client_sockfd, "EXIT", 5);
	if(errno == -1) {
		printf("Error errno: %ld\n", errno);
	}

	memset(buf_out, 0, BUF_SIZE);
	errno = read(client_sockfd, buf_out, BUF_SIZE);
	if(errno == -1) {
		printf("Error errno: %ld\n", errno);
	}
	if(strncmp(buf_out,"EXIT",4) == 0) {
		printf("%s\n", buf_out);
	}

	close(client_sockfd);
	
	return 1;
}
