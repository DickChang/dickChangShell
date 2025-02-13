import argparse
import csv
import eyed3
import logging
import os
import pandas
import re
import sys

valid_extensions = [".mp3", ".m4a"]
global_error_tracker = {
    "inputs": 0,
    "missing_tags": 0,
    "missing_album": 0,
    "missing_track_num": 0,
    "missing_release_year": 0
    }

logging.basicConfig(level=logging.INFO,
                    format='%(asctime)s [%(levelname)-5.5s] %(message)s',
                    handlers=[
                        logging.FileHandler(filename='autogen_playlists.log', mode='w', encoding='utf-8'),
                        logging.StreamHandler(sys.stdout)
                        ]
                    )
logger = logging.getLogger(__name__)

def create_playlist_dir(base_dir):
    if(not os.path.isdir(base_dir)):
        logger.error(f'given directory does not exist: {base_dir}')
        exit()

    playlist_dir = os.path.join(base_dir,"playlists")
    if(not os.path.isdir(playlist_dir)):
        os.mkdir(playlist_dir)
        logger.info(f'mkdir {playlist_dir}')
        if(not os.path.isdir(playlist_dir)):
            logger.error(f'mkdir failed')
            exit()
        else:
            logger.info(f'Created Output Directory: {playlist_dir}')
    else:
        logger.info(f'{playlist_dir} already exists.')

    return playlist_dir

# args
#   root_path       full path to parent directory where the folder is
#   folder          the name of the folder
#   playlists_path  full path to folder we will save playlists in
def generate_playlist(root_path, folder, playlists_path):

    folder_path = os.path.join(root_path, folder)
    logger.info(f'Generating playlist for {folder_path}')

    error_tracker = {
        "inputs": 0,
        "missing_tags": 0,
        "missing_album": 0,
        "missing_track_num": 0,
        "missing_release_year": 0
        }

    playlist = []
    for root, dirs, files in os.walk(folder_path, topdown=True):
        for file in files:
            filename, fileext = os.path.splitext(file)
            if not fileext in valid_extensions:
                continue
            error_tracker["inputs"] += 1
            global_error_tracker["inputs"] += 1

            full_path = os.path.normpath(os.path.join(root,file))
            # we assume the playlist is in the playlist folder, so we replace the root dir with ..
            rel_path = full_path.replace(root_path,"..")
            logger.debug(f'root_path: {root_path}')
            logger.debug(f'full_path: {full_path}')
            logger.debug(f'rel_path: {rel_path}')
            logger.info(f'  Processing music file {rel_path}')

            # default values in case the tag is missing info
            album = "AAAAA"
            track_num = 0
            year = 0

            song_tag = eyed3.load(full_path)
            if(song_tag == None or song_tag.tag == None):
                error_tracker["missing_tags"] += 1
                global_error_tracker["missing_tags"] += 1
                logger.error(f'    No tag found')
            else:
                if(song_tag.tag.album == None):
                    error_tracker["missing_album"] += 1
                    global_error_tracker["missing_album"] += 1
                    logger.error(f'    No album found in tag')
                else:
                    album = song_tag.tag.album

                if(song_tag.tag.track_num == None):
                    error_tracker["missing_track_num"] += 1
                    global_error_tracker["missing_track_num"] += 1
                    logger.error(f'    No track_num found in tag')
                else:
                    if type(song_tag.tag.track_num) is eyed3.core.CountAndTotalTuple:
                        track_num = song_tag.tag.track_num[0]
                    else:
                        track_num = song_tag.tag.track_num

                #if(song_tag.tag.best_release_date == None):
                #    debug_log(errorlogfile, "ERROR No release date found :: " + full_path + "\n")
                #year = song_tag.tag.best_release_date
                year = song_tag.tag.getBestDate()
                if(year == None):
                    error_tracker["missing_release_year"] += 1
                    global_error_tracker["missing_release_year"] += 1
                    logger.error(f'    No release year')
                else:
                    year_match = re.search(r"^[0-9]{4}",str(year))
                    if(year_match != None):
                        year = year_match.group()
                logger.debug(f'    year: {year}, album: {album}, track_num: {track_num}')

            song = {"rel_file_path": rel_path, "album": album, "track_num": track_num, "year": year}
            playlist.append(song)

    logger.info(f"Total input tracks processed: {error_tracker["inputs"]}")
    logger.info(f"  tracks missing tags: {error_tracker["missing_tags"]}")
    logger.info(f"  tracks missing albums: {error_tracker["missing_album"]}")
    logger.info(f"  tracks missing track numbers: {error_tracker["missing_track_num"]}")
    logger.info(f"  tracks missing release years: {error_tracker["missing_release_year"]}")

    columns = ["rel_file_path","album","track_num","year"]
    playlist = pandas.DataFrame(playlist, columns = columns)
    playlist['rel_file_path'] = playlist['rel_file_path'].str.replace('\\','/')
    try:
        playlist = playlist.sort_values(by=["year","album","track_num"], ascending = (True, True, True))
    except Exception as error:
        logger.error(f'Problem sorting the playlist.\n{playlist}\n{error}')
        exit()
    
    playlist_file_path = os.path.join(playlists_path,"!_"+folder+".m3u8")
    #playlist['rel_file_path'].to_csv(playlist_file_path, sep=' ',
    #                                 index=False,
    #                                 header=False,
    #                                 mode="w",
    #                                 encoding="utf-8",
    #                                 quoting=csv.QUOTE_NONE) # this doesn't work. generates with quotes
    playlist_file = open(playlist_file_path, "w", encoding="utf-8")
    for file in playlist['rel_file_path'].tolist():
        playlist_file.write(f"{file}\n")
    playlist_file.close()

    logger.info(f'Created playlist {playlist_file_path}')


