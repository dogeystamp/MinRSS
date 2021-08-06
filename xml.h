typedef struct itemStruct itemStruct;
struct itemStruct {
	char *title;
	char *link;
	char *description;
	char *author;
	char *categories;
	char *comments;
	char *enclosureUrl;
	char *enclosureType;
	unsigned long long enclosureLen;
	char *guid;
	char *pubDate;
	char *sourceName;
	char *sourceUrl;
	itemStruct *next;
};

void freeItem(itemStruct *item);

int readDoc(
    char *content,
    const char *feedName,
    void itemAction(itemStruct *, char const *chanTitle));
