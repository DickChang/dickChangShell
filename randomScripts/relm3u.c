// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// R E L M 3 U
// Convert absolute paths in M3U playlist to relative paths
// Re-search relocated files to update playlist accordingly
// (C) 2020-2024 Julien Thomas
//
// THIS IS OPEN SOURCE FREEWARE, NO WARRANTIES, NO RESTRICTIONS
//
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//
// CHANGELOG:
//
// 20201121 Initial release, fully-functional 32 bit executables
// 20201128 Fixed some syntax and checking of search strings
// 20201206 Closing 'find' command pipe gracefully under all conditions
// 20201207 Changed 'find' invoke to case-insensitive file search
// 20201212 Use 'dir' extended search capability under Windows
// 20210508 Some clean-up on malformed relative paths, slashes vs backslashes
// 20210530 Recursive search over directory tree, improved back-up naming
// 20210606 Introduced testing mode with no write access at all
// 20210607 Added support for .m3u8 files in the recursive search
// 20230320 Hardened protocol/path filtering for references to mixed paths
// 20231220 Fixed some details on filenpaths, quoting rules on Linux console
//
// -----------------------------------------------------------------------------
//
#include <stdio.h>

#define PATHMAX  4096

// detect unix-based systems and define symbol UNIXES for briefness
#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define UNIXES
#endif

#ifdef _WIN32
    #define SLASH "\\"
#else
    #define SLASH "/"
#endif

// -----------------------------------------------------------------------------

// these few string manipulation routines were taken from 'joystring' library

int strlength(char *bstr)
{   // find length of null-terminated char array
    // return values from 0 to length (in byte characters)
int i = 0;  while (*bstr++)    { i++ ; }
return (i);
}

int strlefttrim (char *astr, char *cstr)
{   // remove left part of astr that is identical to cstr
// return(1) on trim success, otherwise (0)
// abort if one of strings is null
if (astr[0] == 0)   { return(0); }
if (cstr[0] == 0)   { return(0); }
int i = 0;
int j = 0;
// check up to where the strings are identical
while (astr[i] == cstr[i])  { i++; }
// abort when cstr was NOT fully identical with left from astr
if (cstr[i] != 0)   { return(0); }
// otherwise shift astr down by number of characters in cstr
j = i;
i = 0;
while (astr[j]) { astr[i++] = astr[j++]; }
astr[i] = 0;
return (1);
}

int strleftcutident(char *astr, char *cstr)
{   // remove only the identical part of astr and cstr
if (astr[0] == 0)   { return(0); }
if (cstr[0] == 0)   { return(0); }
int i = 0;
int j = 0;
while ((astr[i] == cstr[i]) && (astr[i] != 0)) { i++; }
// fast leave when astr is identical to cstr up to the end
if (astr[i] == 0)   { astr[0] = 0; return(1); }
// otherwise shift astr down by number of characters in cstr
j = i;
i = 0;
while (astr[j] != 0) { astr[i++] = astr[j++]; }
astr[i] = 0;
return (1);
}

int strleftcomp(char *astr, char *cstr)
{   // is cstr a left sided subset of astr ? (0/1)
if (astr == NULL) { return(0); }
if (cstr == NULL) { return(0); }
while ((*astr != 0) && (*cstr != 0))
    {   if (*astr != *cstr) { break; }
        astr++; cstr++;
    }
if (*cstr == 0)     { return(1); }
return(0);
}

int strcomp(char *astr, char *bstr)
{   // compare strings, return (1) when identical, return (0) when different
while ((*astr != 0) && (*bstr != 0))
    {   if (*astr != *bstr) { break; }
        astr++; bstr++;
    }
if ((*astr == 0) && (*bstr == 0)) { return(1); }
return(0);
}

int strfindchr(char *sourcestr, char searchchr)
{   // check for a single search character to exist within the source string
    // return (1) when found
    // return (0) when not found or search character was NULL
if (sourcestr[0] == 0)  { return (0); }
while (*sourcestr)  { if (*sourcestr++ == searchchr) { return(1); } }
return(0);
}

int strrightcomp(char *astr, char *cstr)
{   // is cstr a right sided subset of astr ?
int i = 0; while (astr[i] != 0) { i++; }
int j = 0; while (cstr[j] != 0) { j++; }
if (j > i)  { return(0); }
while (j > 0)  {   i-- ; j-- ;  if (astr[i] != cstr[j])  { return(0); }    }
return (1);
}

int strfindstr(char *sourcestr, char *searchstr)
{   // find first occurrence of search string within source string
    // and return direct index
    // or return (-1) if not found
if (sourcestr[0] == 0)  { return (-1); }
if (searchstr[0] == 0)  { return (-1); }
int i = 0;
int j = 0;
while ( sourcestr[i] != 0)
    {
        j = 0;
        while ( (unsigned)searchstr[j] == (unsigned)sourcestr[i+j])
            {
                j++;
                if (searchstr[j] == 0)     // found whole part str!
                    { return(i);  }
            }
    i++;
    }
return(-1);
}

