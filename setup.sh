#!/bin/bash

cd "$(dirname "$0")/mpv-build"

echo "--enable-libmpv-static --disable-gl" > mpv_options
./rebuild $@
rm mpv_options
