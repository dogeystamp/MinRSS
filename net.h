#include <curl/curl.h>

typedef struct {
	char *buffer;
	size_t size;
} outputStruct;

int initCurl();
int createRequest(const char *url, outputStruct *output);
int performRequests(void callback(char *, long));
