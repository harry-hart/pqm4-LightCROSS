import argparse
import gdb
import json
import pathlib

def get_locals(size_dict: dict):
    frame = gdb.selected_frame()
    fname = frame.name()
    func_dict = size_dict.get(fname, {})
    names = set()
    if len(func_dict.items()) > 0:
        return
    step = 0
    while frame.is_valid():
        block = gdb.selected_frame().block()
        lnames = set()
        while block:
            if(block.is_global):
                print()
                print('global vars')
            for symbol in block:
                if (symbol.is_argument or symbol.is_variable):
                    name = symbol.name
                    if name not in lnames:
                        # Get size
                        size = int(gdb.execute(f"p sizeof({name})", to_string=True).split()[-1])
                        print('{} size {}'.format(name, size))
                        lnames.add(name)
                        if name not in names:
                            func_dict[name] = [0 for _ in range(step)]
                        func_dict[name].append(size)
            block = block.superblock
        # Old variables not mentioned
        for name in names.difference(lnames):
            func_dict[name].append(0)
        #func_dict.append(frame_dict)
        names = names.union(lnames)
        step += 1
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
