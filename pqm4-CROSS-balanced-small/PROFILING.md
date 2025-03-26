# Profiling

An important part of optimising this scheme will be memory profiling it
on chip and finding where the bottlenecks are.

## Approach Idea #1: Poor Man's Profiler (+ Flamegraph)

The idea is to use the [poor man's profiler](https://poormansprofiler.org/) idea and 
just randomly sample the program using gdb at different points. Record
the current function using backtrace, then construct a flamegraph with this
information. 

Problems:
- How do we adapt this to remote/embedded targets?
- We may need to write a special "sign" binary that just runs the sign function on loop so we can properly jump in at
  different times.
