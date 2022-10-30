enum feedFormat {
	NONE,
	RSS,
	ATOM,
};

enum fields {
	FIELD_TITLE,
	FIELD_LINK,
	FIELD_DESCRIPTION,
	FIELD_ENCLOSURE_URL,

	FIELD_END
};
enum numFields {
	// currently unimplemented
	NUM_ENCLOSURE_SIZE,

	NUM_END
};
typedef struct itemStruct itemStruct;
struct itemStruct {
	char *fields[FIELD_END];
	int numFields[NUM_END];
	itemStruct *next;
};

void copyField(itemStruct *item, enum fields field, char *str);

void freeItem(itemStruct *item);
void itemAction(itemStruct *item, const char *folder);
void finish(char *url, long responseCode);

int rssEnclosure(itemStruct *item, xmlNodePtr node);

int atomLink(itemStruct *item, xmlNodePtr node);
