#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "util.h"
#include "xml.h"

#define TAGIS(X, Y) (!xmlStrcmp(X->name, (const xmlChar *) Y))

void
freeItem(itemStruct *item)
{
	// Free the memory used by an article.

	char **mems[] = {
		&item->title,
		&item->link,
		&item->description,
		&item->author,
		&item->comments,
		&item->guid,
		&item->pubDate,
		&item->sourceName,
		&item->sourceUrl,
		&item->categories,
		&item->enclosureUrl,
		&item->enclosureType,
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
		logMsg(1, "Empty document for feed. Skipping...\n");
		return 1;
	}

	if (!TAGIS(rootNode, "rss")) {
		logMsg(1, "XML document is not an RSS feed. Skipping...\n");
		return 1;
	}

	xmlChar *key;

	// Get channel XML tag
	xmlNodePtr channel = rootNode->children;

	while(channel && !TAGIS(channel, "channel"))
		channel = channel->next;

	if (!channel || !TAGIS(channel, "channel")) {
		logMsg(1, "Invalid RSS syntax. Skipping...\n");
	}

	// Pointer to an article xml tag
	xmlNodePtr cur = channel->children;

	itemStruct *prev = NULL;

	while (cur) {

		key = xmlNodeListGetString(doc, cur->children, 1);

		if (TAGIS(cur, "item")) {
			itemStruct *item = ecalloc(1, sizeof(itemStruct));

			// Build a linked list of item structs to pass to itemAction()
			item->next = prev;
			prev = item;

			xmlNodePtr itemNode = cur->children;

			while (itemNode) {
				char *itemKey = (char *)xmlNodeListGetString(doc, itemNode->children, 1);

				char *attKeys[] = {
					"title",
					"link",
					"description",
					"author",
					"comments",
					"guid",
					"pubDate",
					"source"
				};
				char **atts[] = {
					&item->title,
					&item->link,
					&item->description,
					&item->author,
					&item->comments,
					&item->guid,
					&item->pubDate,
					&item->sourceName,
				};

				if (itemKey) {
					for (unsigned long int i = 0; i < LEN(attKeys); i++) {
						if (TAGIS(itemNode, attKeys[i])) {
							*atts[i] = ecalloc(strlen(itemKey) + 1, sizeof(char));

							strcpy(*atts[i], itemKey);
						}
					}

					if (TAGIS(itemNode, "category")) {
						if (item->categories) {
							erealloc(item->categories,
							         strlen(item->categories) + strlen(itemKey) + 2);

							strcat(item->categories, " ");
							strcat(item->categories, itemKey);
						} else {
							item->categories = ecalloc(
							                       strlen(itemKey) + 2,
							                       sizeof(char));
							strcpy(item->categories, itemKey);
						}
					}

					if (TAGIS(itemNode, "enclosure")) {
						item->enclosureUrl =
						    (char *) xmlGetProp(itemNode, (xmlChar *) "url");
						item->enclosureType =
						    (char *) xmlGetProp(itemNode, (xmlChar *) "type");

						char *endPtr;
						errno = 0;

						item->enclosureLen = strtoul(
						                         (char *) xmlGetProp(itemNode, (xmlChar *) "length"),
						                         &endPtr,
						                         10);

						if (errno)
							logMsg(1, "Invalid RSS: enclosure length is invalid.\n");
					}
					xmlFree(itemKey);
				}

				itemNode = itemNode->next;
			}
		}

		xmlFree(key);
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

	xmlFreeDoc(doc);

	return stat;
}