void strlinetrim(char *astr)
{   // trim linestring from leading and trailing controls and whitespaces
int i = 0;
int j = 0;

// skip non-ascii characters (e.g. Unicode BOM or code points)
while (((unsigned char)astr[i] > 127) && (astr[i] != 0))  { i++; }

// left trim
while (((unsigned char)astr[i] < 33) && (astr[i] != 0))  { i++; }
j = i;
i = 0;
while (astr[j] != 0)
    {  astr[i++] = astr[j++]; }
astr[i] = 0;
// right trim
i = 0; while (astr[i]) { i++ ; }
while ((i > 0) && ((unsigned char)astr[i] < 33))
    {   astr[i] = 0; i--; }

return;
}

int strappendsafe(char *targetstr, int targetmax, char *appendstr)
{   // append targetstr with appendstr up to length of targetmax
int i = 0; while (targetstr[i] != 0) { i++; }
int j = 0;
while ((appendstr[j]) && (i < targetmax))  { targetstr[i++] = appendstr[j++]; }
targetstr[i] = 0;
if (i == targetmax) { return(0); }
return(1);
}

// -----------------------------------------------------------------------------

int urltostring(char *astr)
{   // convert 'percent-encoded' characters to ASCII/UTF-8 in the same string
// These 'Reserved Characters' pursuant to RFC 3986 must be decoded
// SPC  !   #   $   %   &   '   (   )   *   +   ,   /   :   ;   =   ?   @   [   ]
// %20 %21 %23 %24 %25 %26 %27 %28 %29 %2A %2B %2C %2F %3A %3B %3D %3F %40 %5B %5D
//
// fast hex-to-nibble conversion table
static unsigned char const H2N [] =
//  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 	// 0..9
    0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	// A..F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	//
    0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	// a..f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int i = 0;
int j = 0;
unsigned char a = 0;
while (astr[i] != 0)
    {
        if (astr[i] == '%')     // found 'percent-encoding'
            {
                // get byte value from two consecutive ascii hex digits
                i++;
                a =     H2N[ (unsigned char)astr[i] ]  << 4 ;   i++;
                a = a + H2N[ (unsigned char)astr[i] ]  ;        i++;
                astr[j++] = a;
            }
        else
            {   astr[j++] = astr[i++]; }
    }
astr[j] = 0;
return(1);
}

void backslashestoslashes(char *pathstring)
{
while (*pathstring != 0) { if (*pathstring == 92) { *pathstring = 47; } ; pathstring++; }
return;
}

void slashestobackslashes(char *pathstring)
{
while (*pathstring != 0) { if (*pathstring == 47) { *pathstring = 92; } ; pathstring++; }
return;
}

int check_file_exist(char *filepath)
{
#ifdef _WIN32
    slashestobackslashes(filepath);
    FILE *fp = fopen(filepath, "rb");
    backslashestoslashes(filepath);
    if (fp == NULL)     { return(0) ; }
    if (fclose(fp)!=0)  { return(0) ; }
#else
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL)     { return(0) ; }
    if (fclose(fp)!=0)  { return(0) ; }
#endif
return(1);
}

int get_only_filepath(char *pathname, char *fullpath)
{   // extract path only portion of a full path including last slash
int i = 0;
// copy fullpath[] to pathname[]
while (fullpath[i] != 0) { pathname[i] = fullpath[i]; i++; }
pathname[i] = 0;
// find last pathseparator
while (i > 0)
    {   i--;
        if ( (pathname[i] == 47) || (pathname[i] == 92)) { break; };
    }
// catch no-path condition, replace with current directory path
if (i == 0)  {  pathname[0] = 0;    }   else    {   pathname[++i] = 0;  }
return(1);
}

int get_only_filename(char *filename, char *fullpath)
{   // extract filename portion of fullpath string,
    // regardless of slashes or backslashes
int i = 0;
int j = 0;
// find to end of string
while (fullpath[i] != 0) { i++; }
// find last pathseparator
while (i > 0)
    {
        i--;
        if ( (fullpath[i] == 47) || (fullpath[i] == 92)) { break; };
    }
// catch no-path condition
if (i > 0) { i++; }
while (fullpath[i] != 0)    { filename[j++] = fullpath[i++]; }
filename[j] = 0;
return(1);
}

