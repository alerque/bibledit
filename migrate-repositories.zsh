#!/usr/bin/env zsh

set -e

: ${DEBUG:=false}
$DEBUG && set -x

BASE=~/projects/bibledit
TARGET=~/scratch/bibledit
REMOTE=git@github.com:bibledit

BE_CORE=bibledit-core
CI_MSG="[Migrate repos]"

function show_authors() {
	git log --format='%an <%ae>' | sort | uniq -c | sort -n
}

function normalize_authors () {
	show_authors | grep -qi Compaq && \
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
	while true; do
		if [[ -a $1 ]]; then
			git add $1
			shift
		else
			break
		fi
	done
	git ci -m "$CI_MSG $@"
	git --no-pager show --stat
}

function update_readme() {
	grep -q 'This repository houses' README.md && return ||:
	if [[ ! -f README.md ]]; then
		find -maxdepth 1 -type f -iname '*readme*' | head -n1 | cut -c3- | read old_readme ||:
		[[ -f $old_readme ]] && rename_path $old_readme README.md ||:
	fi
	[[ -f README.md ]] &&  echo >> README.md || touch README.md
	cut -c1- >> README.md <<-EOF
		# Bibledit ${(C)repo/*-} User Interface
		
		This repository houses the user interface wrapper for [Bibledit][be].
		For more details please see the [README][berm] in the main bibledit
		repository.

		[be]: https://bibledit.org
		[berm]: https://github.com/bibledit/bibledit
	EOF
	commit README.md "Add repository details to README"
}

function update_license() {
	[[ -f LICENSE ]] && return ||:
	find -maxdepth 1 -type f -iname 'licen*' -or -iname 'copying' | head -n1 | cut -c3- | read old_license ||:
	[[ -f $old_license ]] && rename_path $old_license LICENSE ||:
	curl -s http://www.gnu.org/licenses/gpl-3.0.txt > LICENSE
	commit LICENSE "Update GPLv3 license"
}

function apply_patches() {
	find $BASE/patches/$repo -type f |
		sort -n |
		while read patch; do
			git am $patch || git am --skip
		done
}

function rename_path() {
	old=$1
	new=$2
	git grep -l "\b$old\b" | while read file; do
	sed -i -e "s/\b$old\b(?!\.)/$new/g" $file
		git add $file
	done
	git mv $old $new
	commit "Rename path $old→$new"
}

function update_remote() {
	git remote -v | grep -q origin && git remote rm origin ||:
	git remote add origin $REMOTE/${repo}.git
}

function add_core_submodule() {
	if ! git submodule status $BE_CORE; then
		git submodule add -- $TARGET/$BE_CORE $BE_CORE
		git config --file=.gitmodules submodule.$BE_CORE.url $REMOTE/${BE_CORE}.git
		git config --file=.gitmodules submodule.$BE_CORE.branch master
		git submodule sync
		git add $BE_CORE .gitmodules
		commit "Initialize $BE_CORE as a submodule"
	fi
	git status
}

function add_editor_config() {
	[[ -f .editorconfig ]] && return ||:
	cut -c1- > .editorconfig <<-EOF
		# Cross-editor configuration
		# @see http://editorconfig.org/
		root = true

		[*]
		end_of_line = LF
		charset = utf-8
		trim_trailing_whitespace = true
		insert_final_newline = true

		[{Makefile}]
		indent_style = tab
		indent_size = 4

		[*.{py,md}]
		indent_style = space
		indent_size = 4

		[*.{xml,html,hbs,zsh}]
		indent_style = tab
		indent_size = 4

		[*.{js,json,yml,css}]
		indent_style = space
		indent_size = 2
	EOF
	commit .editorconfig "Add project-wide editor configuration"
}

function common_cleanup() {
	update_remote
	add_core_submodule
	update_readme
	update_license
	echo "## $repo"
	git gc --aggressive --prune=all
	show_authors
	add_editor_config
	apply_patches
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
normalize_authors
popd

init_repo orig-bibledit-web $BASE savannah/bibledit-web
normalize_authors
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
