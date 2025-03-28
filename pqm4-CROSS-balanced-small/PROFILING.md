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

Got it kind of working with the profile.sh/profile.gdb scripts. Currently they are janky and the output is a bit
unreadable but with a bit of tweaking could get it clean. But I don't think this is the path we want. 

What do we want?

> Run all the algorithms on the embedded chip with smallest footprint.

What is the footprint?

> The amount of **memory** used to run

What is the poor man's profiler doing?

> Measuring where on average we spend our **time**.

Importantly, though we would ideally reduce both memory and time, memory is the more important factor. We want to
profile what parts of the algorithm use the most memory.

I think we follow the lead of the stack test for now and just care about stack, I don't even know if we use any heap
memory, we don't seem to ever allocate anything. 
