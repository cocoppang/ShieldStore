#include "App.h"

/** 
 *  Ocall function for returning results to client
 **/
void message_return(char* ret, size_t ret_size, int client_sock)
{
	if(write(client_sock, ret, ret_size) <= 0)
	{
		close(client_sock);
		return;
	}
}

/** 
 * Ocall function for allocating untrusted memory
 **/
void* sbrk_o(size_t size)
{
	void* result = NULL;
	result = sbrk((intptr_t)size);
	return result;
}

/** 
 * Ocall function for debugging 
 **/
void print(const char *str){
	printf("--------------------\n");
	printf("%s\n", str);
	printf("--------------------\n");
}


