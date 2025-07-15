# LightCROSS

## Overview

An efficient and memory optimised implementation of the CROSS signature scheme. This repository is a fork of the
[pqm4 library](https://github.com/mupq/pqm4) with just the optimised CROSSv2.1 implementations included in `crypto_sign` and `mupq/crypto_sign`. 
The code in the `crypto_sign` and `mupq/crypto_sign` is exactly the same, the only difference is in the 
`crypto_sign/crossv2.0-sha3-r-sdp-1-small/light/parameters.h` the `OPT_DSP` flag is turned off for the `mupq/crypto_sign` variant to prevent it being platform specific. Thus the `mupq/crypto_sign` implementations are generic C optimisations, whereas the `crypto_sign` implementation is M4 specific.

This implementation achieves the following improvements over the reference:

**Memory:**
- Key Generation: 58-95\% Smaller
- Signing: 48-60\% Smaller
- Verifying: 62-77\% Smaller
**Speed:**
- Key Generation: 22-33\% Slower
- Signing: 0-24\% Faster
- Verifying: 2-33\% Faster

Note the speed measurements are *with* the DSP optimisations. The optimisations
can be customised by turning various compiler flags in the `parameters.h` file on or off.

## Instructions

### Benchmarking

1. Activate the python environment defined by `requirements.txt`
2. Run `./scripts/benchmark.sh`.
  N.B. If it is not correctly connecting to the serial port with the device, make sure to check `/dev/ttyACM0` is correct (and adjust in the script if not)
3. Run `python3 ./convert_benchmarks.py csv > results/<result_file_name>.csv`
4. Run `python3 ./results/process-data.py -f ./results/<result_file_name>.csv`
