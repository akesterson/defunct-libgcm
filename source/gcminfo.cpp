#include "gcminfo.h"
#include <unistd.h>
#include <vector>
#include <string>

int needByteSwap = 0;

// these are functions that the user really doesn't need to see,
// so they're declared here away from the external API. They're
// used mainly for populating the internal FST data structure.
// All the user-necessary functions are listed out in gcminfo.h.

int gcmPopulateFST(GCM *data, FILE *file);
void *gcmWhichData(int whichdata, GCM *data);
int gcmGetData(int whichdata, GCM *data, FILE *file);
int gcmGetFile(GCM *data, GCM_FSTEntry *entry, int fnum, FILE *file);
int gcmCheckMagic(FILE *file);

// checks the magic number of the DVD file, and sets needByteSwap
// returns the final value of needByteSwap. Returns -1 if the magic
// word doesn't match either way and needByteSwap couldn't be set.
int gcmCheckMagic(FILE *file)
{
    int magic = 0;
    fseek(file, GCMPieces[GCM_DHEAD_DVDMAGIC].offset, 0);
    fread(&magic, GCMPieces[GCM_DHEAD_DVDMAGIC].size, 1, file);
    
    if ( magic == GCNDVD_MAGICNUM ) {
        needByteSwap = 0;
    } else {
        magic = (((magic&0x000000FF)<<24)+((magic&0x0000FF00)<<8)+
                               ((magic&0x00FF0000)>>8)+((magic&0xFF000000)>>24));
        if ( magic == GCNDVD_MAGICNUM ) {
            needByteSwap = 1;
        }
        else needByteSwap = -1;
    }

    return needByteSwap;
}

// swap from big to little endian (only needed on intel machines)
// only works if needByteSwap is > -1
int byteSwap (int nLongNumber)
{
   if ( needByteSwap ) return (((nLongNumber&0x000000FF)<<24)+((nLongNumber&0x0000FF00)<<8)+
                               ((nLongNumber&0x00FF0000)>>8)+((nLongNumber&0xFF000000)>>24));
   else if ( needByteSwap == -1 ) return 0; // refuse to work
}

// return the type (0=file 1=directory) for the given FST entry
int gcmGetFSTEntryType(GCM_FSTEntry *entry)
{
    return entry->fname_offset >> 24;
}

// uses one of the GCM_* definitions to return a pointer to
// the appropriate point of the GCM struct to read data into.
void *gcmWhichData(int whichdata, GCM *data) 
{
    void *toPrint = NULL;
                    
    switch (whichdata) {
    case GCM_DHEAD_GAMECODE:
        toPrint = &data->head.gamecode;
        break;
    case GCM_DHEAD_GAMEMAKER:
        toPrint = &data->head.gamemaker;
        break;
    case GCM_DHEAD_DVDMAGIC:
        toPrint = &data->head.magicword;
        break;
    case GCM_DHEAD_GAMENAME:
        toPrint = &data->head.gamename;
        break;
    case GCM_DHEAD_DEBUGMON:
        toPrint = &data->head.debugmon;
        break;
    case GCM_DHEAD_DEBUGMONADDR:
        toPrint = &data->head.debugmonaddr;
        break;
    case GCM_DHEAD_BOOTFILE:
        toPrint = &data->head.bootfile;
        break;
    case GCM_DHEAD_FST:
        toPrint = &data->fst.offset;
        break;
    case GCM_DHEAD_FSTSIZE:
        toPrint = &data->fst.size;
        break;
    case GCM_DHEAD_FSTMAXSIZE:
        toPrint = &data->fst.maxsize;
        break;
    case GCM_DHEAD_USERPOS:
        toPrint = &data->head.userpos;
        break;
    case GCM_DHEAD_USERLEN:
        toPrint = &data->head.userlen;
        break;
    case GCM_DHEADINF_DEBUGMONSIZE:
        toPrint = &data->headInf.debugmonsize;
        break;
    case GCM_DHEADINF_SIMMEMSIZE:
        toPrint = &data->headInf.simmemsize;
        break;
    case GCM_DHEADINF_ARGOFFSET:
        toPrint = &data->headInf.simmemsize;
        break;
    case GCM_DHEADINF_DEBUGFLAG:
        toPrint = &data->headInf.debugflag;
        break;
    case GCM_DHEADINF_TRACKLOCATION:
        toPrint = &data->headInf.tracklocation;
        break;
    case GCM_DHEADINF_TRACKSIZE:
        toPrint = &data->headInf.tracksize;
        break;
    case GCM_DHEADINF_COUNTRYCODE:
        toPrint = &data->headInf.countrycode;
        break;
    case GCM_APPLOADER_VERSION:
        toPrint = &data->apploader.datever;
        break;
    case GCM_APPLOADER_ENTRY:
        toPrint = &data->apploader.entry;
        break;
    case GCM_APPLOADER_SIZE:
        toPrint = &data->apploader.size;
        break;
    case GCM_APPLOADER_TRAILERSIZE:
        toPrint = &data->apploader.trailersize;
        break;
    case GCM_APPLOADER_BINARY:
        toPrint = &data->apploader.apploader;
        break;
    case GCM_FST_ROOT:
        toPrint = (void *)"Not implemented yet";
        break;
    case GCM_FST_STRINGTABLEOFF:
        toPrint = &data->fst.stringtable_offset;
        break;
    default:
        return NULL;
    }

    return toPrint;
}

