This repository contains two folders, pqm4-CROSS-balanced-small and pqm4-CROSS-fast. Our paper "LightCROSS: A Secure and Memory Optimized Post-Quantum Digital Signature CROSS" describes the implementation.  

  

## Development

There are a few scripts included for ease of development and saving settings. To get a debug environment
with a remote gdb server running on the microcontroller. 

1. In one terminal connect to the UART output (I use tio). You can use `serial-connect.sh`
2. In another terminal run `debug.sh <bin file>`. This will compile the hex for the bin, flash it to the board, then
  start the server.
3. In a third terminal connect with gdb `arm-none-eabi-gdb` to the matching **elf** file.
4. Target remote in gdb with `target remote:3333`

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
