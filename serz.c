#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"serz.h"

int main (int argc, char** argv) {

	// memory and file variables
	FILE *outFile, *infile; 
	void *buf = malloc(BUFSIZE);
	char *source = NULL;

	// open source file
	if (argc <= 1 || argc > 3) {
		printf("Usage: serz infile /{bin,xml}:outfile\nOr: serz infile\n");
		exit(1);
	} else {
		if ((infile = fopen(argv[1], "r")) == NULL) {
			printf("\nError opening file\n");
			exit(1);
		}
	}

	char outFileName[strlen(argv[1])];
	strcpy(outFileName, argv[1]);
	swapFilename(outFileName);

	// Open output file
	if ((outFile = fopen(outFileName, "wb")) == NULL) {
		printf("\nError opening file\n");
		exit(1);
	}

	printf("Converting %s to %s...\n", argv[1], outFileName);
	source = readInfile(infile);
	char *sourceIter = source;	// Makes sure we save the original source pointer for freeing

	// TODO: if-statement to determine which way this converison is going
	printf("Success\n");
	return 0;
}

// Given a pointer, does proper allocation and reads the xml file into the memory
char* readInfile(FILE* infile) {
	char* buf;
	// Read xml bytes
	if (fseek(infile, 0L, SEEK_END) == 0) {
		/* Get the size of the file. */
		long bufsize = ftell(infile);
		if (bufsize == -1) { /* Error */ }

		/* Allocate our buffer to that size. */
		buf = malloc(sizeof(char) * (bufsize + 1));

		/* Go back to the start of the file. */
		if (fseek(infile, 0L, SEEK_SET) != 0) { /* Error */ }

		/* Read the entire file into memory. */
		size_t newLen = fread(buf, sizeof(char), bufsize, infile);
		if ( ferror( infile ) != 0 ) {
			fputs("Error reading file", stderr);
		} else {
			buf[newLen++] = '\0'; /* Just to be safe. */
		}
	}
	return buf;
}

// fileType takes a string and determines if it is a .bin or a .xml.  It returns the first character of the filetype extension
char* fileType(char *filename) {
	size_t nameLen = strlen(filename);
	char *ext;
	static const char *bin = "bin";
	static const char *xml = "xml";

	for (int i = 0; i < nameLen; i++) {
		if (filename[i] == '.') {
			ext = &filename[i + 1];
		}
	}

	if (strncmp(ext, bin, 3) == 0) {
		return ext;
	} else if (strncmp(ext, xml, 3) == 0) {
		return ext;
	} else {
		return NULL;
	}
}

// swapFilename takes a filename string, and if .xml changes the extension to .bin and vice-versa
void swapFilename(char *filename) {
	char *ext = fileType(filename);
	if (ext == NULL) {
		printf("Error, unknown filetype\n");
		exit(1);
	}

	static const char *bin = "bin";
	static const char *xml = "xml";

	switch (*ext) {
		case 'b':
			for (int i = 0; i < 3; i++) {
				ext[i] = xml[i];
			}
			break;
		case 'x':
			for (int i = 0; i < 3; i++) {
				ext[i] = bin[i];
			}
			break;
	}
	return;
}