#!/usr/bin/env zsh

set -e

: ${DEBUG:=false}
$DEBUG && set -x

BASE=~/projects/bibledit
TARGET=~/scratch/bibledit

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
	name=$1
	src=$2
	branch=$3
	[[ -d $name ]] || git clone --branch=$branch $src $name
	pushd $name
}

function update_readme() {
	if [[ ! -f README.md ]]; then
		find -maxdepth 1 -type f -iname '*readme*' | head -n1 | cut -c3- | read old_readme
		rename_path $old_readme README.md
	else
		: cat README.md
	fi
}

function rename_path() {
	old=$1
	new=$2
	git grep -l "\b$old\b" | while read file; do
		sed -i -e "s/\b$old\b/$new/g" $file
		git add $file
	done
	git mv $old $new
	git ci -m "$CI_MSG Rename path $old→$new"
}

#rm -rf $TARGET ; mkdir $TARGET
pushd $TARGET

init_repo $BE_CORE $BASE master
normalize-authors
update_readme
popd

init_repo bibledit-web $BASE savannah/bibledit-web
normalize-authors
update_readme
popd

init_repo bibledit-osx $BASE master
update_readme
popd

init_repo bibledit-chromeos $BASE master
update_readme
popd

init_repo bibledit-ios $BASE master
update_readme
popd

init_repo bibledit-android $BASE master
update_readme
popd

init_repo bibledit-windows $BASE master
update_readme
popd

init_repo bibledit-linux $BASE master
update_readme
popd

init_repo bibledit-cloud $BASE master
update_readme
popd

init_repo bibledit $BASE master
update_readme
popd

ls -al
