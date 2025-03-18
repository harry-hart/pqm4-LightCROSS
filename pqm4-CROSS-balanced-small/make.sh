#!/bin/bash

if [[ $1 == "-d" ]]; then
  D="DEBUG=1"
fi

make -j`nproc` PLATFORM=mps2-an386 $D
