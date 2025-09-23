import argparse
import gdb
import json
import pathlib

def get_locals(size_dict: dict):
    frame = gdb.selected_frame()
    fname = frame.name()
    func_dict = size_dict.get(frame.name(), [])
    if len(func_dict) > 0:
        return
    while frame.is_valid():
        frame_dict = {} 
        block = frame.block()
        names = set()
        while block:
            if(block.is_global):
                print()
                print('global vars')
            for symbol in block:
                if (symbol.is_argument or symbol.is_variable):
                    name = symbol.name
                    if not name in names:
                        # Get size
                        size = gdb.execute(f"p sizeof({name})", to_string=True).split()[-1]
                        print('{} size {}'.format(name, size))
                        names.add(name)
                        frame_dict[name] = size
            block = block.superblock
        func_dict.append(frame_dict)
        gdb.execute("next", to_string=True)
    size_dict[fname] = func_dict

def setup_file():
    #gdb.execute(f'file {file}', to_string=True)
    gdb.execute('break CROSS_keygen', to_string=True)
    gdb.execute('break CROSS_sign', to_string=True)
    gdb.execute('break CROSS_verify', to_string=True)
    gdb.execute('run', to_string=True)

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-f", "--file", help="Bin file to run and hook", type=pathlib.Path)

    args = parser.parse_args()

    print(gdb.execute("info file", to_string=True).split("\n"))
    # Get filename
    fname = ""
    next = False
    for line in gdb.execute("info file", to_string=True).split("\n"):
        if next:
            fname = line.split()[0].strip(",'")
            break
        elif line == "Local exec file:":
            next = True
    fname = pathlib.Path(fname).name


    setup_file()
    # Where we record the data
    size_dict = {}
    # While still running
    while len(gdb.selected_inferior().threads()) > 0:
        if gdb.selected_inferior().threads()[0].is_stopped():
            print("Getting locals")
            get_locals(size_dict)
        gdb.execute("continue", to_string=True)
    print("finish")
    enc = json.JSONEncoder(indent=4)
    with open(f"{fname}.locals", "w") as f:
        f.write(enc.encode(size_dict))

    gdb.execute("exit")


if __name__ == "__main__":
    main()