int dir_get_recurse_m3u_filepaths(char *pathfilenamestr)
{   // DIRECTORY-SEARCH OF ALL M3U FILES
// initial call : submit starting path to search (root of collection)
// return(1): deliver filename of any *.m3u found in pathfilenamestr
// return(0): no further filename to deliver, pathfilenamestr = nil
//
static FILE *fp;
static int ictr = 0;

// initial invoke triggers piped directory command
if (ictr == 0)
    {
        char cmdstr[PATHMAX] = "";

        #ifdef _WIN32

            slashestobackslashes(pathfilenamestr);
            sprintf (cmdstr, "dir /B /S /ON \"%s*.m3u?\" 2>&1", pathfilenamestr);

        #else

            // launch 'find' with search patterns of *.m3u and *.m3u8,

            if (strfindchr(pathfilenamestr, 39))
                {
                    sprintf (cmdstr, "find \"%s\" -maxdepth 5 -iname \"*.m3u\" -o -iname \"*.m3u8\" 2>&1",  pathfilenamestr);
                }
            else
                {
                    sprintf (cmdstr, "find '%s' -maxdepth 5 -iname '*.m3u' -o -iname '*.m3u8' 2>&1",  pathfilenamestr);
                }

            //~ sprintf (cmdstr, "find \"%s\" -maxdepth 5 -iname \"*.m3u\" -o -iname \"*.m3u8\" 2>&1",  pathfilenamestr);
            // which results in the command:
            // find "[PATH]" -maxdepth 5 -iname "*.m3u" -o -iname "*.m3u8"
            // find '[PATH]' -maxdepth 5 -iname '*.m3u' -o -iname '*.m3u8'

        #endif

        // puts(cmdstr);

        fp = popen(cmdstr, "r"); if (fp == NULL) { fclose(fp); return(0); }

        puts("");
    }

// deliver path of file found matching
ictr = 1 ;
if (fgets(pathfilenamestr, PATHMAX, fp) != NULL)
    {
        strlinetrim(pathfilenamestr);
        backslashestoslashes(pathfilenamestr);

        #ifdef _WIN32
        // catch general error in absence of absolute path indicators
        if (strfindchr(pathfilenamestr,'/') == 0)   { pathfilenamestr[0] = 0; ictr = 0; return(0); }
        if (strfindchr(pathfilenamestr,':') == 0)   { pathfilenamestr[0] = 0; ictr = 0; return(0); }
        #endif

        #ifdef UNIXES
        // Unix: catch general errors
        if (strleftcomp(pathfilenamestr, "find:"))  { pathfilenamestr[0] = 0; ictr = 0; return(0); }
        if (strfindchr(pathfilenamestr, ':') != 0)  { pathfilenamestr[0] = 0; ictr = 0; return(0); }
        #endif

        // trim control characters from line string
        if (pathfilenamestr[0] == 0)    { ictr = 0; return(0); }

        // return TRUE and deliver this line anyway with forward slashes
        return(1);
    }
else
    {   pclose(fp);   }
ictr = 0;
return(0);     // no more entries, finito
}

int dir_get_current_m3u_filepaths(char *pathfilenamestr)
{   // DIRECTORY-SEARCH OF M3U FILES ON CURRENT DIRLEVEL ONLY
// initial call : submit reference path to search within directory tree
// further calls: deliver each filename with .m3u path and return(TRUE)
// when no further file is found, deliver nil-string and return(FALSE)
//
// NOTE: Expects *pathfilenamestr to be generously dimensioned (4096 byte)
// WARNING: pathfilenamestr must not be a string literal (segmentation fault)
// since it is also used for return of found file names
//
static FILE *fp;
static int ictr = 0;


if  (   (strrightcomp(pathfilenamestr, ".m3u") == 0) &&
        (strrightcomp(pathfilenamestr, ".m3u8") == 0)      )
    {
        strappendsafe(pathfilenamestr, PATHMAX, SLASH);
        strappendsafe(pathfilenamestr, PATHMAX, "*.m3u");
    }

// initial invoke triggers piped directory command
if (ictr == 0)
    {
        char cmdstr[PATHMAX] = "";

        #ifdef _WIN32

        // use 'dir' in recursive subdirectory mode to get absolute paths
        slashestobackslashes(pathfilenamestr);
        sprintf (cmdstr, "dir /B /S /ON \"%s\" 2>&1", pathfilenamestr);

        #else

        // use 'find' which needs filepath and filepattern as separate arguments
        char filepathonly[PATHMAX] = ""; get_only_filepath(filepathonly, pathfilenamestr);
        char filenameonly[PATHMAX] = ""; get_only_filename(filenameonly, pathfilenamestr);

        if (strfindchr(pathfilenamestr, 39))
            {
                strappendsafe(cmdstr, PATHMAX, "find \"");
                strappendsafe(cmdstr, PATHMAX, filepathonly);
                strappendsafe(cmdstr, PATHMAX, "\" -maxdepth 1 -iname \"");
                strappendsafe(cmdstr, PATHMAX, filenameonly);
                strappendsafe(cmdstr, PATHMAX, "\" 2>&1");
            }
        else
            {
                strappendsafe(cmdstr, PATHMAX, "find '");
                strappendsafe(cmdstr, PATHMAX, filepathonly);
                strappendsafe(cmdstr, PATHMAX, "' -maxdepth 1 -iname '");
                strappendsafe(cmdstr, PATHMAX, filenameonly);
                strappendsafe(cmdstr, PATHMAX, "' 2>&1");
            }

        #endif

        // puts(cmdstr);

        fp = popen(cmdstr, "r"); if (fp == NULL) { fclose(fp); return(0); }

        puts("");
    }

// deliver path of file found matching
ictr++ ;
if (fgets(pathfilenamestr, PATHMAX, fp) != NULL)
    {
        strlinetrim(pathfilenamestr);
        backslashestoslashes(pathfilenamestr);

        #ifdef _WIN32
        // detect general error by absence of path separators
        if (!strfindchr(pathfilenamestr,'/'))   { pathfilenamestr[0] = 0; ictr = 0; return(0); }
        if (!strfindchr(pathfilenamestr,':'))   { pathfilenamestr[0] = 0; ictr = 0; return(0); }

        // when 'dir' in recursive mode will dive into any subdirectories,
        // suppress further lines when absolute path without filename grows longer
        static unsigned int i = 0;
        static unsigned int aplen = 0;
        if (ictr == 1) // run once
            {   aplen = 0; while (pathfilenamestr[aplen] != 0)     { aplen++; }
                while (aplen > 0) {   aplen--; if (pathfilenamestr[aplen] == 47) { break; } ; }
            }

        i = 0; while (pathfilenamestr[i] != 0)     { i++; }
        while (i > 0)       { i--; if (pathfilenamestr[i] == 47) { break; } ;  }
        if ( i > aplen )    { pclose(fp); pathfilenamestr[0] = 0; ictr = 0; return(0); }

        #endif

        #ifdef UNIXES
        // Unix: catch general error
        if (strleftcomp(pathfilenamestr,"find:")) { pathfilenamestr[0]=0; ictr=0; return(0); }
        #endif

        if (pathfilenamestr[0] == 0)    { ictr = 0; return(0); }

        // deliver current directory line in pathfilenamestr and return(TRUE)
        return(1);
    }
else
    {   pclose(fp);   }
ictr = 0;
return(0);     // no more entries, finito
}


