#!/bin/bash

. $(dirname $0)/init-functions

function usage() {
	echo "Usage: $(basename $0) [-p] [-s] [-k] [-h] upstream-commit-id"
	echo ""
	echo "Options:"
	echo "  -h    Show this help information"
	echo "  -p    Pull current repository before finding"
	echo "  -s    Only check upstream LTS branches"
	echo "  -k    Only check Kylin branches"
	echo ""
	echo "Examples:"
	echo "  $(basename $0) abc123def456    # Check both stable and Kylin branches"
	echo "  $(basename $0) -s abc123def456  # Check only stable branches"
	echo "  $(basename $0) -k abc123def456  # Check only Kylin branches"
}

# Default: find both stable and Kylin branches
PULL_REPO="false"
STABLE_ONLY="true"
KYLIN_ONLY="true"

# Parse command line arguments
ARGS=$(getopt -o pskh -- "$@")
if [ $? -ne 0 ]; then
	echo "Error: Invalid arguments" >&2
	usage
	exit 1
fi

eval set -- "${ARGS}"
while true; do
	case $1 in
		-h)
			usage
			exit 0
			;;
		-p)
			PULL_REPO="true"
			shift
			;;
		-s)
			STABLE_ONLY="true"
			KYLIN_ONLY="false"
			shift
			;;
		-k)
			STABLE_ONLY="false"
			KYLIN_ONLY="true"
			shift
			;;
		--)
			shift
			break
			;;
		*)
			echo "Error: Unknown option $1" >&2
			usage
			exit 1
			;;
	esac
done

# Validate input arguments
if [ $# -lt 1 ]; then
	echo "Error: Missing upstream commit ID" >&2
	usage
	exit 1
fi

# Validate and normalize commit ID
CID=$(git rev-parse "$1" 2>/dev/null)
if [ $? -ne 0 ]; then
	echo "Error: '$1' is not a valid upstream commit ID" >&2
	exit 1
fi

# Update repository if requested
if [ "$PULL_REPO" = "true" ]; then
	echo "Updating repository..."
	if ! git pull --rebase --autostash --tags; then
		echo "Warning: Failed to update repository, continuing with current state" >&2
	fi
fi

# Extract commit information
SCID=$(git log --oneline -1 "$CID" | awk '{print $1}')
CTITLE=$(git log --pretty="%s" -1 "$CID")
CTAG=$(gdct "$CID")

echo "Analyzing patch: ${SCID}(${CTAG}) ${CTITLE}"

# Check upstream stable branches
if [ "$STABLE_ONLY" = "true" ]; then
	echo ""
	echo "Checking upstream stable branches:"
	for BRANCH in ${STABLE_BRANCH}; do
		SP=$(printf "%-14s" "stable-${BRANCH}:")

		# Check if commit is already included in the base version
		if [ "$(git merge-base "$CID" "v$BRANCH" 2>/dev/null)" = "$CID" ]; then
			echo -e "${SP}     ${GREEN}primitive included${NC}"
		else
			# Find latest stable tag for this branch
			STABLE_LATEST_TAG=$(git tag --list "v${BRANCH}.*" --sort="-taggerdate" | head -n1)
			if [ -z "$STABLE_LATEST_TAG" ]; then
				echo -e "${SP}     ${YELLOW}no stable tags found${NC}"
				continue
			fi

			# Search for the patch in stable updates
			STABLE_HEAD=$(git log --oneline "v${BRANCH}..${STABLE_LATEST_TAG}" --grep="${CTITLE}" --fixed-strings | head -n1 | awk '{print $1}')
			if [ -z "$STABLE_HEAD" ]; then
				echo -e "${SP}     ${YELLOW}not included${NC}"
			else
				STABLE_FIX_TAG=$(gdct "$STABLE_HEAD")
				echo -e "${SP}     ${GREEN}${STABLE_HEAD}(${STABLE_FIX_TAG:0:12}) ${CTITLE}${NC}"
			fi
		fi
	done
fi

# Check Kylin branches
if [ "$KYLIN_ONLY" = "true" ]; then
	echo ""
	echo "Checking Kylin branches:"
	for BRANCH in ${KYLIN_BRANCH}; do
		SP=$(printf "%-14s" "$(branch2name "${BRANCH}"):")

		# Check if remote branch exists
		if ! git show-ref --verify --quiet "refs/remotes/origin/$BRANCH"; then
			echo -e "${SP}     ${YELLOW}branch not found${NC}"
			continue
		fi

		# Check if commit is already included in the branch
		if [ "$(git merge-base "$CID" "origin/$BRANCH" 2>/dev/null)" = "$CID" ]; then
			echo -e "${SP}     ${GREEN}primitive included${NC}"
		else
			# Search for the patch in Kylin branch
			# Use the stable version tag as base instead of commit hash
			STABLE_TAG="v${STABLE_MAJ_VER}.${STABLE_MIN_VER}"
			KYLIN_HEAD=$(git log --oneline "${STABLE_TAG}..origin/${BRANCH}" --grep="${CTITLE}" --fixed-strings | head -n1 | awk '{print $1}')
			if [ -z "$KYLIN_HEAD" ]; then
				echo -e "${SP}     ${YELLOW}not included${NC}"
			else
				KYLIN_FIX_TAG=$(better_gdct "$KYLIN_HEAD")
				echo -e "${SP}     ${GREEN}${KYLIN_HEAD} (${KYLIN_FIX_TAG})\t${CTITLE}${NC}"
			fi
		fi
	done
fi

# Summary
echo ""
echo "Search completed for commit: ${SCID} (${CTITLE})"
if [ "$STABLE_ONLY" = "true" ] && [ "$KYLIN_ONLY" = "true" ]; then
	echo "Checked: Both upstream stable and Kylin branches"
elif [ "$STABLE_ONLY" = "true" ]; then
	echo "Checked: Upstream stable branches only"
else
	echo "Checked: Kylin branches only"
fi
