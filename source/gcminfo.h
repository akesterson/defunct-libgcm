// thanks to groepaz and everyone else who wrote YAGCD, most of my info came from it
// also thanks to WntrMute for loaning me some source to clear things up about the FST

// (C) 2005 Andrew K andrew@aklabs.net
// License: Take all you want, but credit for all you take.

#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LIBGCMVERSION					"libGCM Version 0.1a (build Sep 4 2005)"

// defines for internal program control (not necessarily GCM-related)

#define GCNDVD_MAGICNUM                 0xc2339f3d  // if this isn't correct in the GCM, 
                                                    // the program will abort.

#define GCNDVD_SECTORS                  712880  // not really used, but left for future updates that might use it
#define GCNDVD_SECTORSIZE               2048    // " "
#define GCNDVD_FNAME_LENGTH             1024


#define GCM_MAX_PIECES                  29 // change this if (for whatever reason) you add a new entry in GCMPieces

// these all define indices in the GCMPieces global array where
// information about (where to look in the GCM for) a given piece
// of data about the GCM can be found. The rationale should
// be easy enough to follow.
#define GCM_DHEAD                       0
#define GCM_DHEADINF                    1
#define GCM_APPLOADER                   2

#define GCM_DHEAD_GAMECODE              3
#define GCM_DHEAD_GAMEMAKER             4
#define GCM_DHEAD_DVDMAGIC              5
#define GCM_DHEAD_GAMENAME              6
#define GCM_DHEAD_DEBUGMON              7
#define GCM_DHEAD_DEBUGMONADDR          8
#define GCM_DHEAD_BOOTFILE              9
#define GCM_DHEAD_FST                   10
#define GCM_DHEAD_FSTSIZE               11
#define GCM_DHEAD_FSTMAXSIZE            12
#define GCM_DHEAD_USERPOS               13
#define GCM_DHEAD_USERLEN               14

#define GCM_DHEADINF_DEBUGMONSIZE       15
#define GCM_DHEADINF_SIMMEMSIZE         16
#define GCM_DHEADINF_ARGOFFSET          17
#define GCM_DHEADINF_DEBUGFLAG          18
#define GCM_DHEADINF_TRACKLOCATION      19
#define GCM_DHEADINF_TRACKSIZE          20
#define GCM_DHEADINF_COUNTRYCODE        21

#define GCM_APPLOADER_VERSION           22
#define GCM_APPLOADER_ENTRY             23
#define GCM_APPLOADER_SIZE              24
#define GCM_APPLOADER_TRAILERSIZE       25
#define GCM_APPLOADER_BINARY            26
#define GCM_APPLOADER_CODE_LOC          0x2450

#define GCM_FST_ROOT                    28
#define GCM_FST_STRINGTABLEOFF          29

typedef struct {
    int area; // 0 = basic structure, 1 = header, 2= header info, 3 = apploader, 4 = FST
    char *name;
    long offset;
    long size;
    int type; // 1 for string, 0 for numeric value (strings aren't byteswapped)
} GCMPiece;

    // pieces with area 1 add the base of the disk header to their offset
    // pieces with area 2 add the base of the disk header info to their offset
    // pieces with area 3 add the base of the apploader to their offset
    // pieces with area 4 add the base of the FST to their offset (garnered from the disk header)
static GCMPiece GCMPieces [] ={
    // root disc pieces
    {0, "Disk Header (boot.bin)", 0x00000000, 0x0440, 0},
    {0, "Disk Header Information (bi2.bin)", 0x00000440, 0x2000, 0},
    {0, "Apploader (appldr.bin)", 0x00002440, 0x2000, 0}, // 0x2000 size is an estimate I think
    // disk header
    {1, "Game code", 0x0000, 0x0004, 0},
    {1, "Game maker code", 0x0004, 0x0002, 0},
    {1, "DVD Magic Word", 0x001c, 0x0004, 0},
    {1, "Game name", 0x0020, 0x03e0, 1},
    {1, "Debug monitor offset", 0x0400, 0x0004, 0},
    {1, "Debug monitor load address", 0x0404, 0x0004, 0},
    {1, "Bootfile offset", 0x0420, 0x0004, 0},
    {1, "FST offset", 0x0424, 0x0004, 0},
    {1, "FST size", 0x0428, 0x0004, 0},
    {1, "FST Max Size", 0x042c, 0x0004, 0},
    {1, "User position", 0x0430, 0x0004, 0},
    {1, "User length", 0x0434, 0x0004, 0},
    
    // the disk header info is loaded to 0x800000f4 when the disc is loaded by the IPL (tho we're reading it straight 
    // out of the .gcm image, so YMMV)
    {2, "Debug-monitor size", 0x0000, 0x0004, 0},
    {2, "Simulated memory size", 0x0004, 0x0004, 0},
    {2, "Argument offset", 0x0008, 0x0004, 0},
    {2, "Debug flag", 0x000c, 0x0004, 0},
    {2, "Track location", 0x0010, 0x0004, 0},
    {2, "Track size", 0x0014, 0x0004, 0},
    {2, "Countrycode", 0x0018, 0x0004, 0},
    
    // apploader header info
    {3, "Version/Date", 0x0000, 0x000F, 1}, // don't include the padding in the date
    {3, "Apploader entry point", 0x0010, 0x0004, 0},
    {3, "Apploader size", 0x0014, 0x0004, 0},
    {3, "Trailer size", 0x0018, 0x0004, 0},
    {3, "Apploader binary", 0x0020, 0, 1}, // no length given - taken from the "apploader size"
    
    // FST info
    {4, "Root directory entry", 0x00, 0x0c, 0},
    {4, "FST String table offset", 0x0008, 0x0004, 0}
    
};

