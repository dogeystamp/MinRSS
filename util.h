#define LEN(X) (sizeof(X) / sizeof(X[0]))

void logMsg(int argc, char *msg, ...);
void *ecalloc(size_t nmemb, size_t size);
void *erealloc(void *p, size_t nmemb);
char *san(char *str, int rep);
char fsep();
