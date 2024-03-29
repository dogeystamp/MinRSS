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
#include <time.h>
#include <utime.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "util.h"
#include "net.h"
#include "handlers.h"
#include "config.h"

static inline int
tagIs(xmlNodePtr node, char *str)
{
	return !xmlStrcmp(node->name, (const xmlChar *) str);
}

static int
parseXml(xmlDocPtr doc,
         const char *feedName,
         void itemAction(itemStruct *, const char *))
{
	// Parse the XML in a single document.

	if (!feedName || !feedName[0]) {
		logMsg(LOG_ERROR, "Missing feed name, please set one.\n");
		return 1;
	}

	xmlNodePtr rootNode;

	rootNode = xmlDocGetRootElement(doc);

	if (!rootNode) {
		logMsg(LOG_ERROR, "Empty document for feed.\n");
		return 1;
	}

	enum feedFormat format = NONE;

	if (tagIs(rootNode, "rss")) {
		format = RSS;
	} else if (tagIs(rootNode, "feed")) {
		if (!xmlStrcmp(rootNode->ns->href, (const xmlChar *) "http://www.w3.org/2005/Atom"))
			format = ATOM;
	}

	
	if (format == NONE) {
		logMsg(LOG_ERROR, "XML document is not an RSS or Atom feed.\n");
		return 1;
	}

	// Pointer to the first child of the root XML node
	xmlNodePtr cur = rootNode->children;

	switch (format) {
		case RSS:
			// Get channel XML tag
			while(cur && !tagIs(cur, "channel"))
				cur = cur->next;

			if (!cur || !tagIs(cur, "channel")) {
				logMsg(LOG_ERROR, "Invalid RSS syntax.\n");
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
			logMsg(LOG_ERROR, "Missing starting tag for format\n");
			return 1;
	}

	// Previous item (to build a linked list later)
	itemStruct *prev = NULL;

	// Loop over articles (skipping non-article tags)
	while (cur) {

		short isArticle = 0;

		switch (format) {
			case RSS:
				isArticle = tagIs(cur, "item");
				break;
			case ATOM:
				isArticle = tagIs(cur, "entry");
				break;
			default:
				logMsg(LOG_ERROR, "Missing article tag name for format\n");
				return 1;
		}

		if (isArticle) {
			itemStruct *item = ecalloc(1, sizeof(itemStruct));

			// Build a linked list of item structs to pass to itemAction()
			item->next = prev;
			prev = item;

			xmlNodePtr itemNode = cur->children;

			// Value within the tag
			char *itemKey;

			while (itemNode) {
				itemKey = (char *)xmlNodeListGetString(doc, itemNode->children, 1);

				switch (format) {
					case RSS:
						if (tagIs(itemNode, "link"))
							copyField(item, FIELD_LINK, itemKey);
						else if (tagIs(itemNode, "description"))
							copyField(item, FIELD_DESCRIPTION, itemKey);
						else if (tagIs(itemNode, "title"))
							copyField(item, FIELD_TITLE, itemKey);
						else if (tagIs(itemNode, "enclosure"))
							rssEnclosure(item, itemNode);
						break;
					case ATOM:
						if (tagIs(itemNode, "link"))
							atomLink(item, itemNode);
						else if (tagIs(itemNode, "content"))
							copyField(item, FIELD_DESCRIPTION, itemKey);
						else if (tagIs(itemNode, "title"))
							copyField(item, FIELD_TITLE, itemKey);
						break;
					default:
						break;
				}

				xmlFree(itemKey);
				
				itemNode = itemNode->next;
			}
		}

		cur = cur->next;
	}

	errno = 0;
	int stat = mkdir((const char* ) feedName, S_IRWXU);

	if (!stat && errno && errno != EEXIST) {
		logMsg(LOG_ERROR, "Error creating directory for feed.\n");
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
		logMsg(LOG_ERROR, "XML parser error.\n");
		return 1;
	}

	int stat = parseXml(doc, feedName, itemAction);

	if (stat)
		logMsg(LOG_ERROR, "Skipped feed %s due to errors.\n", feedName);

	xmlFreeDoc(doc);

	return stat;
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		logMsg(LOG_FATAL, "MinRSS %s\n", VERSION);
	else if (argc != 1)
		logMsg(LOG_FATAL, "Usage: minrss [-v]\n");

	unsigned int i = 0;

	initCurl();

	outputStruct outputs[LEN(links)];
	memset(outputs, 0, sizeof(outputs));

	time_t timeNow = time(NULL);

	for (i = 0; i < LEN(links); i++) {
		struct stat feedDir;

		if (links[0].url[0] == '\0')
			logMsg(LOG_FATAL, "No feeds, add them in config.def.h\n");

		if (stat(links[i].feedName, &feedDir) == 0) {
			time_t deltaTime = timeNow - feedDir.st_atime;
			if (deltaTime < links[i].update)
				continue;
		}

		logMsg(LOG_VERBOSE, "Requesting %s\n", links[i].url);
		createRequest(links[i].url, &outputs[i]);
	}

	performRequests(finish);

	logMsg(LOG_INFO, "Finished downloads.\n");

	for (i = 0; i < LEN(links); i++) {
		logMsg(LOG_VERBOSE, "Parsing %s\n", links[i].url);

		if (outputs[i].buffer && outputs[i].buffer[0]) {
			if (readDoc(outputs[i].buffer, links[i].feedName, itemAction) == 0) {
				struct stat feedDir;

				if (stat(links[i].feedName, &feedDir) == 0) {
					struct utimbuf update;

					update.actime = timeNow;
					update.modtime = feedDir.st_mtime;
					utime(links[i].feedName, &update);
				}
			}
			free(outputs[i].buffer);
		}
	}

	logMsg(LOG_INFO, "Finished parsing feeds.\n");

	return 0;
}
