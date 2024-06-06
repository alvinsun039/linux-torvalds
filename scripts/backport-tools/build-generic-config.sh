#!/bin/bash

. $(dirname $0)/init-functions

CROSS_ARCH=$ARCH

function usage()
{
	echo -e "`basename $0` [OPTION]"
	echo
	echo -e "    -h, --help\t\tshow this help."
	echo -e "    -a, --arch\t\tset the cross compile to 'arch'"
	echo -e "    -i, --ignore\t\tignore make menuconfig"
	echo
}

ARGS=`getopt -o ha:i -a --long help,arch:,ignore -- "$@"`
[ $? != 0 ] && echo "Terminating..." && usage && exit -1

eval set -- "${ARGS}"

while true
do
	case $1 in
		-h|--help)
			usage
			exit 0;;
		-a|--arch)
			if [ "$2" != "aarch64" -a "$2" != "x86_64" ]; then
				usage
				echo "Not supported, not aarch64 or x86_64."
				exit -1;
			fi
			CROSS_ARCH=$2
			shift 2;;
		-i|--ignore)
			IGNORE_MENUCONFIG=true
			shift 1;;
		--)
			shift
			break;;
	esac
done

CROSS_SOURCE_ARCH=$(echo -e $CROSS_ARCH | sed -e s/i.86/x86/ -e s/x86_64/x86/ \
			-e s/amd64/x86/ -e s/aarch64.*/arm64/ -e s/mips64el/mips/ \
			-e s/loongarch64.*/loongarch/)
if [ $ARCH != $CROSS_ARCH ]; then
	OPT="CROSS_COMPILE=${CROSS_ARCH}-linux-gnu- ARCH=${CROSS_SOURCE_ARCH}"

	if [ -z "$(which ${CROSS_ARCH}-linux-gnu-gcc 2>/dev/null)" ]; then
		echo -e "${RED}Not found ${CROSS_ARCH}-linux-gnu-gcc, Please confirm that the cross-compilation environment has been installed.${NC}"
		exit -2;
	fi
fi

function make_defconfig()
{
	make defconfig ${OPT} -j $(nproc)
}

function make_savedefconfig()
{
	make savedefconfig ${OPT} -j $(nproc)
	mv defconfig arch/${CROSS_SOURCE_ARCH}/configs/${DEFAULT_CONFIG}
}

function make_menuconfig()
{
	make menuconfig ${OPT} -j $(nproc)
}

make_defconfig
[ x"$IGNORE_MENUCONFIG" != x"true" ] && make_menuconfig
make_savedefconfig
