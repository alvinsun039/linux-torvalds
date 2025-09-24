#!/bin/bash

VER=2025-07-29

. "$(dirname $0)/init-functions"

_temp_fixes_=$(mktemp)
_all_log_fixes=$(mktemp)
progress_pid=""
interrupted=0

# cleanup function
cleanup() {
	[ -f "${_temp_fixes_}" ] && rm -f "${_temp_fixes_}"
	[ -f "${_all_log_fixes}" ] && rm -f "${_all_log_fixes}"
	# stop progress indicator if running
	if [[ -n "$progress_pid" ]] && kill -0 "$progress_pid" 2>/dev/null; then
		kill "$progress_pid" 2>/dev/null
		wait "$progress_pid" 2>/dev/null
		printf "\r%*s\r" 20 ""  # clear progress line
	fi
	# kill any remaining background jobs
	[ -n "$(jobs -pr)" ] && kill $(jobs -pr) 2>/dev/null
	# set interrupted flag for signal handling
	if [[ $? -eq 130 ]] || [[ $interrupted -eq 1 ]]; then
		exit 130
	fi
}

# signal handler for interruption
handle_interrupt() {
	interrupted=1
	cleanup
	exit 130
}

# cleanup jobs and temp files
trap handle_interrupt INT QUIT TERM
trap cleanup EXIT

search_fixes() {
	local original="$1"
	local commit_12="${original:0:12}"
	local git_data
	local commit="${original:0:10}"

	if ! git_data=$(git show -s --date=format:'%d-%m-%Y' --format=%cd "$commit_12" 2>/dev/null); then
		echo "Warning: Failed to get commit date for $commit_12" >&2
		return 1
	fi

	git --no-pager log --after "$git_data" --grep "^Fixes:\s.*${commit}" --pretty="%h" origin/master >> "${_temp_fixes_}"
}

single_local_patch() {
	local downstream_commit="$1"
	local upstream_commit
	local commit

	upstream_commit=$(git show "$downstream_commit" 2>/dev/null | grep "Mainline:" | head -n 1 | awk '{print $2}')
	if [[ -z "$upstream_commit" ]]; then
		upstream_commit="$1"
	fi

	if [[ "$upstream_commit" == "KYLIN-only" ]]; then
		return 0
	fi

	# verify upstream_commit
	if ! git show "$upstream_commit" >/dev/null 2>&1; then
		echo "$downstream_commit with $upstream_commit is not upstream-commit, please re-check." >&2
		return 1
	fi

	if ! search_fixes "$upstream_commit"; then
		return 1
	fi

	if [[ ! -s "${_temp_fixes_}" ]]; then
		return 0
	fi

	while IFS= read -r commit; do
		[[ -z "$commit" ]] && continue

		if "$(dirname "$0")/test-commit-in-tree" -q "$commit"; then
			continue
		fi

		git --no-pager log -1 --pretty="${downstream_commit:0:12} <- %h %s" "$commit" >> "${_all_log_fixes}"
	done < "${_temp_fixes_}"

	> "${_temp_fixes_}"  # clear temp file for next use
}

all_local_patches() {
	local current_branch
	local commit_start
	local commits
	local commit

	current_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
	if [[ -z "$current_branch" ]]; then
		echo "Error: Not on any branch." >&2
		exit 1
	fi

	commit_start="origin/$current_branch"
	if [[ -n "$base_commit" ]]; then
		commit_start="$base_commit"
	fi

	commits=$(git log --pretty=oneline "HEAD...${commit_start}" --reverse 2>/dev/null | awk '{print $1}')
	if [[ -z "$commits" ]]; then
		echo -e "${RED}You don't have any un-merged commits${NC}" >&2
		exit 1
	fi

	while IFS= read -r commit; do
		[[ -z "$commit" ]] && continue
		single_local_patch "$commit"
	done <<< "$commits"
}

all_commits_patches() {
	local commit_file="$1"
	local commit

	if [[ ! -f "$commit_file" ]]; then
		echo "Error: $commit_file is not found, please check." >&2
		exit 1
	fi

	while IFS= read -r commit; do
		[[ -z "$commit" ]] && continue
		single_local_patch "$commit"
	done < "$commit_file"
}

usage() {
	cat << EOF
Usage: $0 [OPTIONS]

OPTIONS:
	-h			Show this help message
	-v			Show version information
	-c <commit-id>		Process a single commit
	-f <commit-list>	Process commits from file
	-b <start-commit-id>	Set base commit for comparison

EOF
	exit 0
}

show_version() {
	echo "search-git-fixes version: $VER"
	exit 0
}

show_progress() {
	local spinner="|/-\\"
	local i=0

	# handle termination signals gracefully
	trap 'exit 0' TERM INT

	while true; do
		printf "Searching... %c\r" "${spinner:$((i % 4)):1}"
		sleep 0.3 2>/dev/null || exit 0
		((i++))
	done
}

main() {
	check_git_is_installed
	is_git_repository

	show_progress &
	progress_pid=$!

	if [[ -n "$commit_id" ]]; then
		single_local_patch "$commit_id"
	elif [[ -n "$commit_file" ]]; then
		all_commits_patches "$commit_file"
	else
		all_local_patches
	fi

	# stop progress indicator
	kill $progress_pid 2>/dev/null
	wait $progress_pid 2>/dev/null
	printf "\r%*s\r" 20 ""  # clear progress line

	# process results
	local ret=0
	if [[ -f "${_all_log_fixes}" ]]; then
		if [[ ! -s "${_all_log_fixes}" ]]; then
			echo -e "${BLUE}Not Found.${RC}"
			ret=1
		else
			cat "${_all_log_fixes}"
			ret=0
		fi
	else
		echo "Error: Results file not found." >&2
		ret=1
	fi
	exit $ret
}

while getopts "hf:d:c:vb:" opt; do
	case "$opt" in
		v) show_version ;;
		c) commit_id="${OPTARG}" ;;
		f) commit_file="${OPTARG}" ;;
		h) usage ;;
		b) base_commit="${OPTARG}" ;;
		*) usage ;;
	esac
done

main
