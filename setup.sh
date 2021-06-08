#!/bin/bash

cd "$(dirname "$0")/mpv-build"

echo "--enable-static" > ffmpeg_options
echo "--enable-libmpv-static" > mpv_options

CFLAGS="-fPIC" ./rebuild $@
rm *_options
