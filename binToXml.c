#include <stdlib.h>
#include <stdint.h>
#include "map.h"
#include "serz.h"

const char *prolog = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
const char *binPrelude = "SERZ\x00\x00\x01\x00";

// Global vars
char *symArray[65536];		// Two byte's worth of representation
int symArrayIdx = 0;
static FILE *outFile;
static char *source = NULL;	// bin file copied into this buffer
int tabPos = 0;

int binToXml(FILE *binFile, FILE *xmlFile) {
	outFile = xmlFile;

	source = readInfile(binFile);

	long fileSize = getFileSize(binFile);

	if (checkPrelude() == -1) {
		printf("Incorrect Prelude\n");
		exit(1);
	}

	fputs(prolog, outFile);
	for (long i = 8; i < fileSize; i++) { //TODO once all elements can be processed the i++ should not be neccessary
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


uint32_t conv32(long i) {
	//printf("%x, %x, %x, %x\n", source[i], source[i+1], source[i+2], source[i+3]);
	uint32_t x = ((uint8_t)source[i] | (uint8_t)source[i+1] << 8 | (uint8_t)source[i+2] << 16 | (uint8_t)source[i+3] << 24);
	return x;
}

uint16_t conv16(long i) {
	uint16_t x = ((uint8_t)source[i] | (uint8_t)source[i+1] << 8);
	return x;
}


// Called at first FF of two that indicates new symbol
long newSym(long i) {
	char symbol[20];
	long pos = i + 2;			// Advance past two FF bytes
	long x = conv32(pos);			// 4 bytes that represent symbol length in bytes
	char *nameElem = malloc(x + 1);		// Name string that will be stored in name array
	pos += 4;				// Advance past symbol length bytes
	long j;
	for (j = 0; j < x; j++) {		// Write letters to file
		nameElem[j] = source[pos+j];
	}
	nameElem[j+1] = '\0';
	fputs(nameElem, outFile);
	symArray[symArrayIdx] = nameElem;
	symArrayIdx++;
	
	pos += x;
	return pos;
}

// Called when i is at the byte after the 0x50
long process50(long i) {
	printf("50..");
	addTabs();
	tabPos++;
	fputc('<', outFile);
	if (source[i] == '\xFF') {
		i = newSym(i);
	} else {
		// Lookup element name
	}
	uint32_t id = conv32(i);
	i += 8; // Skip over rest of id and # of children bytes
	if (id == 0) {
		fputs(">\n", outFile);
	} else {
		fprintf(outFile, " d:id=\"%u\">\n", id);
	}
	return i;
	printf("end\n");
}

long process56(long i) {
	addTabs();
	fputc('<', outFile);
	if (source[i] == '\xFF') {
		i = newSym(i);
	} else {
		// Lookup element name
	}
	fprintf(outFile, " d:type=\"");
	if (source[i] == '\xFF') {
		i = newSym(i);
	} else {
		// Lookup property name
	}
	fputs("\">\n", outFile);
}

long process41(long i) {
	addTabs();
	fputc('<', outFile);
	if (source[i] == '\xFF') {
		newSym(i);
	} else {
		// Lookup element name
	}
	fputs(">\n", outFile);
}

long process70(long i) {
	tabPos--;
	addTabs();
	fputs("</", outFile);
	if (source[i] == '\xFF') {
		newSym(i);
	} else {
		int arrIdx = conv16(i);
		i += 2;
		fputs(symArray[arrIdx], outFile);
	}
	fputs(">\n", outFile);
}

void addTabs() {
	for (int i = 0; i < tabPos; i++) {
		fputc('\t', outFile);
	}
	return;
}
