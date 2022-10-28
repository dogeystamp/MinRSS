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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"
#include "util.h"
#include "net.h"

#define TAGIS(X, Y) (!xmlStrcmp(X->name, (const xmlChar *) Y))

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

static int
parseXml(xmlDocPtr doc,
         const char *feedName,
         void itemAction(itemStruct *, const char *))
{
	// Parse the XML in a single document.

	if (!feedName || !feedName[0]) {
		logMsg(1, "Missing feed name, please set one.\n");
		return 1;
	}

	xmlNodePtr rootNode;

	rootNode = xmlDocGetRootElement(doc);

	if (!rootNode) {
		logMsg(1, "Empty document for feed.\n");
		return 1;
	}

	enum feedFormat format = NONE;

	if (TAGIS(rootNode, "rss")) {
		format = RSS;
	} else if (TAGIS(rootNode, "feed")) {
		if (!xmlStrcmp(rootNode->ns->href, (const xmlChar *) "http://www.w3.org/2005/Atom"))
			format = ATOM;
	}

	
	if (format == NONE) {
		logMsg(1, "XML document is not an RSS or Atom feed.\n");
		return 1;
	}

	// Pointer to the first child of the root XML node
	xmlNodePtr cur = rootNode->children;

	switch (format) {
		case RSS:
			// Get channel XML tag
			while(cur && !TAGIS(cur, "channel"))
				cur = cur->next;

			if (!cur || !TAGIS(cur, "channel")) {
				logMsg(1, "Invalid RSS syntax.\n");
				return 1;
			}

			// Set cur to child of channel
			cur = cur->children;
			break;

		case ATOM:
			// Set cur to child of feed
			cur = rootNode->children;
			break;

		default:
			logMsg(1, "Missing starting tag for format\n");
			return 1;
	}

	// Previous item (to build a linked list later)
	itemStruct *prev = NULL;

	// Loop over articles (skipping non-article tags)
	while (cur) {

		short isArticle = 0;

		switch (format) {
			case RSS:
				isArticle = TAGIS(cur, "item");
				break;
			case ATOM:
				isArticle = TAGIS(cur, "entry");
				break;
			default:
				logMsg(1, "Missing article tag name for format\n");
				return 1;
		}

		if (isArticle) {
			itemStruct *item = ecalloc(1, sizeof(itemStruct));

			// The selected set of attribute keys
			char **attKeys;

			// Struct variables to map attributes to
			char **atts[] = {
				&item->title,
				&item->link,
				&item->description,
			};

			// Attribute keys for each format
			
			char *attKeysRss[LEN(atts)] = {
				"title",
				"link",
				"description",
			};

			char *attKeysAtom[LEN(atts)] = {
				"title",
				// link has special treatment because its value is in href not within the tag
				"",
				"content",
			};

			switch (format) {
				case RSS:
					attKeys = attKeysRss;
					break;

				case ATOM:
					attKeys = attKeysAtom;
					break;

				default:
					logMsg(1, "Missing article attribute keys for format\n");
					return 1;
			};

			// Build a linked list of item structs to pass to itemAction()
			item->next = prev;
			prev = item;

			xmlNodePtr itemNode = cur->children;

			// Value within the tag
			char *itemKey;

			while (itemNode) {
				itemKey = (char *)xmlNodeListGetString(doc, itemNode->children, 1);

				if (itemKey) {
					for (unsigned long int i = 0; i < LEN(atts); i++) {
						if (TAGIS(itemNode, attKeys[i])) {
							size_t keyLen = strlen(itemKey) + 1;
							*atts[i] = ecalloc(keyLen, sizeof(char));
							memcpy(*atts[i], itemKey, keyLen * sizeof(char));

							break;
						}
					}

					xmlFree(itemKey);
				}
				
				// Exceptions
				
				// Atom entry link tag
				if (format == ATOM && TAGIS(itemNode, "link")) {
					xmlChar *link = xmlGetProp(itemNode, (xmlChar *) "href");

					if (!link) {
						logMsg(1, "Missing Atom entry link\n");
						xmlFree(link);
						return 1;
					}

					size_t linkLen = strlen((char *) link) + 1;
					item->link = ecalloc(linkLen, sizeof(char));
					memcpy(item->link, (char *) link, linkLen * sizeof(char));

					xmlFree(link);
				}

				itemNode = itemNode->next;
			}
		}

		cur = cur->next;
	}

	errno = 0;
	int stat = mkdir((const char* ) feedName, S_IRWXU);

	if (!stat && errno && errno != EEXIST) {
		logMsg(1, "Error creating directory for feed.\n");
		return 1;
	}

	itemAction(prev, feedName);

	return 0;
}

int
readDoc(char *content,
        const char *feedName,
        void itemAction(itemStruct *, const char *))
{
	// Initialize the XML document, read it, then free it

	xmlDocPtr doc;

	doc = xmlReadMemory(content, strlen(content), "noname.xml", NULL, 0);
	if (!doc) {
		logMsg(1, "XML parser error.\n");
		return 1;
	}

	int stat = parseXml(doc, feedName, itemAction);

	if (stat)
		logMsg(1, "Skipped feed %s due to errors.\n", feedName);

	xmlFreeDoc(doc);

	return stat;
}

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
		filePath[pathLen - 1] = '\0';

		memcpy(filePath + folderLen + 1, fileName, fileNameLen * sizeof(char));
		memcpy(filePath + pathLen - extLen - 1, fileExt, extLen * sizeof(char));

		FILE *itemFile = fopen(filePath, "a");

		free(filePath);
		free(fileName);


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

int
main(int argc, char *argv[])
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
		logMsg(5, "Parsing %s\n", links[i].url);

		if (outputs[i].buffer && outputs[i].buffer[0]) {
			readDoc(outputs[i].buffer, links[i].feedName, itemAction);
			free(outputs[i].buffer);
		}
	}

	logMsg(3, "Finished parsing feeds.\n");

	return 0;
}
