#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "yxml.h"
#include "map.h"
#include <stdbool.h>

#define BUFSIZE 4096

enum elType {NotSet, FF50, FF56, FF41, FF70};

typedef struct {
	uint32_t children; // uint32 because that's the format it is written in the bin file
	uint32_t linenum;  // uint32 because that's the type of the yxml_t.line attribute
	long childrenAdd;  // long based on the type of bufsize
} record;

typedef struct {
	record records[30];
	int index;
} recordArr;

char* readXML(char*);
void WritePrelude();
void WriteElemStart(yxml_t*, enum elType, size_t, char*);
void checkMap(char *symbol, size_t length);

// Global vars
map_int_t symMap; // symbol map
FILE *outFile, *xmlDoc; // binary file
recordArr records;

int main () {
	// memory and file variables
	void *buf = malloc(BUFSIZE);
	char *source = NULL;
	// yxml variables
	yxml_t x;
	yxml_init(&x, buf, BUFSIZE);
	// map variables
	map_init(&symMap);
	// user-defined types
	enum elType elemType;
	records.index = 0;

	printf("Opening file..\n");
	// open xml file
	if ((xmlDoc = fopen("oldTest.xml", "r")) == NULL) {
		printf("\nError opening file\n");
		exit(1);
	}
	// Open bin file
	if ((outFile = fopen("oldTest.bin", "wb")) == NULL) {
		printf("\nError opening file\n");
		exit(1);
	}
	source = readXML(source);
	WritePrelude(outFile);

	// Initialize record and value-keeping variables
	int lastType;
	char attrVal[40];
	char *attrValIndex = attrVal;
	char *sourceIter = source;
	char ff41ElemCt;
	elemType = NotSet;
	size_t nameLen;

	// Parse xml
	for (; *sourceIter; sourceIter++) {
		yxml_ret_t r = yxml_parse(&x, *sourceIter);
		if (r < 0)
			exit(1);
		if (r != 0)
			switch (r) {
				case YXML_ELEMSTART:
					nameLen = yxml_symlen(&x, x.elem); // Uses the fast yxml function to get length rather than slower strlen
					break;
				case YXML_ATTRVAL:
					*(attrValIndex++) = *x.data;
					break;
				case YXML_ATTREND:
					*(attrValIndex) = '\0';
					switch (x.attr[2]) {
						case 'i': // If matches d:id
							elemType = FF50;
							break;
						case 't': // If matches d:type
							elemType = FF56;
							break;
						case 'n': // If matches numElements
							// Take this character and save it to attach to the other ff41 attribute
							ff41ElemCt = *(attrValIndex - 1);
							break;
						case 'e': // If matches d:elementType
							elemType = FF41;
							*(attrValIndex++) = ff41ElemCt;
							*(attrValIndex) = '\0';
							break;
					}
					if (elemType != NotSet) {
					WriteElemStart(&x, elemType, nameLen, attrVal);
					}
					attrValIndex = attrVal;
				case YXML_CONTENT:
					if (*(x.data) == '\n' && lastType == YXML_ELEMSTART) {// If ff50 element with no d:id value
						for (int i = 0; i < 4; i++) // reset the attrval because an empty element won't clear this val
							attrVal[i] = 0;
						WriteElemStart(&x, FF50, nameLen, attrVal);
					}
					break;
				case YXML_ELEMEND:
					//printf("CLOSE %s\n", x.elem);

					break;
			}
		lastType = r;
	}
	printf("Done.\n");
	free(source);
	fclose(xmlDoc);
	fclose(outFile);
	return 0;
}

// Given a pointer, does proper allocation and reads the xml file into the memory
char* readXML(char* source) {
	// Read xml bytes
	if (fseek(xmlDoc, 0L, SEEK_END) == 0) {
		/* Get the size of the file. */
		long bufsize = ftell(xmlDoc);
		if (bufsize == -1) { /* Error */ }

		/* Allocate our buffer to that size. */
		source = malloc(sizeof(char) * (bufsize + 1));

		/* Go back to the start of the file. */
		if (fseek(xmlDoc, 0L, SEEK_SET) != 0) { /* Error */ }

		/* Read the entire file into memory. */
		size_t newLen = fread(source, sizeof(char), bufsize, xmlDoc);
		if ( ferror( xmlDoc ) != 0 ) {
			fputs("Error reading file", stderr);
		} else {
			source[newLen++] = '\0'; /* Just to be safe. */
		}
	}
	return source;
}

// Writes the prelude to the bin file
 void WritePrelude() {
	char *prelude1 = "SERZ";
	uint32_t prelude2 = 0x00010000;
	fwrite(prelude1, sizeof(char), sizeof(4), outFile);
	fwrite(&prelude2, sizeof(uint32_t), 1, outFile);
	return;
}

/*
typedef struct {
	uint32_t children; // uint32 because that's the format it is written in the bin file
	uint32_t linenum;  // uint32 because that's the type of the yxml_t.line attribute
	long childrenAdd;  // long based on the type of bufsize
} record;
*/

void WriteElemStart(yxml_t* x, enum elType elemType, size_t nameLen, char *attrVal) {
	uint32_t attrValLen;

	static uint16_t ff50 = 0x50FF;
	static uint16_t ff56 = 0x56FF;
	static uint16_t ff41 = 0x41FF;
	static uint16_t zero = 0x0;
	switch (elemType) {
		case FF50:
			records.index++;
			records.records[records.index].linenum = x->line;
			fwrite(&ff50, sizeof(uint16_t), 1, outFile);
			checkMap(x->elem, nameLen);
			int numVal = atoi(attrVal);
			fwrite(&numVal, sizeof(uint32_t), 1, outFile);
			records.records[records.index].childrenAdd = ftell(outFile);
			fwrite(&zero, sizeof(uint32_t), 1, outFile);

			break;
		case FF56:
			records.records[records.index].children++;
			fwrite(&ff56, sizeof(uint16_t), 1, outFile);
			checkMap(x->elem, nameLen);
			attrValLen = strlen(attrVal);
			checkMap(attrVal, attrValLen);
			break;
		case FF41:
			records.records[records.index].children++;
			fwrite(&ff41, sizeof(uint16_t), 1, outFile);
			checkMap(x->elem, nameLen);
			attrValLen = strlen(attrVal);
			attrValLen = strlen(attrVal) - 1; // Leaves off the 1-byte elementCount value
			break;
		default:
			printf("NO MATCH: %s, %d\n", x->elem, elemType);
	}

	return;
}

/*
typedef struct {
	record records[30];
	int index;
} recordArr;
*/

// Check for symbol in map
void checkMap(char* symbol, size_t length) {
	static int mapIter = 0;
	static uint16_t ff = 0xFFFF;
	int *val = map_get(&symMap, symbol);
	if (val) { // if symbol match found
		fwrite(val, sizeof(uint16_t), 1, outFile);
	} else { // if symbol match not found
		fwrite(&ff, sizeof(char), 2, outFile);
		fwrite(&length, sizeof(uint32_t), 1, outFile);
		fwrite(symbol, sizeof(char), length, outFile);
		map_set(&symMap, symbol, mapIter++);
	}
}


// 
/*
typedef struct {
	yxml_t* yxml_elem;
	size_t symLen;
	enum elType elemType;
	recordArr* records;
	FILE* fp;
	map_int_t* symMap;
} elemInfo;
*/