// primary GCM disk header
typedef struct GCM_DiskHeader{
    int gamecode;
    int gamemaker;
    int magicword;
    char gamename[0x03e0];
    int debugmon;
    int debugmonaddr;
    int bootfile;
    int userpos;
    int userlen;
};

// the GCM disk header additional info
typedef struct GCM_DiskHeaderInfo {
    int debugmonsize;
    int simmemsize;
    int argoff;
    int debugflag;
    int tracklocation;
    int tracksize;
    int countrycode;
};

// information about the apploader on the GCM
typedef struct GCM_ApploaderInfo{
    char datever[10];
    int entry;
    int size;
    int trailersize;
    char apploader[0x200]; // actual apploader data
};

// information about a particular file in the FST
typedef struct GCM_FSTEntry {
    int fname_offset; // offset into the FST string table for the filename
                    // the offset is only actually 3 bytes, but 
                    // we're only reading, not writing so it doesn't matter
                    // if we're grande sized ....
                    // the first byte of fname_offset is actually the flags of the file; 0: file 1: dir
    int file_offset; // if this is a directory, this is actually the offset of the parent
    int file_length; // actually the number of entries (root) or next_offset (dir)
	int stringtable_off; // offset in the string table
};

// information about the names and locations of files in the GCM
typedef struct GCM_FST {
    int size;
    int offset;
    int stringtable_offset;
    int maxsize;
    std::vector <GCM_FSTEntry>entries; // list of entries
    std::vector <std::string *>stringtable; // string table
};

// GCM image data structure
typedef struct GCM {
    GCM_DiskHeader head;
    GCM_DiskHeaderInfo headInf;
    GCM_ApploaderInfo apploader;
    GCM_FST fst;
	char *fname;
};

// these are just the functions that the user would need beyond
// the functionality provided by gcmInit's loader; more internal
// functions can be found inside gcminfo.c

// returns 0 for a regular file, 1 for a directory
extern int gcmGetFSTEntryType(GCM_FSTEntry *entry);
// initialises the GCM data structures with FST and such;
extern int gcmInit(char *gcmname, GCM *data);
// prints a given portion of the GCM data
// whichdata should be an indice into the GCMPieces array
extern int gcmPrintData(int whichdata, GCM *data);
// prints all the data in the GCM in a (fairly) columnar format
extern int gcmPrint(GCM *data);
// prints a comprehensive listing of all files in the GCM
extern int gcmPrintFST(GCM *data);
// retrieves a given file and puts it in the FST entries,
// given the fnum
extern int gcmGetFile(GCM *data, GCM_FSTEntry *entry, int fnum, FILE *file);
// prints information for a given fname
extern int gcmPrintFile(GCM *data, int fnum);
// frees all data in the FST stringtable
extern int gcmFreeStringTable(GCM *data);
// extracts a file, given the fnum, into the current dir, unless outfname is specified
extern unsigned int gcmExtractFile(GCM *data, int fnum, char *outfname);
// puts the full filename (with path) into dest, given fnum
extern int gcmGetFullFileName(GCM *data, int fnum, char *dest);
// returns the fnum of a given file by its offset
extern int gcmGetFileByOffset(GCM *data, int offset);
// returns the fnum of a given file by its full pathname
extern unsigned int gcmGetFileByName(GCM *data, char *name);
// returns the actual entry of a file given its fnum
extern GCM_FSTEntry gcmGetFileByFnum(GCM *data, int fnum);
// explodes the FST onto the host filesystem in the current directory
extern int gcmExplode(GCM *data);
