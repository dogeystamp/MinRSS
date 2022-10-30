#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#ifdef JSON
#include <json-c/json.h>
#endif // JSON

#include "config.h"
#include "util.h"
#include "handlers.h"

void
freeItem(itemStruct *item)
{
	for (int i = 0; i < FIELD_END; i++) {
		if (item->fields[i])
			free(item->fields[i]);
	}
	free(item);
}

static inline int
propIs(xmlChar *prop, char *name)
{
	return !xmlStrcmp(prop, (const xmlChar *) name);
}

static void
allocField(char **field, char *str)
{
	size_t len = strlen(str) + 1;
	char *fieldStr = ecalloc(len, sizeof(char));
	memcpy(fieldStr, str, len * sizeof(char));
	*field = fieldStr;
}

void
copyField(itemStruct *item, enum fields field, char *str)
{
	allocField(&item->fields[field], str);
}

int
atomLink(itemStruct *item, xmlNodePtr node)
{
	xmlChar *href = xmlGetProp(node, (xmlChar *) "href");
	xmlChar *rel = xmlGetProp(node, (xmlChar *) "rel");

	if (!href) {
		logMsg(1, "Invalid link tag.\n");
		if (rel)
			xmlFree(rel);
		return 1;
	}

	if (!rel || propIs(rel, "alternate")) {
		copyField(item, FIELD_LINK, (char *)href);
	} else if (propIs(rel, "enclosure")) {
		copyField(item, FIELD_ENCLOSURE_URL, (char *)href);
	}

	xmlFree(href);
	xmlFree(rel);
	
	return 0;
}

int
rssEnclosure(itemStruct *item, xmlNodePtr node)
{
	xmlChar *href = xmlGetProp(node, (xmlChar *) "url");
	if (!href) {
		logMsg(1, "Invalid enclosure URL.\n");
		return 1;
	}

	copyField(item, FIELD_ENCLOSURE_URL, (char *)href);

	xmlFree(href);
	
	return 0;
}

FILE *
openFile(const char *folder, char *fileName, char *fileExt)
{
	// [folder]/[fileName][fileExt]
	// caller's responsibility to sanitize names, but frees fileName
	
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
	if (item->fields[FIELD_TITLE])
		fprintf(f, "<h1>%s</h1><br>\n", item->fields[FIELD_TITLE]);
	if (item->fields[FIELD_LINK])
		fprintf(f, "<a href=\"%s\">Link</a><br>\n", item->fields[FIELD_LINK]);
	if (item->fields[FIELD_ENCLOSURE_URL])
		fprintf(f, "<a href=\"%s\">Enclosure</a><br>\n", item->fields[FIELD_ENCLOSURE_URL]);
	if (item->fields[FIELD_DESCRIPTION])
		fprintf(f, "%s", item->fields[FIELD_DESCRIPTION]);
}

#ifdef JSON
static void
outputJson(itemStruct *item, FILE *f)
{
	json_object *root = json_object_new_object();

	if (item->fields[FIELD_TITLE])
		json_object_object_add(root, "title",
				json_object_new_string(item->fields[FIELD_TITLE]));

	if (item->fields[FIELD_LINK])
		json_object_object_add(root, "link",
				json_object_new_string(item->fields[FIELD_LINK]));

	if (item->fields[FIELD_ENCLOSURE_URL]) {
		json_object *enclosure = json_object_new_object();
		json_object_object_add(enclosure, "link",
				json_object_new_string(item->fields[FIELD_ENCLOSURE_URL]));
		json_object_object_add(root, "enclosure", enclosure);
	}

	if (item->fields[FIELD_DESCRIPTION])
		json_object_object_add(root, "description",
				json_object_new_string(item->fields[FIELD_DESCRIPTION]));

	fprintf(f, "%s", json_object_to_json_string_ext(root, 0));
	json_object_put(root);
}
#endif // JSON

void
itemAction(itemStruct *item, const char *folder)
{
	// Receives a linked list of articles to process.
	
	itemStruct *cur = item;
	itemStruct *prev;

	unsigned long long int newItems = 0;

	while (cur) {
		prev = cur;

		char fileExt[10];
		void (*outputFunction)(itemStruct *, FILE *);

		switch (outputFormat) {
			case OUTPUT_HTML:
				memcpy(fileExt, ".html", 6);
				outputFunction = &outputHtml;
				break;
#ifdef JSON
			case OUTPUT_JSON:
				memcpy(fileExt, ".json", 6);
				outputFunction = &outputJson;
				break;
#endif //JSON
	   		
			default:
				logMsg(0, "Output format is invalid.");
				break;
		}
		
		FILE *itemFile = openFile(folder, san(cur->fields[FIELD_TITLE]), fileExt);

		// Do not overwrite files
		if (!ftell(itemFile)) {
			outputFunction(cur, itemFile);
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
