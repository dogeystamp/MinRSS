typedef struct itemStruct itemStruct;
struct itemStruct {
	char *title;
	char *link;
	char *description;
	itemStruct *next;
};

void freeItem(itemStruct *item);

int readDoc(
    char *content,
    const char *feedName,
    void itemAction(itemStruct *, char const *chanTitle));
