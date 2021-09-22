typedef struct {
	const char *url;
	const char *feedName;
} linkStruct;

/* Example link:
	{
		.url = "https://example.com/rss/feed.rss",
		// This will be used as the folder name for the feed.
		.feedName = "examplefeed",
		.tags = "test example sample"
	},
*/

static const linkStruct links[] = {
	{
		.url = "",
		.feedName = "",
	},
};

/*
 * 0: Fatal errors
 * 1: Errors
 * 2: Warnings
 * 3: Info messages
 * 4: Verbose mode
*/

static const int logLevel = 3;

// Set the maximum amount of redirects curl will follow.
// Use 0 to disable redirects, and -1 for no limit.
static const int maxRedirs = 10;

// File extension used for each article.
static const char fileExt[] = ".html";