int remove_protocol_and_drive_letters(char *sourcestr)
{   // search rightmost ':' character in a path, which may indicate protocol
    // prefix or drive letter, and remove all of them
    // return (1) when string was modified
    // return (0) when string was left untouched

if (sourcestr[0] == 0)  { return (0); }
int i = 0;
int j = 0;
while (sourcestr[i] != 0) { i++; } ; i--;
if (i == 0) { return(0); }
// get index position of a ':', search from the right of string
while ((i > 0) && (sourcestr[i] != ':'))  { i--; }
if (i == 0) { return(0); }  // no occurrence of ':', leave
j = i + 1;
i = 0;
// now shift string to the left
while (sourcestr[j] != 0) { sourcestr[i++] = sourcestr[j++]; }
sourcestr[i] = 0;
return(0);
}


// -----------------------------------------------------------------------------

// SEARCH METHOD 1 LINUX & WINDOWS
int find_relpath_by_pathprobing(char *pathfilename, char *pllpath)
{   // SEARCH METHOD 1: probe promising paths, deliver relative path
    // return (1) on success, return (0) on failure (file not found)

// puts ("PATHPROBING");

char *pathfilename_restore = pathfilename;  // original pointer to pathfilename
char probepath[PATHMAX];

//puts(probepath);

// remove one or more leading './' from pathfilename
while (strlefttrim(pathfilename, "./")) {};

// add leading '/' to relative path (for loop convenience)
int i = strlength(pathfilename);
if (pathfilename[0] != '/')
    {   pathfilename[++i] = 0;
        // shift characters backwards
        while (i > 0) { pathfilename[i] = pathfilename[i-1]; i--; }
        pathfilename[0] = '/';
    }

int updir = 0;
while (updir < 3)
    {
        // restore *pointer to start of pathfilename
        pathfilename = pathfilename_restore;

        while (*pathfilename != 0)
            {
                // skip to next directory level
                if (*pathfilename != '/') { pathfilename++; continue; }
                pathfilename++ ;

                switch (updir)
                    {
                        case 0 : { sprintf(probepath, "%s%s", pllpath, pathfilename); break; }
                        case 1 : { sprintf(probepath, "%s../%s", pllpath, pathfilename); break; }
                        case 2 : { sprintf(probepath, "%s../../%s", pllpath, pathfilename); break; }
                        case 3 : { sprintf(probepath, "%s../../../%s", pllpath, pathfilename);break; }
                        case 4 : { sprintf(probepath, "%s../../../../%s", pllpath, pathfilename);break; }
                        default: { sprintf(probepath, "%s%s", pllpath, pathfilename); }
                    }

                // test probe path
                if (check_file_exist(probepath))
                    {
                        // remove prepended playlist path and return relative path
                        strlefttrim(probepath, pllpath);

                        // compose complete relative path
                        if (strleftcomp(probepath, "../"))
                            { sprintf(pathfilename_restore, "%s", probepath); }
                        else
                            { sprintf(pathfilename_restore, "./%s", probepath); }

                        return(1);
                    }
            }
        updir++;
    }
// failed to find path, return original submitted path
return(0);
}

