import urllib
import re

if __name__ == "__main__":
    # specify the save directory
    dl_dir = "/mnt/d/temp/"

    # specify the html file that is being parsed
    index_file = "/mnt/d/temp/index.html"

    # specify the root url where the files are located
    url_base = "http://mirror.centos.org/centos/7/sclo/x86_64/rh/devtoolset-7/"

    # pick out the files you want to download with this regex
    pattern = re.compile("href=\".+\.rpm")

    with open(index_file) as index:
        for line in index:
            dl_file = re.search(pattern, line)
            if(dl_file != None):
                dl_file = (dl_file.group(0)).split('\"')[1]
                urllib.urlretrieve(url_base + dl_file, dl_dir + dl_file)
                print url_base + dl_file
