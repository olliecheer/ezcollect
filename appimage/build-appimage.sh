#!/bin/bash
set -ex

SCRIPT_DIR=$(readlink -f $(dirname $0))
PRJ_ROOT=${SCRIPT_DIR}/..

APPIMAGE_BUILDER="appimage-builder"
APPIMAGE_RECIPE="${PRJ_ROOT}/appimage/AppImageBuilder.yml"

ARCH=$(uname -m)

if [[ $ARCH == "aarch64" ]]; then
  APPIMAGE_RECIPE="${PRJ_ROOT}/appimage/AppImageBuilder-${ARCH}.yml"
fi

cd $PRJ_ROOT

rm -rf build
mkdir -p build
cd build

cmake $PRJ_ROOT -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j$(nproc)
make install DESTDIR=./AppDir

mkdir -p ./AppDir/usr/share/icons
touch ./AppDir/usr/share/icons/empty.svg

# pip3 install packaging==21.3
$APPIMAGE_BUILDER --appdir ./AppDir --recipe $APPIMAGE_RECIPE
# $APPIMAGE_BUILDER --appdir ./AppDir --recipe ${PRJ_ROOT}/appimage/AppImageBuilder.yml

readlink -f ./ezcollect-latest*.AppImage