// SEARCH METHOD 2 for LINUX
#ifdef UNIXES
int shell_search_unix(char *dirlinestr, char *pathpatstr, char *searchfile)
{   // LINUX: outputs first file found in path (forward search)
    // pathpatstr is the reference path
    // searchfile is the filename to search for
    // returns (1) on success;      dirlinestr = fullpath+filename+extension
    // returns (0) no entry found;  dirlinestr = ""
FILE *fp;
char cmdstr[PATHMAX] = "";
sprintf (cmdstr, "find \"%s\" -maxdepth 7 -iname \"%s\" -type f 2>&1", pathpatstr, searchfile);
if ((fp = popen(cmdstr, "r"))== NULL)     { return(0); }

// invoke delivers full path of (first) matching filename
if (fgets(dirlinestr, PATHMAX, fp) != NULL)
    {
        // Unix: catch errors
        if (strleftcomp(dirlinestr, "find:"))   { pclose(fp); return(0); }
        if ( strfindchr(dirlinestr, ':'))       { pclose(fp); return(0); }
        if (!strfindchr(dirlinestr, '/'))       { pclose(fp); return(0); }
        strlinetrim(dirlinestr);
        pclose(fp);
        return(1);
    }
pclose(fp);
return(0);
}

// SEARCH METHOD 2 for LINUX
int find_relpath_by_search(char *pathfilestr, char *pllpath)
{   // LINUX: find relative path for a file from absolute pathfilestr, if possible
    // uses the mighty 'find' utility function on Unix/Linux
    // return (1) on success, return (0) on failure (file not found)

// int i;

int updir = 0;

char testpath [PATHMAX];
char searchpath[PATHMAX];

// isolate filename from path
char searchfile[1024] = ""; get_only_filename(searchfile, pathfilestr);

// joker-out misleading square brackets, which are yet allowed in filenames
int i = 0;
while (searchfile[i] != 0)
    {
        if (searchfile[i] == '[') { searchfile[i] = '?'; }
        if (searchfile[i] == ']') { searchfile[i] = '?'; }
        i++;
    }

while (updir < 3)
    {
        switch (updir)
            {
                case 0 : { sprintf(searchpath, "%s./", pllpath); break; }
                case 1 : { sprintf(searchpath, "%s../", pllpath); break; }
                case 2 : { sprintf(searchpath, "%s../../", pllpath); break; }
                case 3 : { sprintf(searchpath, "%s../../../", pllpath); break; }
                case 4 : { sprintf(searchpath, "%s../../../../", pllpath); break; }
                default: { sprintf(searchpath, "%s./", pllpath); }
            }

        // use shell 'find' with filename 'searchfile' from path 'searchpath'
        if (shell_search_unix(testpath, searchpath, searchfile))
            {       // remove prepended playlist path and return relative path
                    strlefttrim(testpath, pllpath);
                    sprintf(pathfilestr, "%s", testpath);
                    return(1);
            }

        updir++;
    }
return(0);
}

#endif


// SEARCH METHOD 2 for WINDOWS
#ifdef __WIN32__
int shell_search_windows(char *dirlinestr, char *pathpatstr, char *searchfile)
{   // WINDOWS: outputs first found file's full path (forward)
    // using 'dir' utility from standard command shell
    // pathpatstr is the reference path
    // searchfile is the glob to search
    // returns (1) on success;      dirlinestr = fullpath+filename+extension
    // returns (0) no entry found;  dirlinestr = ""

slashestobackslashes(pathpatstr);   // necessary for command shell commands
//slashestobackslashes(searchfile);   // normally not necessary

// puts(dirlinestr);
// puts(pathpatstr);
// puts(searchfile);

FILE *fp;
char cmdstr[PATHMAX] = "";
// this 'dir' syntax delivers line-separated strings of *full path*
sprintf (cmdstr, "dir \"%s\\%s\" /B /S 2>&1", pathpatstr, searchfile);
//~ puts(cmdstr);

if ((fp = popen(cmdstr, "r")) == NULL)     { return(0); }
int i = 0;
if (fgets(dirlinestr, PATHMAX, fp) != NULL)
    {
        // trim control characters (trailing cr/lf) and drop empty line
        strlinetrim(dirlinestr);
        if (dirlinestr[i]==0)               { pclose(fp); return(0); }

        // convert path to forward slashes
        backslashestoslashes(dirlinestr);

        // detect no-path return
        if (!strfindchr(dirlinestr, '/'))   { pclose(fp); return(0); }
        if (!strfindchr(dirlinestr, ':'))   { pclose(fp); return(0); }

        pclose(fp);
        return(1);
    }
puts("UPS");
pclose(fp);
return(0);
}

