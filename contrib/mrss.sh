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
	echo "  select               show each new article and prompt for an action;"
	echo "                         you can run 'select new/feed' for a specific feed"
	echo "                         or 'select watch-later'."
	echo "  fzf                  show articles using fzf"
	echo "                         use the commands /read (enter), /purge (ctrl-d), /purge-all (ctrl-alt-d),"
	echo "                         /watch-later (ctrl-w) and /queue (ctrl-e)"
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
		fi
	done
}

sub_purge() {
	if [ -z "$@" ]; then
		cd "$MRSS_DIR"
		rm -r "$MRSS_NEWDIR"/*
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
			xdg-open $LINK 2> /dev/null &
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

	DIRNAME="$(basename $(dirname "$ARTICLE"))"
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

sub_select() {
	if [ -z "$1" ]; then
		DIR="$MRSS_NEWDIR"
	else
		DIR="$MRSS_DIR/$1"
	fi
	NEWARTS="$(find "$DIR" -type l -or -type f)"
	TOTAL_COUNT="$(printf "%s" "$NEWARTS" | wc -l)"
	printf "%s" "$NEWARTS" | (

	INDEX=0
	while read -r ARTICLE; do
		if [ -n "$SKIPALL" ]; then
			continue
		fi

		INDEX=$((INDEX+1))
		clear
		printf "\nItem %s/%s\n" "$INDEX" "$TOTAL_COUNT"
		sub_preview "$ARTICLE"

		printf "\n\n-----------------\n"
		printf "\nq quit, r read, e queue article, f full summary, d purge (mark read),\n"
		printf "s skip, S skip all, w watch later\n"

		while true; do
			printf "\n> "
			read ans </dev/tty
			case "$ans" in
				q ) exit;;
				r ) sub_read "$ARTICLE"; break;;
				e )
					if [ -n "$QUEUE" ]; then
						QUEUE=$(printf "%s\n%s" "$QUEUE" "$ARTICLE")
					else
						QUEUE="$ARTICLE"
					fi
					break;;
				f ) 
				 cat "$ARTICLE" | jq -r '.description // ""' | w3m -o confirm_qq=false -T text/html
				 ;;
				d )
					sub_purge "$ARTICLE"
					break;;
				s ) break;;
				S ) SKIPALL="y"; break;;
				w ) sub_watchlater "$ARTICLE"
					break;;
				* ) break;;
			esac
		done
	done &&
	if [ -n "$QUEUE" ]; then
		printf "%s\n" "$QUEUE" | list_read 
	fi
	)
}

sub_fzf() {
	if [ -z "$1" ]; then
		DIR="$MRSS_NEWDIR"
	else
		DIR="$MRSS_DIR/$1"
	fi
	cd "$DIR"

	while true; do
		NEWARTS="$(find . -type l -or -type f)"
		OUTPUT="$(printf "%s" "$NEWARTS" |
			export SHELL="/bin/sh"
			fzf --marker='*' --multi --print-query \
				--preview "cd $DIR; mrss preview {}" \
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

		SEL="$(printf "%s" "$OUTPUT" | tail -n+2)"
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
