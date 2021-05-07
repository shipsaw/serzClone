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
	char *sourceIter = source;	// Use original malloc pointer for free later

	long fileSize = getFileSize(binFile);

	if (checkPrelude(source) == -1) {
		printf("Incorrect Prelude\n");
		exit(1);
	}
	printf("Prelude correct\n");
	for (long i = 8; i < fileSize; i++) {
		printf("%c", sourceIter[i]);
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