// SEARCH METHOD 2 for WINDOWS
int find_relpath_by_search(char *pathfilestr, char *pllpath)
{   // WINDOWS: find relative path for a file from absolute pathfilestr, if possible
    // return (1) on success, return (0) on failure (file not found)

// puts("PATHSEARCH");

int updir = 0;

char probepath [PATHMAX];
char searchpath[PATHMAX];

backslashestoslashes(pllpath);
backslashestoslashes(pathfilestr);

// puts(pllpath);

// isolate filename from path
char searchfile[PATHMAX] = "";
get_only_filename(searchfile, pathfilestr);

while (updir < 3)
    {
       switch (updir)
            {
                case 0 : { sprintf(searchpath, "%s./", pllpath); break; }
                case 1 : { sprintf(searchpath, "%s../", pllpath); break; }
                case 2 : { sprintf(searchpath, "%s../../", pllpath); break; }
                case 3 : { sprintf(searchpath, "%s../../../", pllpath); break; }
                case 4 : { sprintf(searchpath, "%s../../../../", pllpath); break; }
                default: { sprintf(searchpath, "%s./", pllpath); }
            }

        // note: uses shell 'dir' with filename 'searchfile' on path 'searchpath'
        if (shell_search_windows(probepath, searchpath, searchfile))
            {
                //printf("PLL PATH  : <%s>\n", pllpath);
                // remove identical part of playlist path
                strleftcutident(probepath, pllpath);

                //printf("FOUNDPATH : <%s>\n", probepath);
                // restore directory-upsteps to preceede relative path

                switch (updir)
                    {
                        case 0 : { sprintf(pathfilestr, "./%s", probepath); break; }
                        case 1 : { sprintf(pathfilestr, "../%s", probepath); break; }
                        case 2 : { sprintf(pathfilestr, "../../%s", probepath); break; }
                        case 3 : { sprintf(pathfilestr, "../../../%s", probepath); break; }
                        case 4 : { sprintf(pathfilestr, "../../../../%s", probepath); break; }
                        default: { sprintf(pathfilestr, "./%s", probepath); }
                    }

                backslashestoslashes(pathfilestr);
                return(1);
            }

        updir++;
    }

return(0);
}
#endif


