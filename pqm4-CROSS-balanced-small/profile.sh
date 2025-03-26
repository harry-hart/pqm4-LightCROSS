#!/usr/bin/env bash

nsamples=1
sleeptime=0
#pid=$(pidof mysqld)
elf=$1

for x in $(seq 1 $nsamples)
  do
    gdb \
      -ex "set pagination 0" \
      -ex "target extended-remote :3333" \
      -ex "thread apply all bt" \
      -batch \
      $elf
    sleep $sleeptime
  done | \
awk '
  BEGIN { s = ""; } 
  /^Thread/ { print s; s = ""; } 
  /^\#/ { if (s != "" ) { s = s "," $4} else { s = $4 } } 
  END { print s }' | \
sort | uniq -c | sort -r -n -k 1,1
