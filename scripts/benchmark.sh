#!/usr/bin/env bash

python3 ./benchmarks.py -p nucleo-l4r5zi --nosize --nohashing --nostack -u /dev/ttyACM1 -t 30 -i 10 -s "crossv2.0" --no-mem