int convert_playlist_to_relative(char *m3ufilepath, int seriousflag)
{   // make playlist with original pathfilename but relative paths, as possible
char sourcefilename [PATHMAX] = "";
char targetfilename [PATHMAX] = "";
char linbuf[PATHMAX] = "";
char playlistpath[PATHMAX] = "";

get_only_filepath(playlistpath, m3ufilepath);

if (!check_file_exist(m3ufilepath)) { puts("FILE DOES NOT EXIST."); return(0); }

FILE *fr;
FILE *fw;

if (seriousflag == 1)       // SERIOUS MODE!
    {
        puts("SERIOUS MODE. PLAYLIST GONNA GET MODIFIED.\n");

        // CREATE BACK-UP FILE
        //
        // find free backup filename
        int bakfno = 0;
        while (bakfno < 99)
            {
                sprintf(targetfilename, "%s.%02d.bak", m3ufilepath, bakfno);
                bakfno++;
                if (check_file_exist(targetfilename)) { continue; } else { break; }
            }
        // make backup copy
        // sourcefilename = original m3ufilepath
        // targetfilename = numbered backup filename

        sprintf(sourcefilename, "%s", m3ufilepath);

        #ifdef _WIN32
        slashestobackslashes(sourcefilename);
        slashestobackslashes(targetfilename);
        #endif

        fr = fopen(sourcefilename, "r"); if (fr == NULL) { return(0); }
        fw = fopen(targetfilename, "w"); if (fw == NULL) { return(0); }
        while (fgets(linbuf, sizeof(linbuf), fr)) {  fputs(linbuf, fw);  }
        if (fclose(fw) != 0) { return(0) ; }
        if (fclose(fr) != 0) { return(0) ; }

        // backup filename is now sourcefile for conversion
        sprintf(sourcefilename, "%s", targetfilename);
        // original filename is now targetfile to overwrite
        sprintf(targetfilename, "%s", m3ufilepath);

        #ifdef _WIN32
        slashestobackslashes(sourcefilename);
        slashestobackslashes(targetfilename);
        #endif
    }

else                        // TESTING MODE

    {
        puts("TEST MODE. NO WRITE ACCESS. JUST INFO.\n");

        sprintf(sourcefilename, "%s", m3ufilepath);

        #ifdef UNIXES
        sprintf(targetfilename, "%s", "/dev/null");
        #endif

        #ifdef __WIN32__
        slashestobackslashes(sourcefilename);
        sprintf(targetfilename, "%s", "NUL:");
        #endif

    }

printf("PATH: \"%s\"\n\n", m3ufilepath);

//~ printf("SOURCE: %s\n", sourcefilename);
//~ printf("TARGET: %s\n", targetfilename);

// parse and modify playlist
fr = fopen(sourcefilename, "r"); if (fr == NULL) { return(0); }
fw = fopen(targetfilename, "w"); if (fw == NULL) { return(0); }

int filestotal = 0;
int filesfound = 0;

// process source file line by line
while (fgets(linbuf, sizeof(linbuf), fr))
    {
        // clean line ends from whitespaces and other unwanted stuff
        strlinetrim(linbuf);

        // discard lines that are empty after trimming
        if (linbuf[0] == 0)     { continue; }

        // discard all #EXT taglines
        if (linbuf[0] == '#')   { continue; }

        // convert backslashes to slashes
        backslashestoslashes(linbuf);

            // remove the file protocol prefix
            //strlefttrim(linbuf, "file://");

        // refined procedure: remove any protocol prefix and drive letters
        // (bug report 20230318, Richard)
        remove_protocol_and_drive_letters(linbuf);
        strlefttrim(linbuf, "/");

        // decode possible URL-style path
        urltostring(linbuf);

            // slash-out drive letter from an absolute dos/win path
            //if (linbuf[1] == ':')    {   linbuf[0] = '/'; linbuf[1] = '/';   }

            // discard lines with other protocol prefixes
            //if (strfindchr(linbuf,':'))    { continue; }

        // so this IS a candidate
        filestotal++;

        //puts(linbuf);
        if (find_relpath_by_pathprobing(linbuf, playlistpath))
            {   // file found on modified playlist path
                fprintf(fw,"%s\n", linbuf);
                printf("1: %s\n", linbuf);
                filesfound++;
            }
        else
            {   // try blind search ('find' / 'dir')
                if (find_relpath_by_search(linbuf, playlistpath))
                    {
                        fprintf(fw,"%s\n", linbuf);
                        printf("2: %s\n", linbuf);
                        filesfound++;
                    }
                else
                    {   printf("X: %s\n", linbuf);   }
            }
    }

printf("\nFOUND: %d / %d\n", filesfound, filestotal);
if (fclose(fw)!=0) { return(0) ; }
if (fclose(fr)!=0) { return(0) ; }
puts("");
return(1);
}

void compiler_version_info(void)
{   // PRINT COMPILER VERSION, DATE AND BITNESS OF THE EXECUTABLE AT BUILD TIME
char astr[12];
astr[0] = __DATE__[4] ; astr[1] = __DATE__[5] ; astr[2] = ' ' ;
astr[3] = __DATE__[0] ; astr[4] = __DATE__[1] ; astr[5] = __DATE__[2] ; astr[6] = ' ' ;
astr[7] = __DATE__[7] ; astr[8] = __DATE__[8] ; astr[9] = __DATE__[9] ; astr[10] = __DATE__[10] ;
astr[11] = 0;
printf ("Build %s with ", astr);
#ifdef __GNUC__
    printf ("GCC %s ", __VERSION__);
#else
    printf ("- other compiler - ");
#endif
printf ("for ");
#ifdef __unix__
printf ("Linux");       // tested
#endif
#ifdef __WIN32__
printf ("Windows");     // tested
#endif
#ifdef __APPLE__
printf ("MacOS");       // untested
#endif
#ifdef __DOS__
printf ("DOS");         // partially tested
#endif
printf (" %d bit", (char)(sizeof(long int)*8));
printf ("\n\n");
return;
}

// -----------------------------------------------------------------------------