// print the info in data as indicated by the GCM_*
// passed to whichdata
int gcmPrintData(int whichdata, GCM *data)
{
    void *ptr;
    if ( GCMPieces[whichdata].type == 1) {
        char *theData = (char *)gcmWhichData(whichdata,data);
        printf("%-38s:\t %s\n", GCMPieces[whichdata].name, theData);
    } 
    else {
        int *theData = (int *)gcmWhichData(whichdata,data);
        if ( theData ) {
            printf("%-38s:\t %x\n", GCMPieces[whichdata].name, *theData);
        }
        else {
            printf("%-38s:\t refusing to print NULL pointer...\n",
                   "(untested function/invalid data)"); // this means you're using a feature that doesn't work (yet)
        }
    }
}

// print the information for a given file in data (by entry num)
int gcmPrintFile(GCM *data, int fnum)
{
    GCM_FSTEntry f;
	char *fname = NULL;
    
	//	printf("DEBUG : retrieving file data for number %d\n", fnum);
    f = data->fst.entries.at(fnum);

    // see gcmPrint for how this is laid out...
	if ( fnum >= data->fst.entries.size() )
		fname = "(out_of_bounds string)" ;
	else
		fname = (char *)data->fst.stringtable.at(fnum)->c_str();

    printf("%-20s%-6d%-6x%-12x%-20d\n", 
           fname,
           fnum,
           gcmGetFSTEntryType(&f),
           f.file_offset,
           f.file_length);
}

// reads data into dest and returns the number of bytes read
// (only used internally by the other gcm functions, this should
// be transparent outside the API guts)
int gcmGetData(int whichdata, GCM *data, FILE *file)
{
    int offset = 0;
    void *dest = gcmWhichData(whichdata, data);
    if ( whichdata > GCM_MAX_PIECES || dest == NULL || file == NULL) {
        return 0;
    }

    switch ( GCMPieces[whichdata].area ) {
    case 0:
        offset = GCMPieces[whichdata].offset;
        break;
    case 1:
        offset = GCMPieces[GCM_DHEAD].offset + GCMPieces[whichdata].offset;
        break;
    case 2:
        offset = GCMPieces[GCM_DHEADINF].offset + GCMPieces[whichdata].offset;
        break;
    case 3:
        offset = GCMPieces[GCM_APPLOADER].offset + GCMPieces[whichdata].offset;
        break;
    case 4:
        offset = data->fst.offset + GCMPieces[whichdata].offset;
    default:
        return 0;
    }
    
    fseek(file, offset, 0);
    if ( GCMPieces[whichdata].type == 1 ) {
        fgets((char *)dest, GCMPieces[whichdata].size, file) == NULL;
    } 
    else {
        fread(dest, 1, GCMPieces[whichdata].size, file);
        int *tmp = (int *)dest;
        *tmp = byteSwap(*tmp);
    }
    rewind(file);
    return GCMPieces[whichdata].size;
}

// get the file at fnum from file for the GCM data and store it
// in entry
int gcmGetFile(GCM *data, GCM_FSTEntry *entry, int fnum, FILE *file)
{
	if ( fnum > -1 && file != NULL ) {
        rewind(file);
        fseek(file, data->fst.offset + (12 * fnum), 0);
        
        if ( !feof(file) ) {
            fread(&entry->fname_offset, 4, 1, file);
            entry->fname_offset = byteSwap(entry->fname_offset);
            fread(&entry->file_offset, 4, 1, file);
            entry->file_offset = byteSwap(entry->file_offset);
            fread(&entry->file_length, 4, 1, file);
            entry->file_length = byteSwap(entry->file_length);
            data->fst.entries.push_back(*entry);
        }
    }
    else return -1;
}

