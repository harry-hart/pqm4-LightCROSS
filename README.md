# LightCROSS

## Overview

An efficient and memory optimised implementation of the CROSS signature scheme. This repository is a fork of the
[pqm4 library](https://github.com/mupq/pqm4) with just the optimised CROSSv2.1 implementations included in `crypto_sign` and `mupq/crypto_sign`. 
The code in the `crypto_sign` and `mupq/crypto_sign` is exactly the same, the only difference is in the 
`crypto_sign/crossv2.0-sha3-r-sdp-1-small/light/parameters.h` the `OPT_DSP` flag is turned off for the `mupq/crypto_sign` variant to prevent it being platform specific. Thus the `mupq/crypto_sign` implementations are generic C optimisations, whereas the `crypto_sign` implementation is M4 specific.

There are two variants of the optimised CROSS implementation in this repository, `light` and `ultra`(light). The main difference being that `light` tries to match or beat the speed of the reference, whereas `ultra` attempts to use minimal memory, while allowing for some speed losses. This only difference is apparent in Signing.

This implementation achieves the following improvements over the reference:

**Memory:**
- Key Generation: 61-95\% Smaller
- Signing: 48-61(92*)\% Smaller (*ultra variant)
- Verifying: 71-83\% Smaller

**Speed:**
- Key Generation: 4-9\% *Slower*
- Signing: 0(-28*)-31(3*)\% Faster (*ultra variant)
- Verifying: 3-33\% Faster

Note the speed measurements are *with* the DSP optimisations. The optimisations
can be customised by turning various compiler flags in the `parameters.h` file on or off.

## Instructions

Note: All of the scripts were written and run with python-3.12.11. Some parts of the
scripts require features only available in python-3.12 or later. Please check your
python version if you have any errors.

### Benchmarking

1. Activate the python environment defined by `requirements.txt`, can be done with:
  ```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```
2. Connect the nucleo-l4r5zi board to your computer
3. Verify the serial connection, check which port it is on (e.g. `/dev/ttyACM0`)
4. Run `./scripts/benchmark.sh`.
  N.B. If it is not correctly connecting to the serial port with the device, make sure to check `/dev/ttyACM0` is correct (and adjust in the script if not)
5. Run `python3 ./convert_benchmarks.py csv > results/<result_file_name>.csv`
6. Run `python3 ./results/process-data.py -f ./results/<result_file_name>.csv`

#### Debugging Benchmarking

`serial.serialutil.SerialException: [Errno 2] could not open port /dev/ttyACM<number>`:
  This means that the board is not connected at the expected port. Please check that the
  correct port is in the script and it matches the one that the board shows up on your
  computer.

Lots of `Permission denied` errors:
  This can happen when trying to compile from a zip file. The easiest way to deal with this
  is by just setting user permissions on everything, `sudo chmod u+rwx -R .`.

Compilation error `expected identifier or '(' before '.' token`:
  Especially if it shows what look like relative paths in the error body. This means
  that the symlinks are broken in the repository. Usually it will be in two places the
  `mupq/crypto_sign` and `crypto_sign` directories. Run 
  `python3 ./scripts/fix-symlink.py -d ./mupq/crypto_sign` and 
  `python3 ./scripts/fix-symlink.py -d ./crypto_sign`
  and check if that has repaired the symlinks. Unsure if this works on Windows.

## Explanation of Optimisation Flags

You can adjust the optimisations by commenting or uncommenting the various
optimisation flags in the `parameters.h` file. To the `OPT_U_PRIME_EPH`
differentiates the `light` and `ultralight` variant.

- OPT_KEYGEN

  This flag turns on the main key generation optimisation, namely the on the fly matrix generation
  for syndrome calculation.

- OPT_MERKLE

  This flag turns on basic merkle tree optimisation which just involves not holding `cmt_0` and hashing the values directly into the leaves of the merkle tree.

- OPT_HASH_CMT1

  This utilises the iterative nature of the SHAKE hash to update-as-you-go while iterating through the leaves instead of collecting the values and hashing at the end. This removes the need to hold `cmt_1`.

- OPT_HASH_Y

  This is the same as the previous optimisation but for `y`.

- OPT_V_BAR

  Recalculates `v_bar` as needed from `e_bar_prime`.

- OPT_E_BAR_PRIME

  The same as above but reverse

- OPT_OTF_MERKLE

  Improves on the merkle optimisation by only holding `cmt_0` and not holding the merkle tree. Then using specialised `tree_root` and `tree_proof` algorithms to build the digest and the proof without needing the tree.

- OPT_GGM

  Removes the need to hold the GGM seed tree through `build_response` function.

- OPT_RECOMPUTE_ROOT

  This optimises the `recompute_root` function in verify to remove the need to hold `cmt_0` and a merkle tree.

- OPT_DSP

  This takes advantage of DSP intrinsics to speed up multiplication, especially vector-matrix multiplication.

- OPT_Y_U_OVERLAP

  This uses the fact that `y` and `u` are independently used one after another, allowing us to overlap them in memory usage.

- OPT_KEYGEN_BLOCKS

  This adds more nuance to the memory-efficient matrix optimisation from OPT_KEYGEN by allowing a small random buffer which is of size `R` and use that to sample from to reduce the number of CSPRNG calls required. This speeds up the algorithm.

- OPT_U_PRIME_EPH

  This flag used to allow for recalculation of `u_prime` and `v_bar` from the CSPRNG functions. This results in a large slowdown but also a large memory gain as these are some of the biggest variables in the implementation. Turn this on for the ultra variant.

- OPT_U_V_VERIFY

This allows for variable re-use overlap of the `u` and `v` variables in `CROSS_verify`
