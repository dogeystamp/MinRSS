#!/bin/sh

set -e

scriptname=$0
subcmd=$1

if [ -z "$MRSS_DIR" ]; then
	MRSS_DIR="$HOME/rss"
fi
mkdir -p "$MRSS_DIR"
if [ -z "$MRSS_NEWDIR" ]; then
	MRSS_NEWDIR="$MRSS_DIR/new"
fi
if [ -z "$MRSS_WATCH_LATER" ]; then
	MRSS_WATCH_LATER="$MRSS_DIR/watch-later"
fi
mkdir -p "$MRSS_WATCH_LATER"

sub_help() {
	echo "usage:"
	echo "  $scriptname <command> [options]"
	echo
	echo "commands:"
	echo "  update               updates feeds"
	echo "  fzf                  show articles using fzf"
	echo "                         use the commands /read (enter), /purge (ctrl-d), /purge-all (ctrl-alt-d),"
	echo "                         /watch-later (ctrl-w) and /queue (ctrl-e)"
	echo "                         you can also 'fzf feed' for a specific feed,"
	echo "                         or 'fzf new/feed' for new articles in the feed,"
	echo "                         or 'fzf watch-later' for articles marked watch-later."
	echo "                         'fzf -s' shuffles the feed."
	echo
	echo "article commands (pass files as arguments):"
	echo "  read                 opens link from an article file in either a browser or mpv"
	echo "  link                 print the article link"
	echo "  purge                mark an article read, or mark all articles read (if no args passed)"
	echo "  watchlater           move articles to a watch-later directory"
	echo
	echo "The following is required to use this script:"
	echo " - jq"
	echo " - minrss compiled with:"
	echo "     * OUTPUT_JSON"
	echo "     * SUMMARY_FILES"
	echo
	echo "Set MRSS_DIR to control where feeds are downloaded, and MRSS_NEWDIR for new articles."
	echo "By default, MRSS_DIR=~/rss, and MRSS_NEWDIR=~/rss/new."
	echo "Use MRSS_WATCH_LATER to store articles for later viewing (default ~/rss/watchlater/.)"
}

sub_update() {
	cd "$MRSS_DIR"
	minrss | {
		while read -r ARTICLE; do
			DIRNAME="$(dirname "$ARTICLE")"
			BASENAME="$(basename "$ARTICLE")"
			mkdir -p "$MRSS_NEWDIR"/"$DIRNAME"
			ln -sr "$MRSS_DIR"/"$ARTICLE" "$MRSS_NEWDIR"/"$ARTICLE"
		done
	}
}

list_purge() {
	while read -r ARTICLE; do
		if [ -h "$ARTICLE" ]; then
			rm "$ARTICLE"
			if [ -n "$INDXFILE" ]; then
				ENTRY="$(grep "^[0-9]*: $ARTICLE" "$INDXFILE")"
				ID="$(printf "%s" "$ENTRY" | grep -o "^[0-9]*")"
				sed -i "/^$ID:.*$/d" "$LISTFILE"
			fi
		fi
	done
}

sub_purge() {
	if [ -z "$@" ]; then
		cd "$MRSS_DIR"/new
		find . -type f | list_purge
	else
		for art in "$@"; do
			printf "%s\n" "$art"
		done | list_purge
	fi
}

list_link() {
	while read -r art; do
		LINK="$(cat "$art" | jq -r '.link // ""')"
		ENCLOSURE="$(cat "$art" | jq -r '.enclosure.link // ""')"
		ENCLOSURE_TYPE="$(cat "$art" | jq -r '.enclosure.type // ""')"

		if [ -z "$ENCLOSURE" ]; then
			printf "%s\n" "$LINK"
		else
			if printf "%s" "$ENCLOSURE_TYPE" | grep --quiet "^audio/.*" ; then
				# this is a podcast so use enclosure instead of link
				printf "%s\n" "$ENCLOSURE"
			else
				# some people put random images in enclosure
				printf "%s\n" "$LINK"
			fi
		fi
	done
}

sub_link() {
	for art in "$@"; do
		printf "%s\n" "$art"
	done | list_link
}

list_read() {
	VID=""
	VIDFILES=""

	while read -r art; do
		LINK="$(sub_link "$art")"
		ENCLOSURE_TYPE="$(cat "$art" | jq -r '.enclosure.type // ""')"

		if [ ! -z "$(printf "%s" "$LINK" | grep 'youtube.com\|odycdn\|twitch\|www.ted.com')" ] \
			|| [ "$ENCLOSURE_TYPE" = "audio/mpeg" ]
		then
			VID="$VID$LINK "
			if [ -n "$VIDFILES" ]; then
				VIDFILES=$(printf "%s\n%s" "$VIDFILES" "$art")
			else
				VIDFILES="$art"
			fi
		else
			xdg-open "$LINK" 2> /dev/null &
			sub_purge "$art"
		fi
	done

	if [ -n "$VID" ]; then
		if mpv $VID 2> /dev/null; then
			printf "%s\n" "$VIDFILES" | list_purge
		else
			printf "\n%s%s%s\n" \
				$blue \
				"mrss: Non-zero return code from mpv, not marking video files as read" \
				$normal
		fi
	fi
}

sub_read() {
	for art in "$@"; do
		printf "%s\n" "$art"
	done | list_read
}

