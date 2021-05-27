#include <stdlib.h>
#include <stdint.h>
#include "map.h"
#include "serz.h"

const char *prolog = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
const char *binPrelude = "SERZ\x00\x00\x01\x00";

// Global vars
/*
char *symArray[65536];		// Two byte's worth of representation
uint16_t symArrayIdx = 0;
uint32_t elemArray[256];	// One byte's worth of representation, uint32 because two 16-bit symbol references
int linenum = 0;		// how element array is referenced
*/

//static FILE *outFile;
//static char *source = NULL;	// bin file copied into this buffer
//int tabPos = 0;



int binToXml(FILE *binFile, FILE *xmlFile) {
	fileStatus fs;
	symbolMaps sm;
	fs.outFile = xmlFile;
	fs.source = readInfile(binFile);


	if (checkPrelude(&fs) == -1) {
		printf("Incorrect Prelude\n");
		exit(1);
	}
	fputs(prolog, fs.outFile);

	long fileSize = getFileSize(binFile);
	for (long i = 8; i < fileSize; i++) { //TODO once all elements can be processed the i++ should not be neccessary
		switch (fs.source[i]) {
			case '\xFF':
				fs.i = i;
				processFF(&fs, &sm);
				break;
		}
	}
	printf("\n");
	free(fs.source);
	return 0;
}

// checkPrelude checks that the binary prelude that exists at the beginning of every bin is correct
int checkPrelude(fileStatus *fs) {
	for (int i = 0; i < 8; i++) {
		if (fs->source[i] != binPrelude[i]) {
			return -1;
		}
	}
	return 1;
}

// ProcessFF is called at the first of the two \xFF's that define the element type
void processFF(fileStatus *fs, symbolMaps *sm) {
	fs->i += 2;
	switch (fs->source[fs->i-1]) {
		case '\x50':
			process50(fs, sm);
			break; 
		case '\x56': 
			process56(fs, sm);
			break;
		case '\x41':
			process41(fs, sm);
			break;
		case '\x70':
			process70(fs, sm);
			break;
	}
	sm->linenum++;
	return;
}

// Called at first FF of two that indicates new symbol
uint16_t newSym(fileStatus *fs, symbolMaps *sm) {
	char symbol[20];
	fs->i += 2;				// Advance past two FF bytes
	long x = conv32(fs->source[fs->i]);			// 4 bytes that represent symbol length in bytes
	char *nameElem = malloc(x + 1);		// Name string that will be stored in name array
	fs->i += 4;				// Advance past symbol length bytes
	long j;
	for (j = 0; j < x; j++) {		// Write letters to file
		nameElem[j] = fs->source[fs->i+j];
	}
	nameElem[j+1] = '\0';
	fputs(nameElem, fs->outFile);
	sm->symArray[sm->symArrayIdx] = nameElem;
	sm->symArrayIdx++;
	
	fs->i += x;
	return sm->symArrayIdx;
}

// newElem takes the 16-bit symbol array values for the name and attribute, concatinating them into a 32-bit
// value that can be stored to save the element signiture 
uint32_t newElem(uint16_t nameSym, uint16_t attrSym) {
	uint32_t elemKey = nameSym << 8 | attrSym;
	printf("ElemKey = %04X\n", elemKey);
	return elemKey;
	}

// process50 is called when i is at the byte after the 0x50, and handles writing an FF50 element to the output file
void process50(fileStatus *fs, symbolMaps *sm) {
	uint16_t nameSym, attrSym = 0;
	addTabs(fs);
	fs->tabPos++;
	fputc('<', fs->outFile);
	if (fs->source[fs->i] == '\xFF') {
		nameSym = newSym(fs, sm);
	} else {
		int arrIdx = conv16(fs->source[fs->i]);
		fs->i += 2;
		fputs(sm->symArray[arrIdx], fs->outFile);
	}
	uint32_t id = conv32(fs->source[fs->i]);
	fs->i += 8; // Skip over rest of id and # of children bytes
	if (id == 0) {
		fputs(">\n", fs->outFile);
	} else {
		fprintf(fs->outFile, " d:id=\"%u\">\n", id);
	}
	newElem(nameSym, attrSym);
	return;
}

void process56(fileStatus *fs, symbolMaps *sm) {
	addTabs(fs);
	uint16_t nameSym, attrSym;
	fputc('<', fs->outFile);
	if (fs->source[fs->i] == '\xFF') {
		nameSym = newSym(fs, sm);
	} else {
		int arrIdx = conv16(fs->source[fs->i]);
		fs->i += 2;
		fputs(sm->symArray[arrIdx], fs->outFile);
	}
	fprintf(fs->outFile, " d:type=\"");
	if (fs->source[fs->i] == '\xFF') {
		attrSym = newSym(fs, sm);
	} else {
		int arrIdx = conv16(fs->source[fs->i]);
		fs->i += 2;
		fputs(sm->symArray[arrIdx], fs->outFile);
	}
	newElem(nameSym, attrSym);
	fputs("\">\n", fs->outFile);
}

void process41(fileStatus *fs, symbolMaps *sm) {
	addTabs(fs);
	uint16_t nameSym, attrSym = 0;		// TODO get attribute symbol
	fputc('<', fs->outFile);
	if (fs->source[fs->i] == '\xFF') {
		nameSym = newSym(fs, sm);
	} else {
		int arrIdx = conv16(fs->source[fs->i]);
		fs->i += 2;
		fputs(sm->symArray[arrIdx], fs->outFile);
	}
	fputs(">\n", fs->outFile);
	newElem(nameSym, attrSym);
	return;
}

void process70(fileStatus *fs, symbolMaps *sm) {
	fs->tabPos--;
	uint16_t nameSym = 0, attrSym = 0;
	addTabs(fs);
	fputs("</", fs->outFile);
	if (fs->source[fs->i] == '\xFF') {
		nameSym = newSym(fs, sm);
	} else {
		int arrIdx = conv16(fs->source[fs->i]);
		fs->i += 2;
		fputs(sm->symArray[arrIdx], fs->outFile);
	}
	fputs(">\n", fs->outFile);
	newElem(nameSym, attrSym);
}

void addTabs(fileStatus *fs) {
	for (int i = 0; i < fs->tabPos; i++) {
		fputc('\t', fs->outFile);
	}
	return;
}
