import argparse
import os.path
from os import listdir
from os import mkdir
import subprocess
import time
import shutil
import re

debug = True

video_file_types = [".mkv",".avi",".mov",".mp4",".wmv"]

def debug_init(d: str):
    logfile = open(d, "w")
    return logfile

def debug_log(logfile, message: str):
    print("qBT_autorun_batch log: "+message)
    if debug:
        logfile.write("qBT_autorun_batch log:"+message)

# logfile - open file descriptor for log messages
# target_root_dir - absolute path to where output folders and files should go
# source_dir - absolute path to where the rar and movie files are
# file - name.ext of the file in question
def create_target_dir(logfile, errorlogfile, output_dir, source_dir):
    input_dir = source_dir[((source_dir).find("/BT/")+4):] # remove the root part of the torrent path
    target_dir = os.path.join(output_dir, input_dir)
    temp_dirs = input_dir.split("/")
    temp_dir = output_dir
    for dir in temp_dirs:
        temp_dir = os.path.join(temp_dir, dir)
        if(not os.path.isdir(temp_dir)):
            os.mkdir(temp_dir) 
            debug_log(logfile,"mkdir "+temp_dir+"\n")
    if(not os.path.isdir(target_dir)):
        debug_log(logfile,"ERROR: error making output directory: " + target_dir + "\n")
        errorlogfile.close()
        logfile.close()
        exit()
    else:
        debug_log(logfile,"Created Output Directory: "+target_dir+"\n")
    return target_dir

# logfile - open file descriptor for log messages
# output_dir - absolute path to where output folders and files should go
# source_dir - absolute path to where the rar and movie files are
# file - name.ext of the file in question
def extract_or_copy(logfile, errorlogfile, output_dir, source_dir, file):
    filename, fileext = os.path.splitext(file)
    if(fileext == ".rar" or ((fileext in video_file_types) and (re.search('sample', filename, re.IGNORECASE) == None))):
        print("Extracting or copying " + os.path.join(source_dir,file))
        target_dir = create_target_dir(logfile, errorlogfile, output_dir, source_dir)

        input_file = os.path.join(source_dir, file)
        if file.endswith('.rar'): # Extract rar
            if re.search("part[0-9]+.rar$",file) != None and re.search("part0*1.rar$",file) == None:
                debug_log(logfile,"\nSkipping \"partXX.rar\": "+input_file+"\n")
                return
            debug_log(logfile,"\nExtracting...\n")
            command = "yes Y | "+os.path.join(output_dir,"BT/7z23/7zzs")+" e \""+input_file+"\" -o\""+target_dir+"\""
        else:
            debug_log(logfile,"\nCopying...\n")
            command = "cp \""+input_file+"\" \""+target_dir+"\""

        debug_log(logfile,command+"\n")
        process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
        for line in iter(process.stdout.readline, b''):
            debug_log(logfile,str(line)+"\n")
        process.wait()
        debug_log(logfile, "    return code: "+str(process.returncode)+"\n")
        if(process.returncode != 0):
            debug_log(errorlogfile, "Error code "+str(process.returncode)+"\n")
            debug_log(errorlogfile, "    Input File"+input_file+"\n")
            debug_log(errorlogfile, "    Target Dir"+target_dir+"\n")

def main():
    parser = argparse.ArgumentParser(prog="unzipTorrents",
        description="Unzip a bunch of downloaded torrents to an approriate place for the media server.",
        epilog="(root dir for this server is /mnt/WDRed14/DCNas/)")
    parser.add_argument("-i", "--input", help="The parent directory with the torrents.", type=str, required=True)
    args = parser.parse_args()

    input = os.path.normpath(args.input)
    if (input).find("/BT/") == -1:
        # skip due to not being the BT folder
        exit()
    root_dir = args.input[:((input).find("/BT/"))] # get the root part of the torrent path
    source_folder = args.input[((input).find("/BT/")+4):] # remove the root part of the torrent path
    #output_dir = os.path.join(root_dir, source_folder) # this is the final output folder

    logfile = debug_init(os.path.join(root_dir, "BT/qBT_auto_batch_unzip_log.txt"))
    errorlogfile = debug_init(os.path.join(root_dir, "BT/qBT_auto_batch_unzip_error_log.txt"))
    debug_log(logfile,"\nBeginning Batch Extract...\n")
    debug_log(logfile,"  Source: "+input+"\n")
    #output_dir = create_target_dir(logfile, root_dir, input) # create the final output folder
    debug_log(logfile,"  Destination: "+root_dir+"\n")
    for root, dirs, files in os.walk(input, topdown=True):
        if(re.search("sample", os.path.basename(os.path.normpath(root)), re.IGNORECASE) != None):
           continue
        for f in files:
            extract_or_copy(logfile, errorlogfile, root_dir, root, f)
        #print(root)
        #print("  "+str(dirs))
        #print("    "+str(files))
    errorlogfile.close()
    logfile.close()

if __name__ == "__main__":
    main()
