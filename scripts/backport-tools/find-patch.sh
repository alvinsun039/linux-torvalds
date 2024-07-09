#!/bin/bash

. $(dirname $0)/init-functions

function usage()
{
	echo -e "Usage: `basename $0` [-p] [-s] [-k] upstream-commit-id"
	echo ""
	echo -e "  -h\t\tshow this help info"
	echo -e "  -p\t\tpull current repository before finding"
	echo -e "  -s\t\tonly check upstream LTS"
	echo -e "  -k\t\tonly check klinux-4.19 stable branch"
}

#defautly find both stable and kylin
PULL_REPO=""
STABLE_ONLY="true"
KYLIN_ONLY="true"
ARGS=`getopt -o pskh -- "$@"`
if [ $? != 0 ]; then
	echo "Terminating..."
	usage && exit 1
fi

eval set -- "${ARGS}"
while true
do
	case $1 in
		-h)
			usage;
			exit 0;;
		-p)
			PULL_REPO="true";
			shift 1;;
		-s)
			STABLE_ONLY="true";
			KYLIN_ONLY="false";
			shift 1;;
		-k)
			STABLE_ONLY="false";
			KYLIN_ONLY="true";
			shift 1;;
		--)
			shift
			break;;
	esac
done

[ $# -lt 1 ] && usage && exit -1

CID=$(git rev-parse $1 2>/dev/null)
if [ $? -ne 0 ]; then
	echo "$1 isn't a upstream commit id, please confirm and try again!"
	exit 1
fi

#update repository before finding
if [ "$PULL_REPO" = "true" ]; then
	git pull --rebase --autostash --tags
fi

SCID=$(git log --oneline -1 $CID | awk '{print $1}')
CTITLE=$(git log --pretty=%s -1 $CID)
CTAG=$(gdct $CID)

echo "Analyze patch: ${SCID}(${CTAG}) ${CTITLE}"

#find the patch for every upstream stables
if [ "$STABLE_ONLY" = "true" ]; then
	for BRANCH in ${STABLE_BRANCH}; do
		if [ "$(git merge-base $CID v$BRANCH)" = "$CID" ]; then
			echo -e "stable-${BRANCH}:\t primitive included"
		else
			STABLE_LATEST_TAG=$(git tag --list "v${BRANCH}.*" --sort="-taggerdate" | head -n1)
			STABLE_HEAD=$(git log --oneline v${BRANCH}..${STABLE_LATEST_TAG} | grep "${CTITLE}$" | awk '{print $1}')
			if [ -z $STABLE_HEAD ]; then
				echo -e "stable-${BRANCH}:\t not included"
			else
				STABLE_FIX_TAG=$(gdct $STABLE_HEAD)
				echo -e "stable-${BRANCH}:\t ${STABLE_HEAD}(${STABLE_FIX_TAG:0:12}) ${CTITLE}"
			fi
		fi

	done
fi

#find the patch for each branch in klinux-4.19
if [ "$KYLIN_ONLY" = "true" ]; then
	for BRANCH in ${KYLIN_BRANCH}; do
		SP=$(branch2name ${BRANCH})
		if [ "$(git merge-base $CID origin/$BRANCH)" = "$CID" ]; then
			echo -e "${SP}:\tprimitive included"
		else
			KYLIN_HEAD=$(git log --oneline ${STABLE_BRANCH_ID}..origin/${BRANCH} | grep "${CTITLE}$" | awk '{print $1}')
			if [ -z $KYLIN_HEAD ]; then
				echo -e "${SP}:\tnot included"
			else
				KYLIN_FIX_TAG=$(better_gdct $KYLIN_HEAD)
				echo -e "$SP:\t${GREEN}${KYLIN_HEAD} (${KYLIN_FIX_TAG})\t${CTITLE}${NC}"
			fi
		fi
	done
fi
