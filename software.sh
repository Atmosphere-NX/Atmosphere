#!/bin/bash

set -o errexit
set -o xtrace

TMP_DIR="$(dirname $0)/tmp"
BUILD_DIR=$(if [[ -z $1 ]]; then echo "./"; else echo $1; fi)

UNZIP_COMMAND="unzip -o"

prepare_nx_hbmenu() {
    download_url=$(curl -s https://api.github.com/repos/switchbrew/nx-hbmenu/releases/latest | jq -r ".assets[0].browser_download_url")
    curl -O -L $download_url --output-dir $TMP_DIR
    $UNZIP_COMMAND $TMP_DIR/nx-hbmenu_*.zip -d $BUILD_DIR
}

prepare_nx_hbloader() {
    download_url=$(curl -s https://api.github.com/repos/switchbrew/nx-hbloader/releases/latest | jq -r ".assets[0].browser_download_url")
    curl -O -L $download_url --output-dir $TMP_DIR
    cp $TMP_DIR/hbl.nsp -d $BUILD_DIR/atmosphere
}

mkdir $TMP_DIR

prepare_nx_hbloader
prepare_nx_hbmenu

rm -rf $TMP_DIR
