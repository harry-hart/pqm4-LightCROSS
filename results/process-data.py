import argparse
import pandas as pd
import pathlib
import matplotlib.pyplot as plt

def process_data(data: pd.DataFrame):
    # Split into three dataframes? Memory, speed, hashing
    pass

def load_data(path: pathlib.Path) -> pd.DataFrame:
    return pd.read_csv(path)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-f", "--file", help="The csv produced by the benchmarks.", type=pathlib.Path)

    args = parser.parse_args()

    data = load_data(args.file)

    print(data.head())

    process_data(data)



if __name__ == "__main__":
    main()
