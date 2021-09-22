#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"
#include "util.h"
#include "net.h"
#include "xml.h"

void
itemAction(itemStruct *item, const char *folder)
{
	// Receives a link list of articles to process.

	itemStruct *cur = item;

	unsigned long long int newItems = 0;

	while (cur) {
		char *filePath;
		char *fileName = san(cur->title, 1);
		itemStruct *prev = cur;

		if (fileName[0])
			filePath = ecalloc(
			               strlen(folder)
						   + strlen(fileName) + 2
						   + strlen(fileExt),
			               sizeof(char));
		else {
			logMsg(1, "Invalid article title.\n");

			cur = cur->next;
			freeItem(prev);

			continue;
		}

		strcpy(filePath, folder);

		unsigned long long int ind = strlen(filePath);
		filePath[ind] = fsep();
		filePath[ind + 1] = '\0';

		strcat(filePath, fileName);
		free(fileName);

		strcat(filePath, fileExt);

		FILE *itemFile = fopen(filePath, "a");
		free(filePath);

		// Do not overwrite files
		if (!ftell(itemFile)) {
			newItems++;

			fprintf(itemFile, "<h1>%s</h1><br>\n", cur->title);
			fprintf(itemFile, "<a href=\"%s\">Link</a><br>\n", san(cur->link, 0));
			fprintf(itemFile, "%s", san(cur->description, 0));
		}

		fclose(itemFile);

		cur = cur->next;
		freeItem(prev);
	}

	if (newItems)
		logMsg(2, "%d new articles for feed %s.\n", newItems, folder);
}

void
finish(char *url, long responseCode)
{
	// Executed after a download finishes

	if (responseCode == 200)
		logMsg(4, "Finished downloading %s\n", url);
	else if (!responseCode)
		logMsg(1, "Can not reach %s\n", url);
	else
		logMsg(1, "HTTP %ld for %s\n", responseCode, url);
}

int
main(int argc, char  *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		logMsg(0, "MinRSS %s\n", VERSION);
	else if (argc != 1)
		logMsg(0, "Usage: minrss [-v]\n");

	unsigned int i = 0;

	initCurl();

	outputStruct outputs[LEN(links)];
	memset(outputs, 0, sizeof(outputs));

	for (i = 0; i < LEN(links); i++) {
		if (links[0].url[0] == '\0')
			logMsg(0, "No feeds, add them in config.def.h\n");

		logMsg(4, "Requesting %s\n", links[i].url);
		createRequest(links[i].url, &outputs[i]);
	}

	performRequests(finish);

	logMsg(3, "Finished downloads.\n");

	for (i = 0; i < LEN(links); i++) {
		logMsg(4, "Parsing %s\n", links[i].url);

		if (outputs[i].buffer && outputs[i].buffer[0]) {
			readDoc(outputs[i].buffer, links[i].feedName, itemAction);
			free(outputs[i].buffer);
		}
	}

	logMsg(3, "Finished parsing feeds.\n");

	return 0;
}
