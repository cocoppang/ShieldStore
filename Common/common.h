#ifndef __COMMON_H
#define __COMMON_H

#include <stdbool.h>

/* PREDEFINED DATA */
#define MAC_SIZE 16
#define NAC_SIZE 16

#define PAGE_SIZE 4096

/* MAC BUffer */
struct mac_entry{
	int size;
	uint8_t mac[MAC_SIZE*30];
};
typedef struct mac_entry MACentry;

struct macbuffer{
	MACentry* entry;
};
typedef struct macbuffer MACbuffer;

struct hashtable{
	int size;
	struct entry **table;
};
typedef struct hashtable hashtable;

/* DATA STRUCTURE DECLARATION */
struct entry{
	uint32_t key_size;			// key size
	uint32_t val_size;			// value size
	uint8_t key_hash;			// key hint
	char* key_val;				// concaternated key and value
	uint8_t nac[NAC_SIZE];		// This field store nonce + counter 
	uint8_t mac[MAC_SIZE];		// This field stores MAC of data entry fields
	struct entry *next;			// next entry
};
typedef struct entry entry;

struct job{
	int client_sock;
	char* buf ;
};
typedef struct job job;

typedef struct {
    char* buf;
	int client_sock_;
	int num_clients_;
	bool child_done_;
} EcallParams;

struct argument {
	int port_num;
	int num_threads;
	int max_buf_size;
	int bucket_size;
	int tree_root_size;
	bool key_opt;
	bool mac_opt;
	bool persistent;
};
typedef struct argument Arg;


#endif
