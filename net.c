#include <curl/curl.h>
#include <curl/easy.h>
#include <stdlib.h>
#include <string.h>

#include "net.h"
#include "util.h"
#include "config.h"

static CURLM *multiHandle;

int
initCurl()
{
	// Initialise the curl handle.

	curl_global_init(CURL_GLOBAL_ALL);
	multiHandle = curl_multi_init();

	return !multiHandle;
}

static size_t
writeCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	// Write blocks of data to the output.

	size_t realsize = size * nmemb;

	outputStruct *mem = (outputStruct*) data;

	mem->buffer = realloc(mem->buffer, mem->size + realsize + 1);

	if (mem->buffer) {
		memcpy(&(mem->buffer[mem->size]), ptr, realsize);

		mem->size += realsize;
		mem->buffer[mem->size] = 0;
	}

	return realsize;
}

int
createRequest(const char* url, outputStruct *output)
{
	// Create the curl request for an URL.

	CURL *requestHandle = curl_easy_init();

	if (!requestHandle)
		logMsg(0, "Can't initialise curl.\n");

	output->buffer = NULL;
	output->size = 0;

	CURLcode stat;
	if (curl_easy_setopt(requestHandle, CURLOPT_URL, url)) {
		logMsg(1, "Invalid URL: %s\n", url);
	}

	stat = curl_easy_setopt(requestHandle, CURLOPT_WRITEFUNCTION, writeCallback);
	stat = curl_easy_setopt(requestHandle, CURLOPT_WRITEDATA, (void*)output);
	stat = curl_easy_setopt(requestHandle, CURLOPT_MAXREDIRS, maxRedirs);
	stat = curl_easy_setopt(requestHandle, CURLOPT_FOLLOWLOCATION, 1L);

	if (stat) {
		fprintf(stderr, "Unexpected curl error: %s.\n", curl_easy_strerror(stat));
		return 1;
	}

	CURLMcode multiStat = curl_multi_add_handle(multiHandle, requestHandle);
	if (multiStat) {
		fprintf(stderr, "Unexpected curl error: %s.\n", curl_multi_strerror(multiStat));
		return 1;
	}

	return 0;
}

int
performRequests(void callback(char *, long))
{
	// Perform all the curl requests.

	int runningRequests;

	do {
		curl_multi_wait(multiHandle, NULL, 0, 1000, NULL);
		curl_multi_perform(multiHandle, &runningRequests);

		CURLMsg* msg;

		int queueMsgs;

		while ((msg = curl_multi_info_read(multiHandle, &queueMsgs))) {
			if (msg->msg == CURLMSG_DONE) {
				CURL *requestHandle = msg->easy_handle;

				char *url = NULL;
				long responseCode = 0;

				curl_easy_getinfo(requestHandle, CURLINFO_EFFECTIVE_URL, &url);
				curl_easy_getinfo(requestHandle, CURLINFO_RESPONSE_CODE, &responseCode);

				callback(url, responseCode);

				curl_multi_remove_handle(multiHandle, requestHandle);
				curl_easy_cleanup(requestHandle);
			}
		}

	// > 0 because curl puts negative numbers when there's broken requests
	} while (runningRequests > 0);

	curl_multi_cleanup(multiHandle);

	return 0;
}
