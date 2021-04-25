#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yxml.h"
#include "map.h"

#define BUFSIZE 4096

enum elType {NotSet, AnyOpen, FF50, FF56, FF41, FF70};
enum contType {boolean, sUInt8, sInt32, sFloat32, sUInt64, cDeltaString};

typedef struct {
	uint32_t children; // uint32 because that's the format it is written in the bin file
	uint16_t linenum;  // uint32 because that's the type of the yxml_t.line attribute
	long childrenAdd;  // long based on the type of bufsize
} record;

typedef struct {
	record records[30];
	int index;
} recordArr;

char* readXML(char*);
void WritePrelude();
void checkMap(char *symbol, size_t length);

void WriteFF50(yxml_t*, size_t, char*);
void WriteFF56(yxml_t*, size_t, char*);
void WriteFF41(yxml_t*, size_t, char*, int);
void WriteFF70(yxml_t*);
enum contType contentType(char*);
void contWrite(char*, enum contType, int);

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
	char *sourceIter = source;	// Makes sure we save the original source pointer for freeing

	WritePrelude(outFile);

	// Initialize record and value-keeping variables
	enum elType lastType;		// lastType saves the value of r from the previous loop
	enum elType closeTracker;	// closeTracker saves the value of r from the last elemstart/elemclose
	char attrVal[40];		// Attribute value buffer
	char contVal[200];		// Content value buffer
	char *attrValIndex = attrVal;	// Attrubute value iterator
	char *contValIndex = contVal;	// Content value iterator
	uint8_t ff41ElemCt = 1;		// Two attributes get writtern for ff41, numElements is read first but written second
	size_t nameLen;			// Length of x.attr
	enum contType contType;		// Content type enum

	// Parse xml
	for (; *sourceIter; sourceIter++) {
		yxml_ret_t r = yxml_parse(&x, *sourceIter);
		if (r < 0)
			exit(1);
		if (r != 0)
			switch (r) {
				case YXML_ELEMSTART:
					ff41ElemCt = 1;				// Reset FF 41 element count to default for other-elem types
					nameLen = yxml_symlen(&x, x.elem); 	// Uses the fast yxml function to get length rather than slower strlen
					closeTracker = AnyOpen;			// No FF 70 if a close is followed by any element open
					contValIndex = contVal;			// Reset content buffer
					break;
				case YXML_ATTRVAL:
					*(attrValIndex++) = *x.data;		// Add to value buffer
					break;
				case YXML_ATTREND:
					*(attrValIndex) = '\0';			// Terminate value buffer string
					attrValIndex = attrVal; 		// Reset buffer that writes attribute values
					switch (x.attr[2]) {
						case 'i': 			// If matches d:id
							elemType = FF50;
							WriteFF50(&x, nameLen, attrVal);
							break;
						case 't': 			// If matches d:type
							contType = contentType(attrVal);
							elemType = FF56;
							WriteFF56(&x, nameLen, attrVal);
							break;
						case 'n': 			// If matches numElements
							ff41ElemCt = atoi(attrVal);
							break;
						case 'e': 			// If matches d:elementType
							contType = contentType(attrVal);
							elemType = FF41;
							WriteFF41(&x, nameLen, attrVal, ff41ElemCt);
							break;
					}
					break;
				case YXML_CONTENT:
					if (*(x.data) == '\n' && lastType == YXML_ELEMSTART) {	// If ff50 element with no d:id value
						for (int i = 0; i < 4; i++) 			// reset the value buffer because an empty element won't clear this
							attrVal[i] = 0;
						WriteFF50(&x, nameLen, attrVal);
					} else if (*(x.data) == ' ' || !isspace(*x.data))  // Eliminate all newline-type characters, but not space
					*(contValIndex++) = *x.data;
					break;
				case YXML_ELEMEND:
					if (contValIndex != contVal) {
						*contValIndex = '\0';
						contWrite(contVal, contType, ff41ElemCt);
						contValIndex = contVal;
					}
					if (closeTracker == FF70) { // If a FF 70, which follows a one-line element close element
						WriteFF70(&x);
					}
					closeTracker= FF70;
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


void WriteFF50(yxml_t* x, size_t nameLen, char *attrVal) {
	static uint16_t ff50 = 0x50FF;
	static uint16_t zero = 0x0;

	records.records[records.index].children++;			// Update child count
	records.index++;						// Push new record element
	if (*(x->data) == '\n') {					// If we got the wrong line number because we just read a newline
		records.records[records.index].linenum = x->line - 2;
	} else {
		records.records[records.index].linenum = x->line -1;
	}

	fwrite(&ff50, sizeof(uint16_t), 1, outFile);			// Write FF 50
	checkMap(x->elem, nameLen);					// Write element Name

	int numVal = atoi(attrVal);					// Convert id value to int
	fwrite(&numVal, sizeof(uint32_t), 1, outFile);			// Write id value
	records.records[records.index].childrenAdd = ftell(outFile);	// Save the address where the blank children value was written, for future updating
	fwrite(&zero, sizeof(uint32_t), 1, outFile);			// Write blank children value

	return;
}

void WriteFF56(yxml_t* x, size_t nameLen, char *attrVal) {
	static uint16_t ff56 = 0x56FF;
	uint32_t attrValLen;

	records.records[records.index].children++;	// Update child count of current record
	fwrite(&ff56, sizeof(uint16_t), 1, outFile);	// Write FF 56
	checkMap(x->elem, nameLen);			// Write element name

	attrValLen = strlen(attrVal);			// Get Length of type value
	checkMap(attrVal, attrValLen);			// Write type value

	return;
}

void WriteFF41(yxml_t* x, size_t nameLen, char *attrVal, int numElems) {
	static uint16_t ff41 = 0x41FF;
	uint32_t attrValLen;

	records.records[records.index].children++;	// Update child count
	fwrite(&ff41, sizeof(uint16_t), 1, outFile);	// Write FF41
	checkMap(x->elem, nameLen);			// Write elem name

	attrValLen = strlen(attrVal);			// Get Length of type value
	checkMap(attrVal, attrValLen);			// Write type value
	fwrite(&numElems, sizeof(char), 1, outFile);	// Write content count

	return;
}

void WriteFF70(yxml_t* x) {
	static uint16_t ff70 = 0x70FF;
	uint16_t retLine;
	long currWriteAdd;

	long writeAdd = records.records[records.index].childrenAdd;
	int children = records.records[records.index].children;

	retLine = records.records[records.index].linenum;	// Pull return line off the records stack
	fwrite(&ff70, sizeof(uint16_t), 1, outFile);		// Write FF 70
	fwrite(&retLine, sizeof(uint16_t), 1, outFile);		// Write return line

	currWriteAdd = ftell(outFile);				// Save the current write position
	fseek(outFile, writeAdd,  SEEK_SET);			// Move to the child field of the open tag
	fwrite(&children, sizeof(uint32_t), 1, outFile);	// Write the child count
	fseek(outFile, currWriteAdd,  SEEK_SET);		// Return to current write position
	
	records.index--;					// Pop the records stack

	return;
}

// Check for symbol in map
void checkMap(char* symbol, size_t length) {
	static int mapIter = 0;					// Value to reference the string, increments every map addition
	static uint16_t ff = 0xFFFF;				// Prelude for not-found symbol write

	int *val = map_get(&symMap, symbol);			// See if symbol is in map
	if (val) { 						// If symbol match found, write that symbol
		fwrite(val, sizeof(uint16_t), 1, outFile);
	} else { 						// If symbol match not found
		fwrite(&ff, sizeof(char), 2, outFile);		// Write prelude
		fwrite(&length, sizeof(uint32_t), 1, outFile);	// Write symbol character length
		fwrite(symbol, sizeof(char), length, outFile);	// Write the symbol
		map_set(&symMap, symbol, mapIter++);		// Since this was a map miss, add symbol to map
	}
	
	return;
}

enum contType contentType(char* type) {
		if (strcmp(type, "bool") == 0) {
			return boolean;
		} else if(strcmp(type, "sUInt8") == 0) {
			return sUInt8;
		} else if(strcmp(type, "sInt32") == 0) {
			return sInt32;
		} else if(strcmp(type, "sFloat32") == 0) {
			return sFloat32;
		} else if(strcmp(type, "cDeltaString") == 0) {
			return cDeltaString;
		} else if(strcmp(type, "sUInt64") == 0) {
			return sUInt64;
		} else {
			printf("UNKNOWN TYPE: %s\n", type);
			exit(1);
		}
}

void contWrite(char* content, enum contType type, int count) {
	static int bool0 = 0;
	static int bool1 = 1;

	uint8_t uint8;
	uint32_t uint32;
	uint64_t uint64;
	float float32;

	char* token = strtok(content, " ");

	for (int i = 0; i < count; i++) {
		if (token != NULL) {
			content = token;
			token = strtok(NULL, " ");
		}

		switch (type) {
			case boolean:
				if (content[0] == '0') {
					fwrite(&bool0, sizeof(char), 1, outFile);
				} else {
					fwrite(&bool1, sizeof(char), 1, outFile);
				}
				break;
			case sUInt8:
				uint8 = atoi(content);
				fwrite(&uint8, sizeof(char), 1, outFile);
				break;
			case sInt32:
				uint32 = atoi(content);
				fwrite(&uint32, sizeof(uint32_t), 1, outFile);
				break;
			case sFloat32:
				float32 = atof(content);
				fwrite(&float32, sizeof(uint32_t), 1, outFile);
				break;
			case cDeltaString:
				checkMap(content, strlen(content));
				break;
			case sUInt64:
				uint64 = strtoll(content, NULL, 10);
				fwrite(&uint64, sizeof(uint64_t), 1, outFile);
				break;
			default:
				printf("Error: uknown type enum\n");
				exit(1);
		}
	}
}
