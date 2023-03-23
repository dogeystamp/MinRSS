/*

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see https://www.gnu.org/licenses/.

© 2022 dogeystamp <dogeystamp@disroot.org>
*/

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
	FIELD_ENCLOSURE_TYPE,

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
