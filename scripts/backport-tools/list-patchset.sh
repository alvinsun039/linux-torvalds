#!/bin/bash
# list-patchset.sh - Find patchset from kernel commit ID via lore.kernel.org
# Author: Jackie Liu <liuyun01@kylinos.cn>
# Compatible: GNU/Linux, macOS

set -euo pipefail

# Load common functions
. $(dirname $0)/init-functions

# --- Configuration ---
LORE_BASE_URL="https://lore.kernel.org"

# --- Global variables ---
COMMIT_ID=""
COMMIT_TITLE=""
COMMIT_MSG=""
THREAD_URL=""

# --- Utility functions ---

print_info() { echo -e "${BLUE}==>${NC} $1"; }
print_step() { echo -e "    ${GREEN}$1${NC}"; }
print_warning() { echo -e "    ${YELLOW}Warning:${NC} $1"; }
print_error() { echo -e "    ${RED}Error:${NC} $1"; }

# Check if required tools are available
check_prerequisites() {
	local required_tools=("git" "curl" "awk" "sed" "grep")

	for tool in "${required_tools[@]}"; do
		if ! command -v "$tool" &>/dev/null; then
			print_error "Required tool '$tool' is not installed."
			exit 1
		fi
	done
}

# Validate commit ID
validate_commit() {
	if ! git cat-file -e "$COMMIT_ID^{commit}" 2>/dev/null; then
		print_error "Invalid or unknown commit ID: $COMMIT_ID"
		exit 1
	fi
}

# Fetch commit information
get_commit_info() {
	COMMIT_MSG=$(git show -s --format=%B "$COMMIT_ID")
	COMMIT_TITLE=$(git show -s --format=%s "$COMMIT_ID")
	print_step "Commit title: $COMMIT_TITLE"
}

# Extract Message-ID from commit message
extract_message_id() {
	echo "$COMMIT_MSG" | grep -i "^Message-ID:" | sed 's/^Message-ID:[[:space:]]*//i' | tr -d '<>' || true
}

# Extract Link from commit message
extract_link() {
	local links=$(echo "$COMMIT_MSG" | grep -i "^Link:" | sed 's/^Link:[[:space:]]*//i; s/[[:space:]]*\[[0-9]\+\][[:space:]]*$//')

	# Prefer patch.msgid.link format as it's more likely to be the actual patch
	local patch_link=$(echo "$links" | grep "patch\.msgid\.link" | head -1)
	if [ -n "$patch_link" ]; then
		echo "$patch_link"
		return
	fi

	# Fall back to the first link
	echo "$links" | head -1 || true
}

# Check if URL is a stable branch patch
is_stable_branch_patch() {
	local page_content="$1"
	# Check for various stable branch patterns
	echo "$page_content" | grep -qE '\[PATCH.*(\.[0-9]+\.y|AUTOSEL|BACKPORT|STABLE|[0-9]+\.[0-9]+ [0-9]+/[0-9]+)'
}

# Check if URL is a standard patch
is_standard_patch() {
	local page_content="$1"
	echo "$page_content" | grep -q '\[PATCH[^]]*\]'
}

# Normalize title for comparison
normalize_title() {
	local title="$1"
	echo "$title" | tr '[:upper:]' '[:lower:]' | sed 's/[[:space:]]\+/ /g; s/^[[:space:]]*//; s/[[:space:]]*$//'
}

