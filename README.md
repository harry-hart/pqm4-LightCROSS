## TODO

  - [x] LightCROSS
    - [x] Key Generation
      - [x] First implementation (sdp-1-small)
      - [x] Port to all versions (sdpg, fast)
    - [x] Merkle Tree
      - [x] First implementation (sdp-1-small)
      - [x] Port to all versions (sdpg)
    - [x] OTF Shake Hash
      - [x] First implementation (sdp-1-small)
      - [x] Port to all versions (sdpg, fast)
  - [ ] New contributions
    - [x] e_bar_prime from v_bar derivation
    - [ ] Merkle Tree (+)
      - [x] Implement in Sign
      - [ ] Implement in Verify
    - [ ] Parallelise matrix multiplication (with DSP)

This repository contains two folders, pqm4-CROSS-balanced-small and pqm4-CROSS-fast. Our paper "LightCROSS: A Secure and Memory Optimized Post-Quantum Digital Signature CROSS" describes the implementation.  

  

## Development

There are a few scripts included for ease of development and saving settings. To get a debug environment
with a remote gdb server running on the microcontroller. 

1. In one terminal connect to the UART output (I use tio). You can use `serial-connect.sh`
2. In another terminal run `debug.sh <bin file>`. This will compile the hex for the bin, flash it to the board, then
  start the server.
3. In a third terminal connect with gdb `arm-none-eabi-gdb` to the matching **elf** file.
4. Target remote in gdb with `target remote:3333`

### Nix Notes

A lot of the development was done on a NixOS system, which is why there are nix dev shells available. Unfortunately
due to complications with:

- gcc-arm-embedded: GDB from this package does not work because of python3 version mismatch
- pkgsCross.build-packages.arm-embedded.gcc: GCC doesn't work from here because of a soft/hard float abi mismatch

So there is the `compile-shell.nix` for compiling the project with gcc-arm-embedded, then `debug-shell.nix` for debugging
and running the binaries on the board with gdb. 

## Setup 

Our code uses the [pqm4](https://github.com/mupq/pqm4) framework to test and benchmark on the [NUCLEO-L4R5ZI](https://www.st.com/en/evaluation-tools/nucleo-l4r5zi.html) board. So, we refer to the [pqm4](https://github.com/mupq/pqm4) documentation for the required essentials. 

  

All the `fast` (or `balance` and `small`) parameters of CROSS can be run using the following comments: 

Enter in the folder "pqm4-CROSS-fast" (or pqm4-CROSS-balanced-small) 

```
cd pqm4-CROSS-fast (or cd pqm4-CROSS-balanced-small) 
``` 

Compile the codes 

```
sudo make clean
sudo make -j4 PLATFORM=nucleo-l4r5zi 
``` 

Benchmark on the NUCLEO-L4R5ZI board 

```
sudo python3 benchmarks.py --platform nucleo-l4r5zi 
sudo python3 convert_benchmarks.py md 
``` 