sub_preview() {
	blue=$(tput setaf 4)
	normal=$(tput sgr0)

	ARTICLE="$1"

	DIRNAME="$(cat "$ARTICLE" | jq -r '.feedname // ""')"
	if [ -z "$DIRNAME" ]; then
		DIRNAME="$(basename $(dirname "$ARTICLE"))"
	fi

	TITLE="$(cat "$ARTICLE" | jq -r '.title')"
	DESC_TRUNC="$(cat "$ARTICLE" | jq -r '.description // ""' | w3m -dump -T text/html | head -n 20)"

	printf "\nFeed '%s'\n" "$DIRNAME"
	printf "\n%s%s%s\n" "$blue" "$TITLE" "$normal"

	printf "\n%s\n" "$DESC_TRUNC"
}

list_watchlater() {
	while read -r ARTICLE; do
		REALPATH="$(realpath "$ARTICLE")"
		sub_purge "$ARTICLE"
		ln -sr "$REALPATH" "$MRSS_WATCH_LATER"/
	done
}

sub_watchlater() {
	for art in "$@"; do
		printf "%s\n" "$art"
	done | list_watchlater
}

list_fzf_filename() {
	# searches entry in index and returns its filename
	while read -r ENTRY; do
		ID="$(printf "%s" "$ENTRY" | grep -o "^[0-9]*")"
		grep "^$ID: " "$INDXFILE" | sed "s/^[0-9]*: //"
	done
}

sub_fzf_preview() {
	# wrapper over sub_preview() that runs fzf_filename
	INDXFILE="$1"
	ENTRY="$2"
	sub_preview "$(echo "$ENTRY" | list_fzf_filename "$INDXFILE")"
}

mrss_find() {
	# a meta-feed should only have directory links
	# if there is a file link, use the way faster approach to list files
	if [ -f "$(realpath "$(ls | head -n 1)")" ]; then
		find . -mindepth 1 -maxdepth 1 -or -type f -or -type l
		return
	fi

	# follow directory links, but not file links
	# this means you can make your own tag folders that link to specific feeds
	# we don't follow file links because mrss purge only deletes links from new/

	find . -mindepth 1 -maxdepth 1 -type d -or -type f -or -type l | \
	(while read -r f; do
		# this loop finds directories (symlink or real)
		# regular files go through this pipeline unaffected
		# symlink files are discarded
		if [ -f "$f" ] || [ -d "$f" ]; then
			printf "%s\n" "$f"
		elif [ -h "$f" ]; then
			if [ -d "$(realpath "$f")" ]; then
				printf "%s\n" "$f"
			fi
		fi
	done) | \
	(while read -r f; do
		# this loop lists directories
		find -H "$f" -type f -or -type l
	done)
}

sub_fzf() {
	while getopts ":s" flag; do
		case "$flag" in
			s) MRSS_SHUF="yes";;
		esac
	done
	shift `expr $OPTIND - 1`

	if [ -z "$1" ]; then
		DIR="$MRSS_NEWDIR"
	else
		DIR="$MRSS_DIR/$1"
	fi
	cd "$DIR"

	LISTFILE=`mktemp --suffix=mrss`
	INDXFILE=`mktemp --suffix=mrss`

	COUNTER="1"
	mrss_find \
		| ([ -n "$MRSS_SHUF" ] && shuf || cat) \
		| nl -ba -d'' -n'rz' -s': ' -w1 \
		> "$INDXFILE"

	cat "$INDXFILE" | sed "s/^[0-9]*: //" \
		| xargs -rd "\n" cat \
		| jq -r '[.feedname?, .title] | map(select(. != null)) | join(" - ") | gsub("\n"; " ")' \
		| nl -ba -d'' -n'rz' -s': ' -w1 \
		> "$LISTFILE"

	while true; do
		OUTPUT="$(cat "$LISTFILE" |
			fzf --marker='*' --multi --print-query \
				--preview "cd $DIR; mrss fzf_preview '$INDXFILE' {}" \
				--bind "ctrl-d:change-query(/purge)+accept" \
				--bind "ctrl-alt-d:change-query(/purge-all)+accept" \
				--bind "enter:change-query(/read)+accept" \
				--bind "ctrl-w:change-query(/watch-later)+accept" \
				--bind "ctrl-e:change-query(/queue)+accept"
			)" || break
			# the break above is necessary with set -e
			# otherwise bash just exits

		if [ -z "$OUTPUT" ]; then
			break
		fi

		SEL="$(printf "$OUTPUT\n" | tail -n+2 | list_fzf_filename)"
		ACTION="$(printf "%s" "$OUTPUT" | head -n 1 | tail -c+2)"

		case "$ACTION" in
			read ) printf '%s\n' "$SEL" | list_read;;
			purge-all ) sub_purge; break;;
			purge ) printf '%s\n' "$SEL" | list_purge;;
			watch-later ) printf '%s\n' "$SEL" | list_watchlater;;
			queue )
				# setting queue like this because piping to a while loop
				# will make a subshell and a subshell can't modify
				# variables in the parent
				QUEUE="$(printf "%s\n" "$SEL" | \
					# another subshell so printf shares QUEUE with the while
					(
						while read -r ARTICLE; do
							REALPATH="$(realpath "$ARTICLE")"
							if [ -n "$QUEUE" ]; then
								QUEUE=$(printf "%s\n%s" "$QUEUE" "$REALPATH")
							else
								QUEUE="$REALPATH"
							fi
						done
						printf "%s" "$QUEUE"
					)
				)"
				printf '%s\n' "$SEL" | list_purge
				;;
		esac
	done

	rm "$LISTFILE"
	rm "$INDXFILE"

	if [ -n "$QUEUE" ]; then
		printf "%s\n" "$QUEUE" | list_read
	fi
}

case $subcmd in
	"" | "--help" | "-h")
		sub_help
		;;
	*)
		shift
		sub_${subcmd} "$@"
		if [ $? = 127 ]; then
			echo "error: unknown command '$subcmd'"
			exit 1
		fi
		;;
esac
