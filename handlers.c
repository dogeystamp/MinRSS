#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "util.h"

enum feedFormat {
	NONE,
	RSS,
	ATOM,
};

typedef struct itemStruct itemStruct;
struct itemStruct {
	char *title;
	char *link;
	char *description;
	itemStruct *next;
};

void
freeItem(itemStruct *item)
{
	// Free the memory used by an article.

	char **mems[] = {
		&item->title,
		&item->link,
		&item->description,
	};

	for (unsigned long int i = 0; i < LEN(mems); i++) {
		if (*mems[i])
			free(*mems[i]);
	}

	free(item);
}

FILE *
openFile(const char *folder, char *fileName, char *fileExt)
{
	// [folder]/[fileName][fileExt]
	// caller's responsibility to sanitize names
	
	if (!folder) {
		logMsg(1, "NULL folder");
		return NULL;
	} else if (!fileName) {
		logMsg(1, "NULL file base name");
		return NULL;
	} else if (!fileExt) {
		logMsg(1, "NULL file extension");
		return NULL;
	}

	size_t folderLen = strlen(folder);
	size_t extLen = strlen(fileExt);
	size_t fileNameLen = strlen(fileName);

	// +1 for null terminator and +1 for path separator
	size_t pathLen = folderLen + 1 + fileNameLen + extLen + 1;

	char *filePath;

	if (fileName[0])
		filePath = ecalloc(pathLen, sizeof(char));
	else {
		logMsg(1, "Invalid filename.\n");
		return NULL;
	}

	memcpy(filePath, folder, folderLen * sizeof(char));

	filePath[folderLen] = fsep();
	filePath[pathLen - 1] = '\0';

	memcpy(filePath + folderLen + 1, fileName, fileNameLen * sizeof(char));
	memcpy(filePath + pathLen - extLen - 1, fileExt, extLen * sizeof(char));

	FILE *itemFile = fopen(filePath, "a");
	free (filePath);
	free (fileName);

	return itemFile;
}

static void
outputHtml(itemStruct *item, FILE *f)
{
	fprintf(f, "<h1>%s</h1><br>\n", item->title);
	fprintf(f, "<a href=\"%s\">Link</a><br>\n", san(item->link));
	fprintf(f, "%s", san(item->description));
}

void
itemAction(itemStruct *item, const char *folder)
{
	// Receives a linked list of articles to process.
	
	itemStruct *cur = item;
	itemStruct *prev;

	unsigned long long int newItems = 0;

	while (cur) {
		prev = cur;
		FILE *itemFile = openFile(folder, san(cur->title), ".html");

		// Do not overwrite files
		if (!ftell(itemFile)) {
			outputHtml(cur, itemFile);
			newItems++;
		}

		fclose(itemFile);
		cur = cur->next;
		freeItem(prev);
	}

	if (newItems)
		logMsg(2, "%s : %d new articles\n", folder, newItems);
}

void
finish(char *url, long responseCode)
{
	// Executed after a download finishes

	if (responseCode == 200)
		logMsg(4, "Finished downloading %s\n", url);
	else if (!responseCode)
		logMsg(1, "Can not reach %s: ensure the protocol is enabled and the site is accessible.\n", url);
	else
		logMsg(1, "HTTP %ld for %s\n", responseCode, url);
}
