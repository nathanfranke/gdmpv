#!/bin/bash

set -e
export LC_ALL=C

cd "$(dirname "$0")/mpv-build"

rm *_options

#echo "--help" > ffmpeg_options
echo "--enable-static-build" > mpv_options

export CFLAGS="-fPIC"

./update
./clean

#scripts/fribidi-config
#scripts/fribidi-build $@
scripts/libass-config
scripts/libass-build $@
scripts/ffmpeg-config
scripts/ffmpeg-build $@

#export LDFLAGS="-L$(pwd)/build_libs/lib -Wl,-Bstatic -l:libass.a -l:libavcodec.a -l:libavdevice.a -l:libavfilter.a -l:libavformat.a -l:libavutil.a -l:libpostproc.a -l:libswresample.a -l:libswscale.a"

scripts/mpv-config
scripts/mpv-build $@
