#!/bin/sh

set -e

scriptname=$0
subcmd=$1

blue=$(tput setaf 4)
normal=$(tput sgr0)

if [ -z "$MRSS_DIR" ]; then
	MRSS_DIR="$HOME/rss"
fi
mkdir -p "$MRSS_DIR"
if [ -z "$MRSS_NEWDIR" ]; then
	MRSS_NEWDIR="$MRSS_DIR/new"
fi

sub_help() {
	echo "usage:"
	echo "  $scriptname <command> [options]"
	echo
	echo "commands:"
	echo "  update               updates feeds"
	echo "  read                 opens link from an article file in either a browser or mpv"
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

sub_select() {
	NEWARTS="$(find "$MRSS_NEWDIR" -type l)"
	TOTAL_COUNT="$(printf "%s" "$NEWARTS" | wc -l)"
	printf "%s" "$NEWARTS" | (

	INDEX=0
	while read -r ARTICLE; do
		if [ -n "$SKIPALL" ]; then
			continue
		fi

		INDEX=$((INDEX+1))
		clear

		DIRNAME="$(basename $(dirname "$ARTICLE"))"
		TITLE="$(cat "$ARTICLE" | jq -r '.title')"
		DESC_TRUNC="$(cat "$ARTICLE" | jq -r '.description // ""' | w3m -dump -T text/html | head -n 20)"

		printf "\nItem %s/%s\n" "$INDEX" "$TOTAL_COUNT"
		printf "\nFeed '%s'\n" "$DIRNAME"
		printf "\n%s%s%s\n" "$blue" "$TITLE" "$normal"

		printf "\n%s\n" "$DESC_TRUNC"

		printf "\n\n-----------------\n"
		printf "\nq quit, r read, e queue article, f full summary, d mark read,\n"
		printf "s skip, S skip all\n"

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
				* ) break;;
			esac
		done
	done &&
	if [ -n "$QUEUE" ]; then
		printf "%s\n" "$QUEUE" | list_read 
	fi
	)
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
