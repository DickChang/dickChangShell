import os
import argparse
import pandas
import eyed3
import re

debug = True

def debug_init(d: str):
    logfile = open(d, "w", encoding="utf-8")
    return logfile

def debug_log(logfile, message: str):
    print("autogen_playlists log: "+message)
    if debug:
        logfile.write("autogen_playlists log:"+message)

def create_playlist_dir(logfile, errorlogfile, dir):
    playlist_dir = os.path.join(dir,"playlists")
    if(not os.path.isdir(dir)):
        debug_log(logfile,"ERROR: given directory does not exist: "+dir)
        errorlogfile.close()
        logfile.close()
        exit()
    if(not os.path.isdir(playlist_dir)):
        os.mkdir(playlist_dir) 
        debug_log(logfile,"mkdir "+playlist_dir+"\n")
        if(not os.path.isdir(playlist_dir)):
            debug_log(logfile,"ERROR: error making playlist directory: " + playlist_dir + "\n")
            errorlogfile.close()
            logfile.close()
            exit()
        else:
            debug_log(logfile,"Created Output Directory: "+playlist_dir+"\n")
    
    return playlist_dir

def generate_playlist(logfile, errorlogfile, dir, playlist_dir, root_dir):
    columns = ["rel_file_path","album","track_num","year"]
    error_tracker = {
        "inputs": 0,
        "missing_tags": 0,
        "missing_album": 0,
        "missing_track_num": 0,
        "missing_release_year": 0
        }

    folder = os.path.basename(dir)

    #playlist = pandas.DataFrame(columns = columns)
    playlist = []
    for root, dirs, files in os.walk(dir, topdown=True):
        for file in files:
            filename, fileext = os.path.splitext(file)
            if fileext != ".mp3":
                continue

            error_tracker["inputs"] = error_tracker["inputs"] + 1

            full_path = os.path.normpath(os.path.join(root,file))
            rel_file_path = full_path[(full_path.find(root_dir))+len(root_dir):]
            rel_file_path = ".." + rel_file_path.replace(os.sep, '/')

            album = "AAA"
            track_num = 0
            year = 0

            song_tag = eyed3.load(full_path)
            if(song_tag == None or song_tag.tag == None):
                error_tracker["missing_tags"] = error_tracker["missing_tags"] + 1
                debug_log(errorlogfile, "ERROR No tag found :: " + full_path + "\n")
            else:
                if(song_tag.tag.album == None):
                    error_tracker["missing_album"] = error_tracker["missing_album"] + 1
                    debug_log(errorlogfile, "ERROR No album found in tag :: " + full_path + "\n")
                else:
                    album = song_tag.tag.album

                if(song_tag.tag.track_num == None):
                    error_tracker["missing_track_num"] = error_tracker["missing_track_num"] + 1
                    debug_log(errorlogfile, "ERROR No track_num found in tag :: " + full_path + "\n")
                else:
                    #if type(song_tag.tag.track_num) is tuple:
                    if type(song_tag.tag.track_num) is eyed3.core.CountAndTotalTuple:
                        track_num = song_tag.tag.track_num[0]
                    else:
                        track_num = song_tag.tag.track_num

                #if(song_tag.tag.best_release_date == None):
                #    debug_log(errorlogfile, "ERROR No release date found :: " + full_path + "\n")
                #year = song_tag.tag.best_release_date
                year = song_tag.tag.getBestDate()
                if(year == None):
                    error_tracker["missing_release_year"] = error_tracker["missing_release_year"] + 1
                    debug_log(errorlogfile, "ERROR no release year: " + full_path + " :: " + str(year) + "::"+str(album)+"::"+str(track_num)+"\n")
                else:
                    year_match = re.search(r"^[0-9]{4}",str(year))
                    if(year_match != None):
                        year = year_match.group()
                    #elif(re.search("^[0-9]{2}$|^[0-9]{4}$",str(year)) == None):
                        #debug_log(errorlogfile, "ERROR no release year: " + full_path + " :: " + str(year) + "::"+str(album)+"::"+str(track_num)+"\n")

            song = {"rel_file_path": rel_file_path, "album": album, "track_num": track_num, "year": year}
            playlist.append(song)

    debug_log(errorlogfile, "Total input tracks processed: " + str(error_tracker["inputs"]))
    debug_log(errorlogfile, "  tracks missing tags: " + str(error_tracker["missing_tags"]))
    debug_log(errorlogfile, "  tracks missing albums: " + str(error_tracker["missing_album"]))
    debug_log(errorlogfile, "  tracks missing track numbers: " + str(error_tracker["missing_track_num"]))
    debug_log(errorlogfile, "  tracks missing release years: " + str(error_tracker["missing_release_year"]) + "\n")

    playlist = pandas.DataFrame(playlist, columns = columns)
    try:
        playlist = playlist.sort_values(by=["year","album","track_num"],ascending = (True, True, True))
    except Exception as error:
        debug_log(errorlogfile, "ERROR couldn't sort: " + playlist.to_string() + "\n")
        debug_log(errorlogfile, str(error)+"\n")
        exit()
    
    playlist_file = open(os.path.join(playlist_dir,"!_"+folder+".m3u8"), "w", encoding="utf-8")
    for file in playlist['rel_file_path'].tolist():
        playlist_file.write(file+"\n")
    playlist_file.close()



def main():
    parser = argparse.ArgumentParser(prog="autoget_playlists",
        description="Generate a playlist for the supplied folder. Or select --all to make a playlist for each folder in the supplied directory. Only gets mp3 files")
    parser.add_argument('--all', dest='all', default=False, action='store_true')
    parser.add_argument("-i", "--input", help="The parent directory.", type=str, required=True)
    args = parser.parse_args()

    input = os.path.normpath(args.input)
    all = args.all

    if all:
        logfile = debug_init(os.path.join(input, "autogen_playlists_log.txt"))
        errorlogfile = debug_init(os.path.join(input, "autogen_playlists_error_log.txt"))

        playlist_dir = create_playlist_dir(logfile, errorlogfile, input)

        alldirs = next(os.walk('.'))[1]

        # folders to skip
        skip = ["playlists", "Bill Cosby", "Random"]

        for folder in alldirs:
            current_dir = os.path.normpath(os.path.join(input,folder))

            if folder in skip:
                continue

            generate_playlist(logfile, errorlogfile, current_dir, playlist_dir, input)
    else:
        current_dir = os.path.normpath(input)
        root_dir = os.path.normpath(os.path.join(current_dir, ".."))

        logfile = debug_init(os.path.join(root_dir, "autogen_playlists_log.txt"))
        errorlogfile = debug_init(os.path.join(root_dir, "autogen_playlists_error_log.txt"))

        playlist_dir = create_playlist_dir(logfile, errorlogfile, root_dir)
        generate_playlist(logfile, errorlogfile, current_dir, playlist_dir, root_dir)

    debug_log(logfile, "COMPLETE\n\nLog files are at autogen_playlists_error_log.txt\n")

    errorlogfile.close()
    logfile.close()

if __name__ == "__main__":
    main()
exit()
