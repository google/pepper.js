#!/bin/bash
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

SCRIPT_DIR="$(cd $(dirname $0) && pwd)"
cd ${SCRIPT_DIR}

OUT_DIR=${SCRIPT_DIR}/out
NACLAM_URL=https://github.com/binji/NaClAMBase
NACLAM_DIR=${OUT_DIR}/NaClAMBase
NACLAM_TAG=1.0.1

readonly TOOLCHAIN=${1:-pnacl}
readonly CONFIG=${2:-Release}

export PEPPERJS_SRC_ROOT=${SCRIPT_DIR}/../..

if [ -z "${NACL_SDK_ROOT:-}" ]; then
  echo "-------------------------------------------------------------------"
  echo "NACL_SDK_ROOT is unset."
  echo "This environment variable needs to be pointed at some version of"
  echo "the Native Client SDK (the directory containing toolchain/)."
  echo "NOTE: set this to an absolute path."
  echo "-------------------------------------------------------------------"
  exit -1
fi

Banner() {
  echo "######################################################################"
  echo $*
  echo "######################################################################"
}

# echo a command to stdout and then execute it.
LogExecute() {
  echo $*
  $*
}

Clone() {
  local url=$1
  local dir=$2
  local tag=$3
  if [ ! -d $dir ]; then
    Banner "Cloning $url => $dir"
    LogExecute git clone -b $tag $url $dir
  else
    Banner "Fetching $url"
    pushd $dir
    LogExecute git fetch origin
    LogExecute git checkout $tag
    popd
  fi

  Banner "Updating submodules"
  pushd $dir
  LogExecute git submodule update --init
  popd
}

readonly OS_NAME=$(uname -s)
if [ $OS_NAME = "Darwin" ]; then
  OS_JOBS=4
elif [ $OS_NAME = "Linux" ]; then
  OS_JOBS=`nproc`
else
  OS_JOBS=1
fi

BuildBullet() {
  Banner Building Bullet ${TOOLCHAIN}
  pushd ${NACLAM_DIR}
  LogExecute make NACL_ARCH=${TOOLCHAIN} ports
  popd
}

BuildNaClAM() {
  Banner Building NaClAM ${TOOLCHAIN}
  pushd ${NACLAM_DIR}
  LogExecute make -j${OS_JOBS} TOOLCHAIN=${TOOLCHAIN} CONFIG=${CONFIG}
  popd
}

Install() {
  local data_files="box.json example.js NaClAMBullet.js NaClAM.js scene.js \
      scenes.js three.min.js world.js"
  mkdir -p data
  for file in ${data_files}; do
    LogExecute cp ${OUT_DIR}/NaClAMBase/data/${file} data/
  done
  mkdir -p ${TOOLCHAIN}/${CONFIG}
  local SRC_DIR=${OUT_DIR}/NaClAMBase/${TOOLCHAIN}/${CONFIG}
  case ${TOOLCHAIN} in
    pnacl)
      if [ ${CONFIG} = "Debug" ]; then
        local exe_files="NaClAMBullet_x86_32.nexe NaClAMBullet_x86_64.nexe \
            NaClAMBullet_arm.nexe NaClAMBullet.nmf"
      else
        local exe_files="NaClAMBullet.pexe NaClAMBullet.nmf"
        if [ -f "${SRC_DIR}/NaClAMBullet_unstripped.pexe" ]; then
          local exe_files="${exe_files} NaClAMBullet_unstripped.pexe"
        fi
      fi
      ;;
    emscripten)
      local exe_files="NaClAMBullet.js"
      ;;
    *)
      echo "Unknown toolchain $TOOLCHAIN"
      exit 1
      ;;
  esac
  for file in ${exe_files}; do
    LogExecute cp ${SRC_DIR}/${file} ${TOOLCHAIN}/${CONFIG}
  done
}

Banner "Building for ${TOOLCHAIN} (${CONFIG})"

Clone ${NACLAM_URL} ${NACLAM_DIR} ${NACLAM_TAG}
BuildBullet
BuildNaClAM
Install

Banner Done!