def main():
    parser = argparse.ArgumentParser(prog="autoget_playlists",
        description='Generate a playlist for the supplied folder.\n'+
                    'Or select --all to make a playlist for each folder in the supplied directory.\n'+
                    'Paths will be generated relative to the "Music" folder.')
    parser.add_argument('-a', '--all', help='Include this to create a playlist for every folder in the supplied input folder', default=False, action='store_true')
    parser.add_argument('-i', '--input', help='The full path to the folder to generate a playlist for.', type=str, required=True)
    args = parser.parse_args()

    logging.basicConfig(filename='autogen_playlists.log', level=logging.INFO, filemode='w', encoding="utf-8",
                        format='%(asctime)s %(levelname)s %(message)s')
    logger.info('Starting autogen_playlists.py...')

    input_dir = os.path.normpath(args.input)
    all = args.all

    logger.info(f"autogen_playlists.py")
    logger.info(f"Input directory: {input_dir}")
    if all:
        logger.info(f'--all option given. Generating playlists for all folders in given input directory.')
    if "Music" not in input_dir:
        logger.error(f'Please give full path to the input folder, including the "Music" folder.')
        exit()

    if all:
        # folders to skip
        skip = ["playlists", "Bill Cosby", "Random", "Barkley OST"]

        playlists_dir = create_playlist_dir(input_dir)

        all_dirs = next(os.walk(input_dir))[1] # we'll create a playlist for each dir in this list
        for folder in all_dirs:
            if folder in skip:
                continue
            generate_playlist(input_dir, folder, playlists_dir)
    else:
        input_dir, folder = os.path.split(input_dir)
        if "Music" not in input_dir:
            logger.error(f'Must supply the folder that you intend to make a playlist out of. Or maybe you meant to use --all')
            exit()

        playlists_dir = create_playlist_dir(input_dir)
        generate_playlist(input_dir, folder, playlists_dir)

    logger.info(f'COMPLETE')
    logger.info(f'    Files processed {global_error_tracker['inputs']}')
    logger.info(f'    Files missing tags {global_error_tracker['missing_tags']}')
    logger.info(f'    Files missing album tag {global_error_tracker['missing_album']}')
    logger.info(f'    Files missing track number in tag {global_error_tracker['missing_track_num']}')
    logger.info(f'    Files missing release year in tag {global_error_tracker['missing_release_year']}')    

if __name__ == "__main__":
    main()
exit()
