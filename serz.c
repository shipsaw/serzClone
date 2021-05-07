#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"serz.h"

int main (int argc, char** argv) {

	// memory and file variables
	FILE *outfile, *infile; 
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

	char outfileName[strlen(argv[1])];
	strcpy(outfileName, argv[1]);
	swapFilename(outfileName);

	// Open output file
	if ((outfile = fopen(outfileName, "wb")) == NULL) {
		printf("\nError opening file\n");
		exit(1);
	}

	if (*fileType(argv[1]) == 'x') {			// xml to bin or bin to xml?
		printf("Converting xml to bin\n");
		xmlToBin(infile, outfile);
	} else {
		printf("Converting bin to xml\n");
		binToXml(infile, outfile);
	}

	printf("Success\n");
	return 0;
}

