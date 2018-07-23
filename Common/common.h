#ifndef __COMMON_H
#define __COMMON_H

/* PREDEFINED DATA */
#define KEY_SIZE 17
#define MAC_SIZE 16
#define NAC_SIZE 16

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

/* MAC Bucketing Optimization */
//#define Enable_MACBUFFER 1
#define Enable_MACBUFFER 0

/* Key Hashing Optimization */
//#define Enable_KEYOPT 1
#define Enable_KEYOPT 0

/* MAC bucketing buffer */
struct mac_entry{
	int size;
	uint8_t mac[MAC_SIZE*30];
};
typedef struct mac_entry MACentry;

struct macbuffer{
	MACentry* entry;
};
typedef struct macbuffer MACbuffer;

struct node{
	uint8_t mac[MAC_SIZE];	// This field stores MAC of child nodes
	struct node *left;		// This field points left child
	struct node *right;		// This field points right child
};
typedef struct node node;

/* Hash Table for ShieldStore */
struct hashtable{
	int size;
	struct entry **table;
};
typedef struct hashtable hashtable;

/* Data entry */
struct entry{
	uint32_t key_size;
	uint32_t val_size;
	uint8_t key_hash;
	char* key_val;						// This field stores key + val
	uint8_t nac[NAC_SIZE];		// This field store IV + counter
	uint8_t mac[MAC_SIZE];		// This field stores MAC
	struct entry *next;
};
typedef struct entry entry;

/* Data passing structure */
struct job{
	int op;  // This field stores operations
	char plain[BUF_SIZE];
};
typedef struct job job;

/* For HotCalls */
typedef struct {
    char buf[BUF_SIZE];
    hashtable *ht_;
    MACbuffer *MACbuf_;
} EcallParams;

#endif
