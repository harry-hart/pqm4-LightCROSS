from mupq import mupq
import pathlib
from interface import parse_arguments, get_platform
import sys

class ProfilingBenchmark(mupq.StackBenchmark):
    test_type = 'profile'

def profile_process():
    results_dir = pathlib.Path("./benchmarks/profile")
    for algo_type in results_dir.iterdir():
        for scheme in algo_type.iterdir():
            for impl in scheme.iterdir():
                for res in impl.iterdir():
                    print(res)


def main():
    args, rest = parse_arguments()
    platform, settings = get_platform(args)
    with platform:
        schemes = [s for s in rest if s not in ['--nostack',
                                                '--nospeed',
                                                '--nohashing',
                                                '--nosize']]
        test = ProfilingBenchmark(settings, platform)
        if test.test_all(schemes):
            sys.exit(1)

if __name__=="__main__":
    main()
