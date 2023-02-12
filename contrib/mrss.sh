#!/bin/sh

set -e

scriptname=$0
subcmd=$1

if [ -z "$MRSS_DIR" ]; then
	MRSS_DIR="$HOME/rss"
fi
mkdir -p "$MRSS_DIR"

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
}

sub_update() {
	cd "$MRSS_DIR"
	minrss | {
		while read -r ARTICLE; do
			DIRNAME="$(dirname "$ARTICLE")"
			BASENAME="$(basename "$ARTICLE")"
			mkdir -p "$MRSS_DIR"/new/"$DIRNAME"
			ln -sr "$MRSS_DIR"/"$ARTICLE" "$MRSS_DIR"/new/"$ARTICLE"
		done
	}
}

sub_purge() {
	cd "$MRSS_DIR"
	rm -r "$MRSS_DIR"/new/*
}

sub_link() {
	# extract the link from a single article file
	cat "$@" | jq -r '.enclosure.link // .link'
}

sub_read() {
	VID=""

	for art in "$@"; do
		LINK="$(sub_link "$art")"
		if [ ! -z "$(printf "%s" "$LINK" | grep 'youtube.com\|odycdn\|simplecastaudio\|podcasts\|twitch')" ]; then
			VID="$VID$LINK "
		else
			xdg-open $LINK &
		fi
		if [ -h "$art" ]; then
			# remove symlinks from new/
			rm "$art"
		fi
	done

	if [ -n "$VID" ]; then
		mpv $VID
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
