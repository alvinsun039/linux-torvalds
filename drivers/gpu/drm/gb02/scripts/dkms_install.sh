#!/bin/bash
set -euo pipefail

CUSTOM_VERSION=""
SCRIPT_NAME=$(basename "$0")

show_help() {
	echo "Usage: sudo $SCRIPT_NAME [-v version]"
	echo "Examples:"
	echo " sudo $SCRIPT_NAME           # Use original config from dkms.conf"
	echo " sudo $SCRIPT_NAME -v 1.2.0  # Override version and update dkms.conf"
	exit 0
}

while getopts ":v:h" opt; do
case $opt in
v) CUSTOM_VERSION="$OPTARG" ;;
h) show_help ;;
\?) echo "Invalid option: -$OPTARG" >&2; exit 1 ;;
:) echo "Option -$OPTARG requires an argument" >&2; exit 1 ;;
esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DKMS_CONF_FILE="${PROJECT_ROOT}/dkms.conf"

[[ ! -f "$DKMS_CONF_FILE" ]] && { echo "Error: dkms.conf file not found!" >&2; exit 1; }

PACKAGE_NAME=$(grep -oP '^PACKAGE_NAME="\K[^"]+' "$DKMS_CONF_FILE" || true)
[[ -z "$PACKAGE_NAME" ]] && { echo "Error: Failed to parse PACKAGE_NAME!" >&2; exit 1; }

if [[ -n "$CUSTOM_VERSION" ]]; then
	if ! [[ "$CUSTOM_VERSION" =~ ^[a-zA-Z0-9_.-]+$ ]]; then
		echo "Error: Invalid version format '$CUSTOM_VERSION'" >&2
    		exit 1
  	fi
	sudo sed -i.bak "s/^PACKAGE_VERSION=.*/PACKAGE_VERSION=\"$CUSTOM_VERSION\"/" "$DKMS_CONF_FILE"
	if ! grep -q "PACKAGE_VERSION=\"$CUSTOM_VERSION\"" "$DKMS_CONF_FILE"; then
		echo "Error: Version update failed" >&2
		exit 1
	fi
fi

VERSION=$(grep -oP '^PACKAGE_VERSION="\K[^"]+' "$DKMS_CONF_FILE")
[[ -z "$VERSION" ]] && { echo "Error: Failed to parse PACKAGE_VERSION!" >&2; exit 1; }

TARGET_DIR="/usr/src/${PACKAGE_NAME}-${VERSION}"

dkms status | { grep "${PACKAGE_NAME}" || true; } | awk -F'[/ ,]+' '{print $1,$2}' | while read mod ver; do
	echo "Removing module: $mod version: $ver"
	sudo dkms remove -m $mod -v $ver --all
	src_dir="/usr/src/${mod}-${ver}"
	if [ -d "$src_dir" ]; then
		echo "Cleaning source directory: $src_dir"
		sudo rm -rf "$src_dir"
		[ ! -d "$src_dir" ] && echo "Successfully removed" || echo "Failed to remove"
	else
		echo "Directory does not exist: $src_dir (No cleanup needed)"
	fi
done

[[ -d "$TARGET_DIR" ]] && {
  echo "Cleaning residual directory: $TARGET_DIR"
  sudo rm -rf "$TARGET_DIR"
}

echo "Deploying driver to system directory..."
sudo cp -rp "$PROJECT_ROOT" "$TARGET_DIR"

pushd "$TARGET_DIR" >/dev/null
trap 'popd >/dev/null' EXIT

echo "Registering DKMS module..."
sudo dkms add -m "$PACKAGE_NAME" -v "$VERSION" || {
  echo "DKMS installation failed! Exit code: $?" >&2
  exit 1
}
echo "Building and installing kernel module..."
sudo dkms install -m "$PACKAGE_NAME" -v "$VERSION" || {
  echo "DKMS installation failed! Exit code: $?" >&2
  exit 1
}

echo "Configuring module auto-load at boot..."
echo "$PACKAGE_NAME" | sudo tee "/etc/modules-load.d/${PACKAGE_NAME}.conf" >/dev/null

echo "Updating initramfs image..."
sudo update-initramfs -u
