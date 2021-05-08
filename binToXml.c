#include <stdlib.h>
#include <stdint.h>
#include "map.h"
#include "serz.h"

const char *prolog = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
const char *binPrelude = "SERZ\x00\x00\x01\x00";

// Global vars
map_str_t symSmap;	// symbol map
map_str_t elemSmap;	// element map
static FILE *outFile;
static char *source = NULL;	// bin file copied into this buffer

int binToXml(FILE *binFile, FILE *xmlFile) {
	outFile = xmlFile;

	map_init(&symSmap);
	map_init(&elemSmap);

	source = readInfile(binFile);

	long fileSize = getFileSize(binFile);

	if (checkPrelude() == -1) {
		printf("Incorrect Prelude\n");
		exit(1);
	}

	for (long i = 8; i < fileSize; i++) {
		switch (source[i]) {
			case '\xFF':
				processFF(i);
				break;
		}
	}
	printf("\n");
	free(source);
	return 0;
}

// checkPrelude checks that the binary prelude that exists at the beginning of every bin is correct
int checkPrelude() {
	for (int i = 0; i < 8; i++) {
		if (source[i] != binPrelude[i]) {
			return -1;
		}
	}
	return 1;
}

void processFF(long i) {
	switch (source[i+1]) {
		case '\x50':
			i = process50(i+2);
			break; 
		case '\x56': 
			i = process56(i+2);
			break;
		case '\x41':
			i = process41(i+2);
			break;
		case '\x70':
			i = process70(i+2);
			break;
	}
	return;
}

// Called when i is at the byte after the 0x50
long process50(long i) {
	fputc('<', outFile);
	if (source[i] == '\xFF') {
		newElemName(i+2);
	} else {
		// Lookup element name
	}
	fputs(">\n", outFile);
}

uint32_t conv32(long i) {
	printf("%x, %x, %x, %x\n", source[i], source[i+1], source[i+2], source[i+3]);
	uint32_t x = (source[i] | source[i+1] << 8 | source[i+2] << 16 | source[i+3] << 24);
	return x;
}

long newElemName(long i) {
	long x = conv32(i);
	i += 4;
	for (long j = 0; j < x; j++) {
		fputc(source[i+j], outFile);
	}
	i += x;
	return i;
}
long process56(long i) {
	fputc('<', outFile);
	if (source[i] == '\xFF') {
		newElemName(i+2);
	} else {
		// Lookup element name
	}
	fputs(">\n", outFile);
}
long process41(long i) {
	fputc('<', outFile);
	if (source[i] == '\xFF') {
		newElemName(i+2);
	} else {
		// Lookup element name
	}
	fputs(">\n", outFile);
}
long process70(long i) {
	fputc('<', outFile);
	if (source[i] == '\xFF') {
		newElemName(i+2);
	} else {
		// Lookup element name
	}
	fputs(">\n", outFile);
}
