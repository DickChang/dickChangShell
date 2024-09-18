import os
import argparse

def main():
    parser = argparse.ArgumentParser(prog="relative_paths_m3u.py",
        description="Modify an m3u/m3u8 playlist to use relative paths and linux paths")
    parser.add_argument("-i", "--input", help="The m3u/m3u8 playlist file.", type=str, required=True)
    args = parser.parse_args()

    input = os.path.normpath(args.input)
    input_filename, input_ext = os.path.splitext(input)
    if(not (input_ext == ".m3u" or input_ext == ".m3u8")):
        print("ERROR: Supplied input was not an m3u/m3u8 playlist file.")
        exit()

    input_split = os.path.split(input)
    input_dir = input_split[0]
    
    with open(input, encoding="utf8") as input_fd:
        lines = [line.rstrip() for line in input_fd]
    for i, line in enumerate(lines):
        for line2 in lines[i+1:]:
            if(line == line2):
                print("FOUND DUPLICATE: "+line)

    exit()

if __name__ == "__main__":
    main()
exit()
