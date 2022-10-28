/*

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see https://www.gnu.org/licenses/.

© 2021 dogeystamp <dogeystamp@disroot.org>
*/

#define LEN(X) (sizeof(X) / sizeof(X[0]))

void logMsg(int argc, char *msg, ...);
void *ecalloc(size_t nmemb, size_t size);
void *erealloc(void *p, size_t nmemb);
char *san(char *str);
char fsep();
