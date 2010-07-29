#include "gcminfo.h"
#include <string.h>
#include <unistd.h>

// All the interesting stuff is in gcminfo.c|h ; gcmbrowser.cpp is just a proof
// and simple util.

const char *usage = 
"gcmbrowser V0.01 (C) 2005 Andrew Kesterson (andrew@aklabs.net) \n\
\n\
Usage: gcmbrowser <.gcm file> <operation>\n\
\n\
operations:\n\
    -h : print this help\n\
    -H : print the disk header (maker, title, etc)\n\
    -E : explode the filesystem into the current directory\n\
    -e <file> : extract \"file\" from the gcm. With -o you\n\
         may specify an optional output filename; by default,\n\
         the file will be named the same (and reside in the\n\
         same directory tree) as in the image. You may specify\n\
         the source file by file number or by filename (full path\n\
         is necessary when specifying by filename.)\n\
    -o <filename> : specify the destination for a -e operation.\n\
    -l : list the names and file numbers of all files in the GCM.\n\
         All files are printed with full path.\n\
    -d : when extracting a file, create all needed directories for\n\
         the output file. This can, but really shouldn't, be used with\n\
         the -o option.\n\
\n";

const char *shortopts = "hHEe:o:ld";

#define OP_PRINT_HEADER          0
#define OP_EXPLODE               2
#define OP_EXTRACT               4
#define OP_LIST                  8
#define OP_CREATE_DIRS           1024

int main(int argc, char **argv)
{
	std::string gcmFname;
	std::string outFname;
	std::string extractFname;
	unsigned int extractFnumber= -1;
	int outIsDigit = 1;
	int ctr = 0;
	int op = 0;
    GCM theGCM;
	unsigned int opt = 0;
	unsigned int ret = 0;

	if ( argc < 2 ) {
		fprintf(stderr, usage);
		return 1;
	}

	opt = getopt(argc, argv, shortopts);
	while ( opt != -1) {
		switch ( opt ){
		case 'h':
			fprintf(stderr, usage);
			return 1;
		case 'H':
			op = OP_PRINT_HEADER;
			break;
		case 'E':
			op = OP_EXPLODE;
			break;
		case 'e':
			op = OP_EXTRACT;
			for ( ctr = 0; ctr < strlen(optarg) ; ctr++ ) {
				if ( isprint(optarg[ctr]) && !isdigit(optarg[ctr])) {
					outIsDigit = 0;
				} else {
					//printf("DEBUG : extract filename character %c is a digit\n", optarg[ctr]);
					continue;
				}
			}
			if ( outIsDigit == 0 ) {
				//printf("DEBUG : extract filename option is NON-DIGIT %s\n", optarg);
				extractFname.reserve(strlen(optarg));
				extractFname.assign(optarg);
				break;
			}
			else {
				//printf("DEBUG : extract filename option is file number %s\n", optarg);
				extractFnumber = atoi(optarg);
				break;
			}
		case 'o':
			outFname.reserve(strlen(optarg));
			outFname.assign(optarg);
			break;
		case 'l':
			op = OP_LIST;
			break;
		case 'd':
			op |= OP_CREATE_DIRS;
			break;
		default:
			fprintf(stderr, "I don't understand option -%c\n", opt);
			return 1;
		}
		opt = getopt(argc, argv, shortopts);
	}
	gcmFname.reserve(strlen(argv[optind]));
	gcmFname.assign(argv[optind]);

    if ( gcmInit((char *)gcmFname.c_str(), &theGCM) ) {
        printf("Couldn't init GCM file %s\n", argv[1]);
        return 1;
    }

	int top = 0;
	if ( op & OP_CREATE_DIRS ) top = op ^ OP_CREATE_DIRS;
	else top = op;
	switch ( top ) {
	case OP_EXPLODE:
		ret = gcmExplode(&theGCM);
		if ( ! ret )
			fprintf(stderr, "Failed to explode filesystem.\n");		
		break;
	case OP_EXTRACT:
		if ( extractFnumber == -1 ) {
			//printf("DEBUG : getting file number for %s\n", extractFname.c_str());
			extractFnumber = gcmGetFileByName(&theGCM, (char *)extractFname.c_str());
			//printf("DEBUG: got %d\n", extractFnumber);
		}
		if ( extractFnumber > theGCM.fst.stringtable.size() || extractFnumber == -1) {
			fprintf(stderr, "File number %d requested was beyond the length of the filesystem\n", extractFnumber);
			break;
		}
		if ( outFname.length() == 0 && ( op & OP_CREATE_DIRS ) ) {
			char *outfile = (char *) new char[GCNDVD_FNAME_LENGTH];
			gcmGetFullFileName(&theGCM, extractFnumber, outfile);
			std::string execstring = "";		
			// this is a quick hack to ensure that the output directory exists
			execstring.append("mkdir -p .");
			execstring.append("`dirname ");
			execstring.append(outfile);
			execstring.append("`");
			system(execstring.c_str());
			delete outfile;
		}
		if ( outFname.length() > 0 )
			ret = gcmExtractFile(&theGCM, extractFnumber, (char *)outFname.c_str());
		else
			ret = gcmExtractFile(&theGCM, extractFnumber, NULL);
		if ( ret != 0 ) 
			fprintf(stderr, "Failed to extract file number %d to output file %s\n", extractFnumber, outFname.length() > 0 ? (char *)outFname.c_str() : "" );
		break;
	case OP_PRINT_HEADER:
		ret = gcmPrint(&theGCM);
		break;
	case OP_LIST:
		ret = gcmPrintFST(&theGCM);
		break;
	}
		
    gcmFreeStringTable(&theGCM);

    return ret;
}