# Check if patchset title matches our commit title
is_matching_patchset() {
	local page_content="$1"
	local commit_title="$2"

	# Extract patchset title from page content
	local patchset_title=$(echo "$page_content" | grep -o '\[PATCH[^<]*' | head -1 | sed 's/\[PATCH[^]]*\] //' || true)

	if [ -z "$patchset_title" ]; then
		return 1
	fi

	# Normalize titles for comparison
	local normalized_commit=$(normalize_title "$commit_title")
	local normalized_patchset=$(normalize_title "$patchset_title")

	# Split commit title into meaningful words and check matches
	local commit_words=$(echo "$normalized_commit" | tr ' ' '\n' | grep -v '^$' | sort | uniq)
	local match_count=0
	local total_words=0
	local common_words="the|and|or|for|in|on|at|to|of|with|by|from|this|that|have|has|had|will|would|could|should|can|may|might|must|shall"

	for word in $commit_words; do
		# Skip short words and common words
		if [ ${#word} -ge 3 ] && ! echo "$word" | grep -qE "^($common_words)$"; then
			total_words=$((total_words + 1))
			if echo "$normalized_patchset" | grep -q "$word"; then
				match_count=$((match_count + 1))
			fi
		fi
	done

	# Check if at least 60% of meaningful words match
	if [ $total_words -gt 0 ]; then
		local match_percentage=$((match_count * 100 / total_words))
		if [ $match_percentage -ge 60 ]; then
			print_step "Title match found: $match_count/$total_words words match ($match_percentage%)"
			print_step "Commit: $commit_title"
			print_step "Patchset: $patchset_title"
			return 0
		fi
	fi

	return 1
}

# Convert relative link to absolute URL
make_absolute_url() {
	local link="$1"
	case "$link" in
		/*) echo "https://lore.kernel.org$link" ;;
		*) echo "$link" ;;
	esac
}

# Extract lore.kernel.org links from search results
extract_lore_links() {
	local search_result="$1"

	# Extract absolute links first
	local absolute_links=$(echo "$search_result" | grep -o 'href="[^"]*lore\.kernel\.org/all/[^"]*"' | sed 's/href="//;s/"$//' || true)

	# If no absolute links found, look for relative links
	if [ -z "$absolute_links" ]; then
		local relative_links=$(echo "$search_result" | grep -o 'href="[0-9][^"]*@[^"]*"' | sed 's/href="//;s/"$//' || true)
		[ -n "$relative_links" ] && absolute_links=$(echo "$relative_links" | sed 's|^|https://lore.kernel.org/all/|')
	fi

	echo "$absolute_links"
}

# Process a single lore link for standard patch search
process_lore_link() {
	local link="$1"
	local full_link=$(make_absolute_url "$link")
	local page_content=$(curl -sS -L "$full_link" || true)

	# Skip stable branch patches
	if is_stable_branch_patch "$page_content"; then
		print_step "Skipping stable branch patch: $full_link"
		return 1
	fi

	# Check for standard patches
	if ! is_standard_patch "$page_content"; then
		print_step "Skipping non-standard patch: $full_link"
		return 1
	fi

	print_step "Found standard patch: $full_link"

	# Check if this patchset title matches our commit title
	if is_matching_patchset "$page_content" "$COMMIT_TITLE"; then
		print_step "Title match confirmed! Using this patchset."
		THREAD_URL="$full_link"
		return 0
	else
		print_step "Title mismatch, continuing search..."
		return 1
	fi
}

# Search for standard patch in lore links
search_standard_patch() {
	local lore_links="$1"
	local search_name="$2"

	[ -z "$lore_links" ] && return 1

	print_step "Found lore.kernel.org links:"
	print_step "Total links found: $(echo "$lore_links" | wc -l)"
	print_step "Looking for standard patches with title matching..."

	for link in $lore_links; do
		if process_lore_link "$link"; then
			return 0
		fi
	done

	return 1
}

# Search lore.kernel.org by title
search_by_title() {
	local title="$1"
	local encoded_title=$(echo "$title" | sed 's/ /%20/g')
	local search_url="${LORE_BASE_URL}/all/?q=${encoded_title}"

	print_step "Searching: $search_url"

	local search_result=$(curl -sS "$search_url")
	if [ -z "$search_result" ]; then
		print_step "No results from title search"
		return 1
	fi

	# Look for Message-ID in search results first
	local search_message_id=$(echo "$search_result" | grep -o 'Message-ID: <[^>]*>' | head -1 | sed 's/Message-ID: <//;s/>//' || true)

	if [ -n "$search_message_id" ]; then
		print_step "Found Message-ID in search results: $search_message_id"
		THREAD_URL="${LORE_BASE_URL}/r/${search_message_id}/"
		return 0
	fi

	# Extract and search through lore links
	local lore_links=$(extract_lore_links "$search_result")
	if search_standard_patch "$lore_links" "title search"; then
		return 0
	fi

	return 1
}

# Decode HTML entities in title
decode_html_entities() {
	local title="$1"
	echo "$title" | sed -e 's/&lt;/</g' -e 's/&gt;/>/g' -e 's/&amp;/\&/g' -e "s/&#39;/'/g" -e 's/&quot;/"/g' -e 's/&#34;/"/g' -e 's/^[[:space:]]*//; s/[[:space:]]*$//'
}

# Process a single patch title and display its details
process_patch_title() {
	local title="$1"

	# Clean up the title
	local clean_title=$(decode_html_entities "$title")

	[ -z "$clean_title" ] && return

	# Skip cover letters (patch 0/X)
	if echo "$clean_title" | grep -q '\[PATCH[^]]*0/'; then
		echo -e "  ${BLUE}(cover letter) $clean_title${NC}"
		return
	fi

	# Remove patch prefix for git search
	local search_title=$(echo "$clean_title" | sed 's/^\[PATCH[^]]*\] //')

	# Search for commit ID in origin/master using the title
	local commit_id=$(git log origin/master --oneline --grep="$search_title" --format="%H" | head -1)

	if [ -n "$commit_id" ]; then
		# Use short SHA (14 characters) to align with "(cover letter)"
		local short_commit_id=$(echo "$commit_id" | cut -c1-14)
		echo -e "  ${BLUE}$short_commit_id $clean_title${NC}"
	else
		echo -e "  ${BLUE}(not found in local repo) $clean_title${NC}"
	fi
}

# Extract patch titles from thread page
extract_patch_titles() {
	local thread_page="$1"
	echo "$thread_page" | grep -o '\[PATCH[^]]*\][^<]*' | sed 's/<\/a>.*$//' | sed 's/ - [^-]*$//' | sed 's/&quot;.*$//' | sed 's/&#34;.*$//' | while read -r title; do decode_html_entities "$title"; done | sort | uniq || true
}

# Remove duplicate titles that are prefixes of longer titles
remove_duplicate_titles() {
	local patch_titles="$1"
	[ -z "$patch_titles" ] && return

	local cleaned_titles=""
	while read -r title; do
		local is_duplicate=0
		while read -r other_title; do
			if [ "$title" != "$other_title" ] && [[ "$other_title" == "$title"* ]]; then
				is_duplicate=1
				break
			fi
		done <<< "$patch_titles"
		if [ $is_duplicate -eq 0 ]; then
			cleaned_titles="$cleaned_titles$title"$'\n'
		fi
	done <<< "$patch_titles"
	echo "$cleaned_titles"
}

# Find the latest version number from patch titles
find_latest_version() {
	local patch_titles="$1"
	local latest_version_num=0
	local latest_version=""

	while read -r title; do
		if echo "$title" | grep -q '\[PATCH v[0-9]'; then
			local version=$(echo "$title" | grep -o 'v[0-9]*' | head -1)
			if [ -n "$version" ]; then
				local version_num=$(echo "$version" | sed 's/v//')
				if [ "$version_num" -gt "$latest_version_num" ] 2>/dev/null; then
					latest_version="$version"
					latest_version_num="$version_num"
				fi
			fi
		fi
	done <<< "$patch_titles"
	echo "$latest_version"
}

# Filter patches to keep only the latest version
filter_latest_version() {
	local patch_titles="$1"
	local latest_version="$2"
	local filtered_titles=""

	while read -r title; do
		if [ -n "$latest_version" ]; then
			# If we have versioned patches, keep only the latest version
			if echo "$title" | grep -q "\[PATCH $latest_version"; then
				filtered_titles="$filtered_titles$title"$'\n'
			fi
		else
			# If no versioned patches, keep all non-versioned patches
			if ! echo "$title" | grep -q '\[PATCH v[0-9]'; then
				filtered_titles="$filtered_titles$title"$'\n'
			fi
		fi
	done <<< "$patch_titles"
	echo "$filtered_titles"
}

# Display patch titles and details
display_patch_results() {
	local patch_titles="$1"

	if [ -n "$patch_titles" ]; then
		echo "Found patch titles:"
		while read -r title; do
			echo "  $title"
		done <<< "$patch_titles"

		# Extract and display commit ID + patch title combinations
		echo ""
		echo "Patch details (Commit ID + Title):"
		while read -r title; do
			process_patch_title "$title"
		done <<< "$patch_titles"
	else
		echo "No patch series found in this thread."
		echo "This might be a single patch or the page structure is different."
	fi
}

# Parse patchset information
parse_patchset() {
	local thread_page="$1"

	echo ""
	echo "--- Patch Series Found ---"

	# Extract and process patch titles
	local patch_titles=$(extract_patch_titles "$thread_page")
	patch_titles=$(remove_duplicate_titles "$patch_titles")

	# Filter to keep only the latest version of patches
	if [ -n "$patch_titles" ]; then
		local latest_version=$(find_latest_version "$patch_titles")
		patch_titles=$(filter_latest_version "$patch_titles" "$latest_version")
	fi

	# Display results
	display_patch_results "$patch_titles"
}

# Main search function
search_patchset() {
	print_info "Step 3: Searching lore.kernel.org for matching email thread"
	print_step "Trying search strategies..."

	# Strategy 1: Full title search
	print_step "Strategy 1: Full title search"
	if search_by_title "$COMMIT_TITLE"; then
		return 0
	fi

	print_error "Could not locate any matching patch thread on lore.kernel.org"
	print_error "This commit may not have a corresponding patchset, or the search terms need adjustment."
	return 1
}

# Extract thread URL from commit information
extract_thread_url() {
	local message_id=$(extract_message_id)
	local link=$(extract_link)

	if [ -n "$message_id" ]; then
		print_step "Found Message-ID: $message_id"
		local message_id_clean=$(echo "$message_id" | tr -d '<>')
		THREAD_URL="${LORE_BASE_URL}/r/${message_id_clean}/"
	elif [ -n "$link" ]; then
		print_step "Found Link: $link"
		# Check if it's a patch.msgid.link format and convert to lore.kernel.org format
		if echo "$link" | grep -q "patch\.msgid\.link"; then
			# Extract MESSAGE-ID from patch.msgid.link/MESSAGE-ID
			local msgid=$(echo "$link" | sed 's|.*patch\.msgid\.link/||')
			THREAD_URL="${LORE_BASE_URL}/r/${msgid}/"
			print_step "Converted patch.msgid.link to lore format: $THREAD_URL"
		else
			THREAD_URL="$link"
		fi
	else
		print_step "No Message-ID or Link found in commit message"
		THREAD_URL=""
	fi
}

# Fetch and parse thread page
fetch_and_parse_thread() {
	print_info "Step 4: Fetching patchset thread:"
	print_step "$THREAD_URL"

	local thread_page=$(curl -sS -L "$THREAD_URL")
	[ -z "$thread_page" ] && { print_error "Failed to fetch thread page"; exit 1; }

	parse_patchset "$thread_page"
}

# Display final results
display_final_results() {
	echo ""
	echo "--- Thread URL ---"
	echo "$THREAD_URL"
	echo "-------------------"
	print_info "Done."
}

# Main function
main() {
	# Check arguments
	[ $# -ne 1 ] && { echo "Usage: $0 <commit-id>"; exit 1; }

	COMMIT_ID="$1"

	# Initialize
	check_prerequisites

	# Step 1: Fetch commit info
	print_info "Step 1: Fetching commit info for '$COMMIT_ID'"
	validate_commit "$COMMIT_ID"
	get_commit_info "$COMMIT_ID"

	# Step 2: Check for Message-ID or Link in commit message
	print_info "Step 2: Checking commit message for Message-ID or Link"
	extract_thread_url

	# Step 3: Search lore.kernel.org if no direct link found
	[ -z "$THREAD_URL" ] && { search_patchset || exit 1; }

	# Step 4: Fetch and parse thread
	fetch_and_parse_thread

	# Display results
	display_final_results
}

# Run main function
main "$@"
