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

void freeItem(itemStruct *item);
void itemAction(itemStruct *item, const char *folder);
void finish(char *url, long responseCode);