// populate the FST of the given GCM from file
int gcmPopulateFST(GCM *data, FILE *file)
{
    GCM_FSTEntry *newEntry;
    GCM_FSTEntry root;
    int i = 0;
    int j = 0;
    int oldpos = 0;
    int maxentries = 0;
	char *tmpbuff = new char[GCNDVD_FNAME_LENGTH];
    std::string * strdest = new std::string("");
    char tmp = 0xFF;
    int f = 0;
	int *appSize = NULL;
    
    if ( !tmpbuff ) {
        return -1;
    }

    fseek(file, data->fst.offset, 0);
    
    // now just loop until there are no more entries...
    gcmGetFile(data, &root, 0,file);
    root = data->fst.entries.front();
	// this is where we inject the filesystem entries for bootloader.bin, apploader.bin,
	// and game.dol ... these aren't technically part of the filesystem, but we
	// do this so they can be viewed/extracted like other files.
	/*
	appSize = (int *)gcmWhichData(GCM_APPLOADER_SIZE, data);
	newEntry = new GCM_FSTEntry;
	newEntry->fname_offset = 0;
	newEntry->file_offset = GCM_APPLOADER_CODE_LOC;
	newEntry->file_length = *appSize;
	newEntry->stringtable_off = 1;
	data->fst.entries.push_back(*newEntry);
	delete newEntry;*/
    for ( int i =  1; i < root.file_length ; i++) {
		newEntry = new GCM_FSTEntry;
        gcmGetFile(data, newEntry, i, file);
		delete newEntry;
    }

	
    // we're past the FST proper; now the string table... 
    data->fst.stringtable_offset = data->fst.offset + (root.file_length * 12);
    fseek(file, data->fst.stringtable_offset, 0);
    //printf("FST string table offset calculated to 0x\%x...\n", data->fst.stringtable_offset);
	data->fst.stringtable.clear();
    data->fst.stringtable.push_back(&std::string("/\0"));
	//data->fst.stringtable.push_back(&std::string("apploader.bin\0"));
    memset(tmpbuff, GCNDVD_FNAME_LENGTH, 0x00);
    while ( ftell(file) < (data->fst.offset + data->fst.size) && !feof(file)) {
        i = 0;
        while ( 1 ) {
            tmp = fgetc(file);
            if ( tmp == 0x00 ) {
                break;
            }
            tmpbuff[i] = tmp;
            i++;
        }
		tmpbuff[i] = '\0';
		strdest->clear();
		strdest->reserve(i);
		strdest->assign(tmpbuff);
		data->fst.stringtable.push_back(strdest);
		//		printf("DEBUG : FILENAME WAS %s ... added %s\n", strdest, 
		//	   data->fst.stringtable.at(f+1)->c_str());
        memset(tmpbuff, GCNDVD_FNAME_LENGTH, 0x00);
		strdest = new std::string("");
        f++;
    }
	delete tmpbuff;
	return 0;
}

// called inside of gcmInit to make sure everything inside is sane
// (only needed internally)
int gcmZeroOut(GCM *data)
{
    data->apploader.entry = 0;
    data->apploader.size = 0;
    data->apploader.trailersize = 0;
    data->fst.maxsize = 0;
    data->fst.offset = 0;
    data->fst.size = 0;
    data->head.bootfile = 0;
    data->head.debugmon = 0;
    data->head.debugmonaddr = 0;
    data->head.gamecode = 0;
    data->head.gamemaker = 0;
    data->head.magicword = 0;
    data->head.userlen = 0;
    data->head.userpos = 0;
    data->headInf.argoff = 0;
    data->headInf.countrycode = 0;
    data->headInf.debugflag = 0;
    data->headInf.debugmonsize = 0;
    data->headInf.simmemsize = 0;
    data->headInf.tracklocation = 0;
    data->headInf.tracksize = 0;
}

// reads all info from the GCM in gcmname and puts it in data
int gcmInit(char *gcmname, GCM *data)
{
    FILE *gcm = fopen(gcmname, "r+b");
    int i = 0;

    if (!gcm) {
        return -1;
    }

    gcmZeroOut(data);

    switch ( gcmCheckMagic(gcm) ) {
    case 0:
        needByteSwap = 1;
        break;
    case 1:
        break;
    case -1:
        //printf("DEBUG: DVD Magic number on GCM image was incorrect or couldn't be understood!\n");
        //printf("DEBUG: Aborting in GCMInit! (filename %s)\n", gcmname);
        fclose(gcm);
        return -1;
    }

    for ( i = 3; i < GCM_MAX_PIECES ; i++) {
        gcmGetData(i,data, gcm);
    }

    gcmPopulateFST(data, gcm);

    fclose(gcm);

	data->fname = new char[strlen(gcmname)+1];
	if ( !data->fname ) {
		printf("couldn't store GCM filename for later retrieval. Aborting.\n");
		gcmFreeStringTable(data);
		exit(-1);
	}
	else strcpy(data->fname, gcmname);

    return 0;
}

