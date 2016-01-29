#!/usr/bin/env zsh

set -e
#set -x

BASE=~/projects/bibledit
TARGET=~/scratch/bibledit


BE_CORE=bibledit-core

function show-authors() {
	git log --format='%an <%ae>' | sort | uniq -c | sort -n
}

function normalize-authors () {
	show-authors | grep -qi Compaq && \
		git filter-branch -f --commit-filter '
			if  [[ $GIT_AUTHOR_EMAIL =~ teus* ]] || [[ $GIT_AUTHOR_EMAIL =~ translation* ]]; then
				GIT_AUTHOR_NAME="Teus Benschop";
				GIT_AUTHOR_EMAIL="teusjannette@gmail.com";
			fi;
			if  [[ $GIT_AUTHOR_EMAIL =~ mattias* ]]; then
				GIT_AUTHOR_NAME="Mattias Põldaru";
				GIT_AUTHOR_EMAIL="mahfiaz@gmail.com";
			fi;
			if  [[ $GIT_AUTHOR_EMAIL =~ dand* ]]; then
				GIT_AUTHOR_NAME="Dan Dennison";
				GIT_AUTHOR_EMAIL="dan@thedennisons.org";
			fi;
			GIT_COMMITTER_NAME="$(git config user.name)";
			GIT_COMMITTER_EMAIL="$(git config user.email)";
			git commit-tree "$@";
		'
	show-authors
}

#rm -rf $TARGET ; mkdir $TARGET
pushd $TARGET

[[ -d $BE_CORE ]] || git clone --branch=master $BASE $BE_CORE
pushd $BE_CORE
normalize-authors
git log -1
popd

[[ -d bibledit-web ]] || git clone --branch=savannah/bibledit-web $BASE bibledit-web
pushd bibledit-web
normalize-authors
popd

[[ -d bibledit-osx ]] || git clone --branch=master $BE_CORE bibledit-osx
pushd bibledit-osx
popd

[[ -d bibledit-ios ]] || git clone --branch=master $BE_CORE bibledit-ios
pushd bibledit-ios
popd

[[ -d bibledit-android ]] || git clone --branch=master $BE_CORE bibledit-android
pushd bibledit-android
popd

[[ -d bibledit-windows ]] || git clone --branch=master $BE_CORE bibledit-windows
pushd bibledit-windows
popd

[[ -d bibledit-linux ]] || git clone --branch=master $BE_CORE bibledit-linux
pushd bibledit-linux
popd

[[ -d bibledit-cloud ]] || git clone --branch=master $BE_CORE bibledit-cloud
pushd bibledit-cloud
popd

[[ -d bibledit ]] || git clone --branch=master $BE_CORE bibledit
pushd bibledit
popd

ls -al
