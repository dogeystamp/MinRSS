/*

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see https://www.gnu.org/licenses/.

© 2021 dogeystamp <dogeystamp@disroot.org>
*/

#include <curl/curl.h>

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
 * 2: Normal output
 * 3: Info messages
 * 4: Verbose mode
*/

static const int logLevel = 2;

// Set the maximum amount of redirects curl will follow.
// Use 0 to disable redirects, and -1 for no limit.
static const int maxRedirs = 10;

// Restrict allowed protocols for curl using a bitmask.
// For more information: https://curl.se/libcurl/c/CURLOPT_PROTOCOLS.html
static const int curlProtocols = CURLPROTO_HTTPS | CURLPROTO_HTTP;

enum outputFormats {
	OUTPUT_HTML,
#ifdef JSON
	OUTPUT_JSON,
#endif // JSON
};

// When saving, sets the format of the saved file.
static const enum outputFormats outputFormat = OUTPUT_HTML;