// print all information about the GCM in a columnar format,
// including the FST.
int gcmPrint(GCM *data)
{
    int ml = 0;
    int i = 0;

    for (i = 3; i < GCM_MAX_PIECES; i++ ) {
        gcmPrintData(i,data);
    }
    
    GCM_FSTEntry root = data->fst.entries.front();

    printf("%-38s:\t%d\n", "Total files in the FST", root.file_length);
    printf("%-38s:\t%d\n", "Total entries in the FST string table", data->fst.stringtable.size());
	return 0;
}

int gcmPrintFST(GCM *data) 
{
    int ml = 0;
    int i = 0;
	char *tbuff = new char[GCNDVD_FNAME_LENGTH];
    if (!tbuff) {
        printf("Couldn't get temporary buffer for full path printing. Aborting.\n");
        return -1;
    } else {
		memset(tbuff, 0x00, GCNDVD_FNAME_LENGTH);
	}

    GCM_FSTEntry root = data->fst.entries.front();
    printf("%-38s:\t%d\n", "Total files in the FST", root.file_length);
	printf("%-38s:\t%d\n", "Total entries in the FST string table", data->fst.stringtable.size());
    printf("\n\n\n%30s\n", "(FST CONTENTS)");
    printf("%-16s %-6s %-6s %s %-12s\n",
           "(name)", "(fnum)", "(type)", "(offset)", "(length)");
    for (i = 0; i < data->fst.entries.size() - 1 ; i++) {
		//printf("DEBUG: retrieving file data for number %d\n", i);
        gcmPrintFile(data, i);
    }

    printf("\n\t\t\t(PRINTING FILES WITH FULL PATH)\n");
    for (i = 0; i < data->fst.entries.size() - 1; i++ ) {
		printf("File %d:", i);
		gcmGetFullFileName(data, i, tbuff);
        printf(" %s\n", tbuff);
        memset(tbuff, 0x00, GCNDVD_FNAME_LENGTH);
    }

	delete tbuff;
    return 0;
}

// frees memory used by the string table. MUST be called before zeroing
// the gcm for new reading (gcmInit) or quitting the application.
// This should be accompanied by clearing the entries vector, though the
// destructor in that member should take care of cleanup for you.
// Thou shalt not leak memory!

int gcmFreeStringTable(GCM *data)
{
    std::string *string = NULL;
    int i = 0;

    //printf("freeing memory used by FST string table... ");
    
    for (i = 0; i < (data->fst.stringtable.size()) ; i++ ) {
        string = (std::string *)data->fst.stringtable.at(i);
		// we create the "/" entry for 0 manually, and it's not alloc'ed
		// so don't try freeing it!
        if ( string != NULL && i > 0 ) {
            delete string;
        }
		string = NULL;
    }
    data->fst.stringtable.clear();
	delete data->fname;

    //printf(" ...done.\n");
}

GCM_FSTEntry gcmGetFileByFnum(GCM *data, int fnum)
{
	return data->fst.entries.at(fnum);
}
 
int gcmGetFileByOffset(GCM *data, int offset)
{
    GCM_FSTEntry curr;
	bool done = false;

	for (int i = 0; i < data->fst.entries.size() -1; i++) {
		curr = data->fst.entries.at(i);
		if (curr.file_offset == offset) {
			return i;
		}
	}
}

// takes in a full pathname and retrieves the fnum
// for the appropriate file in the FST
unsigned int gcmGetFileByName(GCM *data, char *name)
{
    GCM_FSTEntry curr;
	bool done = false;
    int nextStart = 0;
	int i = 0;
	char *tmp2 = (char *) new char[GCNDVD_FNAME_LENGTH];
	if ( !tmp2 ) {
		fprintf(stderr, "Failed to allocate temporary string storage\n");
		return -1;
	}
	for ( i = 0; i < data->fst.stringtable.size() ; i++ ) {
		gcmGetFullFileName(data, i, tmp2);
		//printf("DEBUG : comparing filename %s to original %s...\n", name, tmp2);
		if ( !strcmp(name, tmp2) ) {
			delete tmp2;
			//printf("DEBUG : found filename %s at number %d", name, i);
			return i;
		}
	}
	delete tmp2;
	return -1;
}

