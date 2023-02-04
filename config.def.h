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
	const time_t update;
} linkStruct;

/* Example link:
	{
		.url = "https://example.com/rss/feed.rss",
		// This will be used as the folder name for the feed.
		.feedName = "examplefeed",
		// The time in seconds between checks for updates.
		.update = 3600,
	},
*/

static const linkStruct links[] = {
	{
		.url = "",
		.feedName = "",
		.update = 0,
	},
};

enum logLevels {
	LOG_FATAL,
	LOG_ERROR,
	LOG_OUTPUT,
	LOG_INFO,
	LOG_VERBOSE,
};

// Print all messages at least as important as this level.
static const int logLevel = LOG_OUTPUT;

// Set the maximum amount of redirects curl will follow.
// Use 0 to disable redirects, and -1 for no limit.
static const int maxRedirs = 10;

// Restrict allowed protocols for curl.
// For more information: https://curl.se/libcurl/c/CURLOPT_PROTOCOLS_STR.html
static const char curlProtocols[] = "http,https";

enum outputFormats {
	OUTPUT_HTML,
#ifdef JSON
	OUTPUT_JSON,
#endif // JSON
};

// When saving, sets the format of the saved file.
static const enum outputFormats outputFormat = OUTPUT_HTML;
