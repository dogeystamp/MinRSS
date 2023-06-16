/*

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see https://www.gnu.org/licenses/.

© 2022 dogeystamp <dogeystamp@disroot.org>
*/

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
	if (!str) {
		logMsg(LOG_ERROR, "Attempted to assign a null pointer to a field!\n");
		return;
	}

	allocField(&item->fields[field], str);
}

int
atomLink(itemStruct *item, xmlNodePtr node)
{
	xmlChar *href = xmlGetProp(node, (xmlChar *) "href");
	xmlChar *rel = xmlGetProp(node, (xmlChar *) "rel");

	if (!href) {
		logMsg(LOG_ERROR, "Invalid link tag.\n");
		if (rel)
			xmlFree(rel);
		return 1;
	}

	if (!rel || propIs(rel, "alternate")) {
		copyField(item, FIELD_LINK, (char *)href);
	} else if (propIs(rel, "enclosure")) {
		copyField(item, FIELD_ENCLOSURE_URL, (char *)href);

		xmlChar *enclosure_type = xmlGetProp(node, (xmlChar *) "type");
		copyField(item, FIELD_ENCLOSURE_TYPE, (char *)enclosure_type);
		xmlFree(enclosure_type);
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
		logMsg(LOG_ERROR, "Invalid enclosure URL.\n");
		return 1;
	}

	copyField(item, FIELD_ENCLOSURE_URL, (char *)href);
	xmlFree(href);

	xmlChar *enclosure_type = xmlGetProp(node, (xmlChar *) "type");
	copyField(item, FIELD_ENCLOSURE_TYPE, (char *)enclosure_type);
	xmlFree(enclosure_type);
	
	return 0;
}

FILE *
openFile(const char *folder, char *fileName, char *fileExt)
{
	// [folder]/[fileName][fileExt]
	// caller's responsibility to sanitize names and free fileName
	
	if (!folder) {
		logMsg(LOG_ERROR, "NULL folder");
		return NULL;
	} else if (!fileName) {
		logMsg(LOG_ERROR, "NULL file base name");
		return NULL;
	} else if (!fileExt) {
		logMsg(LOG_ERROR, "NULL file extension");
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
		logMsg(LOG_ERROR, "Invalid filename.\n");
		return NULL;
	}

	memcpy(filePath, folder, folderLen * sizeof(char));

	filePath[folderLen] = fsep();
	filePath[pathLen - 1] = '\0';

	memcpy(filePath + folderLen + 1, fileName, fileNameLen * sizeof(char));
	memcpy(filePath + pathLen - extLen - 1, fileExt, extLen * sizeof(char));

	FILE *itemFile = fopen(filePath, "a");
	free (filePath);

	return itemFile;
}

static void
outputHtml(itemStruct *item, FILE *f, const char *folder)
{
	if (item->fields[FIELD_TITLE])
		fprintf(f, "<h1>%s</h1><br>\n", item->fields[FIELD_TITLE]);

	fprintf(f, "From feed <b>%s</b><br>\n", folder);

	if (item->fields[FIELD_LINK])
		fprintf(f, "<a href=\"%s\">Link</a><br>\n", item->fields[FIELD_LINK]);
	if (item->fields[FIELD_ENCLOSURE_URL])
		fprintf(f, "<a href=\"%s\">Enclosure</a><br>\n", item->fields[FIELD_ENCLOSURE_URL]);
	if (item->fields[FIELD_ENCLOSURE_TYPE])
		fprintf(f, "Enclosure type: %s\n", item->fields[FIELD_ENCLOSURE_TYPE]);
	if (item->fields[FIELD_DESCRIPTION])
		fprintf(f, "%s", item->fields[FIELD_DESCRIPTION]);
}

#ifdef JSON
static void
outputJson(itemStruct *item, FILE *f, const char *folder)
{
	json_object *root = json_object_new_object();

	json_object_object_add(root, "feedname",
			json_object_new_string(folder));

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
		if (item->fields[FIELD_ENCLOSURE_TYPE]) {
			json_object_object_add(enclosure, "type",
					json_object_new_string(item->fields[FIELD_ENCLOSURE_TYPE]));
		}
		json_object_object_add(root, "enclosure", enclosure);
	}

	if (item->fields[FIELD_DESCRIPTION])
		json_object_object_add(root, "description",
				json_object_new_string(item->fields[FIELD_DESCRIPTION]));

	fprintf(f, "%s", json_object_to_json_string_ext(root, 0));
	json_object_put(root);
}
#endif // JSON

int
processItem(itemStruct *item, const char *folder)
{
	// Returns 1 if the article is new, 0 if not, -1 for error.

	int ret = 0;

	char fileExt[10];
	void (*outputFunction)(itemStruct *, FILE *, const char *);

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
			logMsg(LOG_FATAL, "Output format is invalid.\n");
			return -1;
	}
	
	char *basename = san(item->fields[FIELD_TITLE]);

	FILE *itemFile = openFile(folder, basename, fileExt);

	if (!itemFile) {
		logMsg(LOG_ERROR, "Could not open file '%s/%s/%s'.\n",
				folder,
				basename,
				fileExt
			);

		ret = -1;
		goto cleanup_basename;
	}

	// Do not overwrite files
	if (!ftell(itemFile)) {
		outputFunction(item, itemFile, folder);
		ret = 1;
		if (summaryFormat == SUMMARY_FILES)
			logMsg(LOG_OUTPUT, "%s%c%s%s\n", folder, fsep(), basename, fileExt);
	}

	fclose(itemFile);
cleanup_basename:
	free(basename);
	return ret;
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
		int res = processItem(cur, folder);
		if (res == 1)
			newItems++;
		cur = cur->next;
		freeItem(prev);
	}

	switch (summaryFormat) {
		case SUMMARY_HUMAN_READABLE:
			if (newItems)
				logMsg(LOG_OUTPUT, "%s : %d new articles\n", folder, newItems);
			break;
		case SUMMARY_FILES:
			// print output after saving each file
			break;
	}
}

void
finish(char *url, long responseCode)
{
	// Executed after a download finishes

	if (responseCode == 200)
		logMsg(LOG_VERBOSE, "Finished downloading %s\n", url);
	else if (!responseCode)
		logMsg(LOG_ERROR, "Can not reach %s: ensure the protocol is enabled and the site is accessible.\n", url);
	else
		logMsg(LOG_ERROR, "HTTP %ld for %s\n", responseCode, url);
}