int main(int argc, char *argv[])
{
puts("");
if (argc == 1)
    {
    puts("RELM3U  -  CONVERT M3U PLAYLISTS TO RELATIVE LOCAL PATHS\n");
#ifdef _WIN32
    puts("USAGE:\n");
    puts("relm3u [path] [serious-switch]\n");
    puts("EXAMPLES:\n");
    puts("Show this help screen");
    puts("relm3u");
    puts("");
    puts("SINGLE playlist; specify exact path+filename");
    puts("relm3u \"C:\\fullpath\\to\\music\\folder\\playlist.m3u\" [-s]\n");
    puts("MULTIPLE playlists in SAME DIRECTORY; use wildcards in filename");
    puts("relm3u \"C:\\fullpath\\to\\music\\folder\\*.m3u\" [-s]\n");
    puts("ALL playlists in directory AND SUBDIRECTORIES; path only");
    puts("relm3u \"C:\\fullpath\\to\\music\\\" [-s]");
    puts("");
    puts("RESULT:\n");
    puts("New M3U(s) with all paths relative to playlist's location.");
    puts("");
    puts("NOTES:");
    puts("Only by second argument '-s' or '--serious' changes are actually");
    puts("written to the playlist and a back-up file is created.");
    puts("With no further argument, default is safe testing mode.");
    puts("Paths containing whitespaces, wildcards and special characters");
    puts("must be quoted according to your command shell rules.");
    puts("Only existing local files are kept in the new playlist.");
    puts("Files not found, extended tags and URLs are removed.");
    puts("Recursive search through several directory levels");
    puts("may take a little longer. Do not be alarmed.");
    puts("This tool can deal with relocated files and playlists.");
    puts("Encoding fully compatible to ASCII/ISO-8859/UTF-8.");
    puts("Exotic encoding problems may be solved by switching codepage.\n");
#else
    puts("USAGE:\n");
    puts("./relm3u [path] [serious-switch]\n");
    puts("EXAMPLES:\n");
    puts("Show this help screen");
    puts("./relm3u");
    puts("");
    puts("SINGLE playlist; specify exact path+filename");
    puts("./relm3u '/fullpath/to/music/folder/playlist.m3u' [-s]\n");
    puts("MULTIPLE playlists in SAME DIRECTORY; use wildcards in filename");
    puts("./relm3u '/fullpath/to/folder/*.m3u' [-s]\n");
    puts("ALL playlists in directory AND SUBDIRECTORIES; path only");
    puts("./relm3u '/fullpath/to/music/' [-s]");
    puts("");
    puts("NOTES:");
    puts("Only by second argument '-s' or '--serious' changes are actually");
    puts("written to the playlist and a back-up file is created.");
    puts("With no further argument, default is safe testing mode.");
    puts("Paths containing whitespaces, wildcards and special characters");
    puts("must be quoted according to your command shell rules.");
    puts("Only local files that actually exist are kept in the new playlist.");
    puts("Files not found, extended tags and URLs are removed.");
    puts("Recursive search through several directory levels");
    puts("may take a little longer. Do not be alarmed.");
    puts("This tool can deal with relocated files and playlists.");
    puts("Encoding fully compatible to ASCII/ISO-8859/UTF-8.\n");
#endif
    puts("(C) 2020-2024 Julien Thomas [jtxp.org]");
    puts("");
    puts("THIS IS OPEN SOURCE FREEWARE; NO WARRANTIES, NO RESTRICTIONS.");
    puts("");
    compiler_version_info();
    return(0);
    }

puts ("R E L M 3 U");

// int c = 0; printf("argc: %d\n",argc); while(c<argc) { printf("%d\t<%s>\n",c,argv[c]); c++; }

// parse for second argument which may or may not be the serious switch
int serious = 0;    // safe default

if (argc == 3)
    {
        if (strrightcomp(argv[2], "s"))        { serious = 1; }
        if (strrightcomp(argv[2], "serious"))  { serious = 1; }
    }
if (argc > 3)
    {
        puts("TOO MANY ARGUMENTS. BYE."); return(1);
    }

if (strlength(argv[1]) < 1)
    {
        puts("REFERENCE PATH TOO SHORT. BYE."); return(1);
    }

int j = 0;

// copy submitted path/filename for further processing
char cstr[PATHMAX] = "";

if ( (strfindchr(argv[1],47)) || (strfindchr(argv[1],92)) )
    { sprintf(cstr, "%s", argv[1]); }
else
    { sprintf(cstr, "./%s", argv[1]); }

// ALWAYS use forward slashes internally
backslashestoslashes(cstr);

// printf("\nARGV[1] resolved to: <%s>\n", cstr);

// DECIDE ON DIRECTORY-ONLY OR RECURSIVE PROCESSING MODE
if (strrightcomp(cstr, "/") || strrightcomp(cstr, "./")  )
    {
        puts("M3U SEARCH IN SUBMITTED DIRECTORY AND SUBDIRECTORIES\n");
        while (dir_get_recurse_m3u_filepaths(cstr))
            {
                //printf("M3U: <%s>\n", cstr);

                if (convert_playlist_to_relative(cstr, serious))
                    { puts("SUCCESS."); j++; }
                else
                    { puts("FAILED."); }
                puts("");
            }
    }
else
    {
        puts("M3U SEARCH IN SUBMITTED DIRECTORY ONLY\n");
        while (dir_get_current_m3u_filepaths(cstr))
            {
                //printf("M3U: <%s>\n", cstr);

                if (convert_playlist_to_relative(cstr, serious))
                    { puts("SUCCESS."); j++; }
                else
                    { puts("FAILED."); }
                puts("");
            }
    }


// concluding a little statistics
puts("");
if (j == 0) { printf("NO FILE PROCESSED.\n");   return(1);  }
if (j <  2) { printf("%d PLAYLIST FILE PROCESSED.\n",j);    }
else        { printf("%d PLAYLIST FILES PROCESSED.\n",j);   }

puts("\nFINISHED.\n");

return (0);
}


