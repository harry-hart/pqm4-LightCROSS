#!/usr/bin/env bash

sudo perf record -F 15000 -g ./build/bin/CROSS_KATgen_cat_1_sdp_small_ref
sudo chown harryhart:harryhart perf.data
perf report --stdio --no-children -n -g folded,0,caller,count -s comm | awk '/^ / {comm = $3 } /^[0-9]/ {print comm ";" $2, $1 }' > out.perf-folded
~/Source/Tools/FlameGraph/flamegraph.pl out.perf-folded > perf.svg
