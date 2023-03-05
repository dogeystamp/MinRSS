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
	echo "  read                 opens link from an article file in either a browser or mpv"
	echo "  select               show each new article and prompt for an action;"
	echo "                         you can run 'select new/feed' for a specific feed"
	echo "                         or 'select watch-later'."
	echo "  fzf                  select articles to read using fzf"
	echo "  link                 print the article link from a .json file"
	echo "  purge                purge new/ directory"
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

sub_purge() {
	cd "$MRSS_DIR"
	rm -r "$MRSS_NEWDIR"/*
}

sub_link() {
	# extract the link from a single article file
	cat "$@" | jq -r '.enclosure.link // .link'
}

list_read() {
	VID=""
	VIDFILES=""

	while read -r art; do
		LINK="$(sub_link "$art")"
		if [ ! -z "$(printf "%s" "$LINK" | grep 'youtube.com\|odycdn\|simplecastaudio\|podcasts\|twitch')" ]; then
			VID="$VID$LINK "
			if [ -n "$VIDFILES" ]; then
				VIDFILES=$(printf "%s\n%s" "$VIDFILES" "$art")
			else
				VIDFILES="$art"
			fi
		else
			xdg-open $LINK 2> /dev/null &
			if [ -h "$art" ]; then
				# remove symlinks from new/
				rm "$art"
			fi
		fi
	done

	if [ -n "$VID" ]; then
		if mpv $VID 2> /dev/null; then
			printf "%s" "$VIDFILES" | xargs -d "\n" rm
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
		echo "$art"
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

sub_select() {
	if [ -z "$1" ]; then
		DIR="$MRSS_NEWDIR"
	else
		DIR="$MRSS_DIR/$1"
	fi
	NEWARTS="$(find "$DIR" -type l)"
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
		printf "\nq quit, r read, e queue article, f full summary, d mark read,\n"
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
					if [ -h "$ARTICLE" ]; then
						rm "$ARTICLE"
					fi
					break;;
				s ) break;;
				S ) SKIPALL="y"; break;;
				w ) 
					REALPATH="$(realpath "$ARTICLE")"
					rm "$ARTICLE"
					ln -sr "$REALPATH" "$MRSS_WATCH_LATER"/
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
	NEWARTS="$(find . -type l)"
	export -f sub_preview
	printf "%s" "$NEWARTS" | fzf --disabled --marker='*' --multi --cycle --preview 'bash -c "sub_preview {}"' | \
		list_read
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
