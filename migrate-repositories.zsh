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
	git --no-pager show
}

function update_readme() {
	if [[ ! -f README.md ]]; then
		find -maxdepth 1 -type f -iname '*readme*' | head -n1 | cut -c3- | read old_readme ||:
		[[ -f $old_readme ]] && rename_path $old_readme README.md && echo >> README.md || touch README.md
	fi
	grep -q 'main bibledit repository' README.md && return
	echo "# Bibledit ${(C)repo/*-} User Interface\n\nThis repository houses the user interface wrapper for [Bibledit][be]. For more details please see the [README][berm] in the main bibledit repository.\n\n[be]: https://bibledit.org\n[berm]: https://github.com/bibledit/bibledit" >> README.md
	commit README.md "Add repository details to README"
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
	add_core_submodule
	update_readme
	echo "## $repo"
	git gc --aggressive --prune=all
	show-authors
}

function trim_to_path() {
	keep=$1
	[[ ! -d $keep ]] && return
	git subtree split -P $keep -b tmp
	git checkout tmp
	git branch -M tmp master
}

function remove_paths() {
	needed=false
	for dir in ${(z)@}; do
		[[ -a $dir ]] && needed=true ||:
	done
	$needed && git filter-branch -f --tag-name-filter cat --prune-empty --index-filter "
		git rm -rf --cached --ignore-unmatch -- $@
	"
}

function remove_dead() {
	git rm -rf -- $@ && commit "Remove dead code paths" ||:
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
remove_paths lib osx chromeos ios android windows linux
remove_paths bibletime bibleworks gtk onlinebible paratext web xiphos
remove_dead test
common_cleanup
popd

init_repo $BE_CORE orig-bibledit master
trim_to_path lib
common_cleanup
popd

init_repo bibledit-osx orig-bibledit master
trim_to_path osx
common_cleanup
popd

init_repo bibledit-chromeos orig-bibledit master
trim_to_path chromeos
common_cleanup
popd

init_repo bibledit-ios orig-bibledit master
trim_to_path ios
common_cleanup
popd

init_repo bibledit-android orig-bibledit master
trim_to_path android
common_cleanup
popd

init_repo bibledit-windows orig-bibledit master
trim_to_path windows
common_cleanup
popd

init_repo bibledit-linux orig-bibledit master
trim_to_path linux
common_cleanup
popd

init_repo bibledit-cloud orig-bibledit master
#trim_to_path
common_cleanup
popd

echo "This too was meaningless."
