import pathlib
import re
import sys


# Args are same as rename
# 1. expression to replace
# 2. replacement
# 3. files to search
def main():
    (exp, rep, fil) = sys.argv[1:4]
    print(exp, rep, fil)
    root = pathlib.Path(".").resolve()
    print(f"Root {root}")
    # Change map
    mapping = {}
    # Rename all files
    for file in pathlib.Path(".").glob(fil):
        print(file)
        new_file_name = re.sub(exp, rep, file.name)
        old_file = file
        file.rename(new_file_name)
        mapping[old_file.resolve()] = file.parent.resolve() / new_file_name
    print(mapping)
    # Fix symlinks
    for (dirpath, dirnames, files) in pathlib.Path(".").walk():
        for f in files:
            file = dirpath / f
            if file.is_symlink() and exp in (target := str(file.readlink())):
                file = root / file
                print(f"File: {file}")
                print("******* File is symlink *******")
                target = file.readlink()
                print(f"Target: {target}")
                #for parent in target.parents:
                #    if parent == root:
                #        break
                #    print(f"Check {parent}")
                #    if parent in mapping:
                        #new_target_dir = mapping[parent]
                        # Get new file target
                        #new_target = new_target_dir / (target.relative_to(parent))
                        # Get relative path
                        #new_rel_target = new_target
                new_target = re.sub(exp, rep, str(target))
                if str(new_target) == str(target):
                    continue
                print(f"Change {file} target:\n\t{target} to\n\t{new_target}")
                file.unlink()
                file.symlink_to(new_target)




if __name__ == "__main__":
    main()