// extracts a file into outfile from the GCM given the fnum.
// FIXME : allow the user to specify a different output filename?
unsigned int gcmExtractFile(GCM *data, int fnum, char *outfname)
{
    FILE *out = NULL;
	FILE *gcm = NULL;
	int pos = 0;
	int endpos = 0;
	int b = 0;
	GCM_FSTEntry curr;
	std::string execstring = "";
    char *outfile = NULL;
	std::string tmpStr = "";
    
	// check that the fnum is in range
    
	if ( fnum < data->fst.entries.size() - 1) {
		curr = data->fst.entries.at(fnum);
	}
	else {
		return -1;
	}

    // if it's a file, open the outfile, open the GCM, and perform the copy - clean up and exit
    if ( gcmGetFSTEntryType(&curr) == 0 ) {
		if ( outfname ) {
			outfile = outfname;
			tmpStr = "";
		} else {
			outfile = new char[GCNDVD_FNAME_LENGTH];
			if ( !outfile ) return -1;
			else gcmGetFullFileName(data, fnum, outfile);
			tmpStr = ".";
		}
		tmpStr.append(outfile);
		gcm = fopen(data->fname, "rb");
		out = fopen(tmpStr.c_str(), "wb");
		if ( !gcm || !out ) {
			gcm ? fclose(gcm) : printf("Couldn't read from gcm file\n");
			out ? fclose(out) : printf("Couldn't write to output file\n");
			return -1;
		}
		fseek(gcm, curr.file_offset, 0);
		endpos = curr.file_offset + curr.file_length;
		if ( ftell(gcm) != curr.file_offset || !out || !gcm) {
			delete outfile;
			fclose(gcm);
			fclose(out);
			return -1;
		}

		while ( !feof(gcm) && ftell(gcm) <= endpos) {
			fread(&b, 1, 1, gcm);
			if ( needByteSwap == 1 ) b = byteSwap(b); // swap the bytes for the host
			fwrite(&b, 1, 1, out);
		}

		if ( outfile && !outfname )delete outfile;
		fclose(gcm);
		fclose(out);
		printf("exploded %s\n", tmpStr.c_str());
		return 0;
	}
	// if it's a dir, construct the mkdir command, do it, clean up and exit.
	else {
		if ( outfname ) 
			outfile = outfname;
		else {
			outfile = new char[GCNDVD_FNAME_LENGTH];
			if (!outfile ) {
				return -1;
			}
		}
		gcmGetFullFileName(data, fnum, outfile);
		if ( !outfname ) 
			execstring.append("mkdir -p .");
		else 
			execstring.append("mkdir -p ");
		execstring.append(outfile);
		int retcode =  system(execstring.c_str());
		if ( outfile && !outfname ) delete outfile;
		return 0;
	}
}


// explodes the FST onto the host filesystem
int gcmExplode(GCM *data)
{
	for ( int i = 0; i <= (data->fst.entries.size() - 1) ; i++ ) {
		if ( gcmExtractFile(data, i, NULL) < 0 ) return -1;
	}

}

// retrieves the full filename for fnum with full path
int gcmGetFullFileName(GCM *data, int fnum, char *dest)
{
	bool finished = false;
    GCM_FSTEntry curr;
	std::vector<int> fnames;
	int fn = fnum;
	int depth = 0;
	std::string work = "";

	curr = data->fst.entries.at(fn);

	while (!finished) {
		depth++;
		if ( depth > data->fst.entries.size() - 1) {
			printf("went too far. aborting.\n");
			exit(-1);
		}
        if ( fn == 0 ) {
			fnames.push_back(0);
			finished = true;
			break;
		}
		if ( gcmGetFSTEntryType(&curr) == 0 ) {
			// file
			fnames.push_back(fn);
			while (gcmGetFSTEntryType(&curr) == 0) {
				// search up for the parent directory....
				curr = data->fst.entries.at(fn - 1);
				fn -= 1;
			}
		}
		else if ( gcmGetFSTEntryType(&curr) == 1) {
			// directory
			fnames.push_back(fn);
			fn = curr.file_offset;
			curr = data->fst.entries.at(fn);
		}
	}
	
	if ( fnum != 0 ) {
		for (int i = fnames.size() -1 ; i >= 0; i--) {		
			if ( i < fnames.size()-1 ) {
				work.append("/");
			} 
			if ( fnames.at(i) != 0 ) work.append(data->fst.stringtable.at(fnames.at(i))->c_str());
		}
	} else work = "/";
	strcpy(dest, work.c_str());
	return 0;
}
