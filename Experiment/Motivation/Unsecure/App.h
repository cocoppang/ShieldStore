#define FOOTPRINT 1024*1024*1024

struct region {
	int addr[FOOTPRINT];
};

struct region *page;
