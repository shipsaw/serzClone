#include <stdlib.h>
#include "map.h"
#include "serz.h"

const char *prolog = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
const char *binPrelude = "SERZ\x00\x00\x01\x00";

// Global vars
map_str_t symSmap;	// symbol map
map_str_t elemSmap;	// element map
static FILE *outFile;

int binToXml(FILE *binFile, FILE *xmlFile) {
	outFile = xmlFile;

	map_init(&symSmap);
	map_init(&elemSmap);

	char *source = NULL;
	source = readInfile(binFile);
	char *sourceIter = source;	// Use original malloc pointer for free later TODO Necessary?

	long fileSize = getFileSize(binFile);

	if (checkPrelude(source) == -1) {
		printf("Incorrect Prelude\n");
		exit(1);
	}

	for (long i = 8; i < fileSize; i++) {
		switch (source[i]) {
			case '\xFF':
				processFF(source, i);
				break;
		}
	}
	printf("\n");
	return 0;
}

// checkPrelude checks that the binary prelude that exists at the beginning of every bin is correct
int checkPrelude(char* source) {
	for (int i = 0; i < 8; i++) {
		if (source[i] != binPrelude[i]) {
			return -1;
		}
	}
	return 1;
}

void processFF(char *source, long i) {
	switch (source[i+1]) {
		case '\x50':
			printf("FF50\n");
			break;
		case '\x56':
			printf("FF56\n");
			break;
		case '\x41':
			printf("FF41\n");
			break;
		case '\x70':
			printf("FF70\n");
			break;
	}
	return;
}
