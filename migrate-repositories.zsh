#!/usr/bin/env zsh

set -e

BASE=~/projects/bibledit
TARGET=~/scratch/bibledit

function show-authors() {
	git log --format='%an <%ae>' | sort | uniq -c | sort -n
}

function normalize-authors () {
	show-authors
	git filter-branch -f --commit-filter '
		if  [[ $GIT_AUTHOR_EMAIL =~ teus* ]] || [[ $GIT_AUTHOR_EMAIL =~ translation* ]]; then
			GIT_AUTHOR_NAME="Teus Benschop";
			GIT_AUTHOR_EMAIL="teusjannette@gmail.com";
		fi;
		if  [[ $GIT_AUTHOR_EMAIL =~ dand* ]]; then
			GIT_AUTHOR_NAME="Dan Dennison";
			GIT_AUTHOR_EMAIL="dan@thedennisons.org";
		fi;
		GIT_COMMITTER_NAME="Caleb Maclennan";
		GIT_COMMITTER_EMAIL="caleb@alerque.com";
		git commit-tree "$@";
	'
	show-authors
}

pushd $TARGET

[[ -d bibledit-core ]] || git clone --branch=master $BASE bibledit-core
pushd bibledit-core
normalize-authors
git log -1
popd

ls -al
