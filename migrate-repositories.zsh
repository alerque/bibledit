#!/usr/bin/env zsh

set -e

: ${DEBUG:=false}
$DEBUG && set -x

BASE=~/projects/bibledit
TARGET=~/scratch/bibledit
REMOTE=git@github.com:bibledit

BE_CORE=bibledit-core
CI_MSG="[Migrate repos]"

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

function init_repo() {
	repo=$1
	src=$2
	branch=$3
	[[ -d $repo ]] || git clone --single-branch --no-hardlinks --branch=$branch $src $repo
	pushd $repo
}

function commit() {
	[[ -n $2 ]] && git add $1
	git ci -m "$CI_MSG $2"
}

function update_readme() {
	if [[ ! -f README.md ]]; then
		find -maxdepth 1 -type f -iname '*readme*' | head -n1 | cut -c3- | read old_readme
		rename_path $old_readme README.md
	fi
	echo "\n\n### $repo\n\nSee README in bibledit repo" >> README.md
	commit README.md "Add repo details to README"
}

function rename_path() {
	old=$1
	new=$2
	git grep -l "\b$old\b" | while read file; do
	sed -i -e "s/\b$old\b(?!\.)/$new/g" $file
		git add $file
	done
	git mv $old $new
	commit "$CI_MSG Rename path $old→$new"
}

function update_remote() {
	git remote -v | grep -q origin && git remote rm origin ||:
	git remote add origin $REMOTE/${repo}.git
}

function add_core_submodule() {
	if ! git submodule status $BE_CORE; then
		git submodule add -- $TARGET/$BE_CORE $BE_CORE
		git submodule init -- $BE_CORE
		git config --file=.gitmodules submodule.$BE_CORE.url $REMOTE/${BE_CORE}.git
		git config --file=.gitmodules submodule.$BE_CORE.branch master
		git submodule sync
		git submodule update --init --remote 
		git add $BE_CORE .gitmodules
		commit "Initialize $BE_CORE as a submodule"
	fi
	git status
}

function common_cleanup() {
	update_remote
	#add_core_submodule
	update_readme
}

#rm -rf $TARGET ; mkdir $TARGET
pushd $TARGET

init_repo orig-bibledit $BASE master
normalize-authors
popd

init_repo orig-bibledit-web $BASE savannah/bibledit-web
normalize-authors
popd

init_repo bibledit orig-bibledit master
common_cleanup
popd

init_repo $BE_CORE orig-bibledit master
common_cleanup
popd

init_repo bibledit-osx orig-bibledit master
common_cleanup
popd

init_repo bibledit-chromeos orig-bibledit master
common_cleanup
popd

init_repo bibledit-ios orig-bibledit master
common_cleanup
popd

init_repo bibledit-android orig-bibledit master
common_cleanup
popd

init_repo bibledit-windows orig-bibledit master
common_cleanup
popd

init_repo bibledit-linux orig-bibledit master
common_cleanup
popd

init_repo bibledit-cloud orig-bibledit master
common_cleanup
popd

ls -al
