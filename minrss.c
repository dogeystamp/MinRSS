/*

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see https://www.gnu.org/licenses/.

© 2021 dogeystamp <dogeystamp@disroot.org>
*/

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

	size_t folderLen = strlen(folder);
	size_t extLen = strlen(fileExt);

	while (cur) {
		char *filePath;
		char *fileName = san(cur->title, 1);
		size_t fileNameLen = strlen(fileName);

		itemStruct *prev = cur;


		// +1 for null terminator and +1 for path separator
		size_t pathLen = folderLen + fileNameLen + extLen + 2;

		if (fileName[0])
			filePath = ecalloc(pathLen, sizeof(char));
		else {
			logMsg(1, "Invalid article title.\n");

			cur = cur->next;
			freeItem(prev);

			continue;
		}

		memcpy(filePath, folder, folderLen * sizeof(char));

		filePath[folderLen] = fsep();
		filePath[pathLen] = '\0';

		memcpy(filePath + folderLen + 1, fileName, fileNameLen * sizeof(char));
		free(fileName);

		memcpy(filePath + pathLen - extLen - 1, fileExt, extLen * sizeof(char));

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
