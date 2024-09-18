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
    rel_playlist_file = open(os.path.join(input_dir,input_filename+"_rel"+input_ext), "w", encoding="utf-8")
    with open(input, encoding="utf8") as input_fd:
        for line in input_fd:
            if(line.startswith("﻿#")): # skip the m3u8 start line
                rel_playlist_file.write(line)
                continue
            #line = os.path.normpath(line) # normpath on window always uses '\' instead of '/'
            line = line.replace("\\", "/") # normpath on window always uses '\' instead of '/'
            line = os.path.join("./",line)
            rel_playlist_file.write(line)
    input_fd.close()
    rel_playlist_file.close()

    exit()

if __name__ == "__main__":
    main()
exit()
