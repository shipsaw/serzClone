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

	printf("Converting %s to %s...\n", argv[1], outfileName);
	printf("Filetype of input: %c\n", *fileType(argv[1]));

	// TODO: if-statement to determine which way this converison is going
	xmlToBin(infile, outfile);
	printf("Success\n");
	return 0;
}

