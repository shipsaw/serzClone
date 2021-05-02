#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "yxml.h"
#include "map.h"
#include "serz.h"

// Global vars
map_int_t symMap; 	// symbol map
map_int_t elemMap;	// element map
FILE *outFile, *xmlDoc; // binary file
recordArr rStack;	// stack of open xml elements

int main (int argc, char** argv) {
	// memory and file variables
	void *buf = malloc(BUFSIZE);
	char *source = NULL;

	// yxml variables
	yxml_t x;
	yxml_init(&x, buf, BUFSIZE);

	// map variables
	map_init(&symMap);
	map_init(&elemMap);

	// user-defined types
	elType elemType;
	rStack.idx = 0;

	// open xml file
	if (argc <= 1 || argc > 3) {
		printf("Usage: serz infile /{bin,xml}:outfile\nOr: serz infile\n");
		exit(1);
	} else {
		if ((xmlDoc = fopen(argv[1], "r")) == NULL) {
			printf("\nError opening file\n");
			exit(1);
		}
	}

	char outFileName[strlen(argv[1])];
	strcpy(outFileName, argv[1]);
	swapFilename(outFileName);

	// Open bin file
	if ((outFile = fopen(outFileName, "wb")) == NULL) {
		printf("\nError opening file\n");
		exit(1);
	}

	printf("Converting %s to %s...\n", argv[1], outFileName);
	source = readXML(source);
	char *sourceIter = source;	// Makes sure we save the original source pointer for freeing

	WritePrelude(outFile);

	// Initialize record and value-keeping variables
	yxml_ret_t lastType;		// Saves whether ElemOpen or ElemClosed was the most recently read
	elType closeTracker;	// closeTracker saves the value of r from the last elemstart/elemclose

	char attrVal[40];		// Attribute value buffer
	char contVal[200];		// Content value buffer
	char *attrValIndex = attrVal;	// Attrubute value iterator
	char *contValIndex = contVal;	// Content value iterator

	uint8_t elemCount = 1;		// Two attributes get writtern for ff41, numElements is read first but written second
	size_t nameLen;			// Length of x.attr
	contType retContType;		// Content type returned from content type function
	char nextChar;			// Used to store the upcoming char, used to predict and handle void elements
	char prevChar;			// Used to store prevous char, used to recognize void elements with id's
   uint64_t seekAdjust;      // Adjusts for but in yxml where a newline somehow throws the "total" property off by one 
                                 // every time

	// Parse xml
	for (; *sourceIter; sourceIter++) {
		yxml_ret_t r = yxml_parse(&x, *sourceIter);
		if (r < 0)
			exit(1);
		if (r != 0) {
            seekAdjust = x.line - 2;   // Account for newline "total" error
			switch (r) {
				case YXML_ELEMSTART:
					elemCount = 1;				// Reset FF 41 element count to default for other-elem types
					nameLen = yxml_symlen(&x, x.elem); 	// Uses the fast yxml function to get length rather than slower strlen
					closeTracker = ElemOpen;		// No FF 70 if a close is followed by any element open
					contValIndex = contVal;			// Reset content buffer

					fseek(xmlDoc, x.total + seekAdjust, SEEK_SET);				// Seek to the next character to read
					fread(&nextChar, sizeof(char), 1, xmlDoc);			// Read that character
					if (nextChar == '>' || nextChar == '\r' || nextChar == '\n') 	// If empty FF50>\r\n or void Element/>
						WriteFF50(&x, nameLen, NULL);
					break;
				case YXML_ATTRVAL:
					*(attrValIndex++) = *x.data;		// Add to value buffer
					break;
				case YXML_ATTREND:
					*(attrValIndex) = '\0';			// Terminate value buffer string
					attrValIndex = attrVal; 		// Reset buffer that writes attribute values

					switch (x.attr[2]) {
						case 'i': 			// If matches d:id
							WriteFF50(&x, nameLen, attrVal);
							break;
						case 't': 			// If matches d:type
							retContType = contentType(attrVal);
							WriteFF56(&x, nameLen, attrVal);
							break;
						case 'n': 			// If matches numElements
							elemCount = atoi(attrVal);
							break;
						case 'e': 			// If matches d:elementType
							retContType = contentType(attrVal);
							WriteFF41(&x, nameLen, attrVal, elemCount);
						case 'a': 			// alt_encoding
							break;
						case 'p':			// precision
							break;
					}
					break;
				case YXML_CONTENT:
					if (*(x.data) == ' ' || !isspace(*x.data))  // Eliminate all newline-type characters, but not space
						*(contValIndex++) = *x.data;
					break;
				case YXML_ELEMEND:
					if (contValIndex != contVal) {				// If there is content to write
						*contValIndex = '\0';
						contWrite(contVal, retContType, elemCount);
						contValIndex = contVal;
					}

					fseek(xmlDoc, x.total - 2, SEEK_SET);		// Seek to the next character to read
					fread(&prevChar, sizeof(char), 1, xmlDoc);	// Read that character
					if (prevChar == '/')				// Void elements are followed by ff70's
						WriteFF70(&x);

					if (closeTracker == ElemClose) // If a FF 70, which follows a one-line element close element
						WriteFF70(&x);
					closeTracker= ElemClose;
					break;
			}
			lastType = r;
		}
	}
	printf("Conversion complete\n");
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
	static char *prelude1 = "SERZ";
	static uint32_t prelude2 = 0x00010000;
	static uint32_t prelude3 = 0x0C0043FF;
	static uint32_t prelude4 = 0;
	fwrite(prelude1, sizeof(char), sizeof(4), outFile);
	fwrite(&prelude2, sizeof(uint32_t), 1, outFile);
	// TODO Check for xml start element, if so write this next part of the prelude
	fwrite(&prelude3, sizeof(uint32_t), 1, outFile);
	fwrite(&prelude4, sizeof(char), 3, outFile);
	return;
}


void WriteFF50(yxml_t* x, size_t nameLen, char *attrVal) {
	static uint16_t ff50 = 0x50FF;
	static uint16_t zero = 0x0;
	uint16_t mapIdNum;

	rStack.record[rStack.idx].children++;			// Update child count
	rStack.idx++;						// Push new record element
	rStack.record[rStack.idx].children = 0;			// Make sure the children value is reset on opening new stack frame

	if (!checkElem(x, x->elem)) {				// Check for element duplicates, 
		fwrite(&ff50, sizeof(uint16_t), 1, outFile);			// If first instance of this element..
		mapIdNum = checkSym(x->elem, nameLen);				// Write element Name
		rStack.record[rStack.idx].mapId = mapIdNum;		// Push map id num onto records stack
	}

	if (attrVal) {
		int numVal = atoi(attrVal);					// Convert id value to int
		fwrite(&numVal, sizeof(uint32_t), 1, outFile);			// Write id value
	} else {
		fwrite(&zero, sizeof(uint32_t), 1, outFile);			// Write id value
	}
	rStack.record[rStack.idx].childrenAdd = ftell(outFile);	// Save the address where the blank children value was written, for future updating
	fwrite(&zero, sizeof(uint32_t), 1, outFile);			// Write blank children value

	return;
}

void WriteFF56(yxml_t* x, size_t nameLen, char *attrVal) {
	static uint16_t ff56 = 0x56FF;
	uint32_t attrValLen;
	attrValLen = strlen(attrVal);			// Get Length of type value

	size_t elemKeyLen = nameLen + attrValLen + 1;
	char elemKey[elemKeyLen];		// name+attribute key used for element replacement lookup
	strcpy(elemKey, x->elem);
	strcat(elemKey, attrVal);

	rStack.record[rStack.idx].children++;	// Update child count of current record
	if (!checkElem(x, elemKey)) {
		fwrite(&ff56, sizeof(uint16_t), 1, outFile);	// Write FF 56
		checkSym(x->elem, nameLen);			// Write element name

		checkSym(attrVal, attrValLen);			// Write type value
	}

	return;
}

void WriteFF41(yxml_t* x, size_t nameLen, char *attrVal, int numElems) {
	static uint16_t ff41 = 0x41FF;
	uint32_t attrValLen;
	attrValLen = strlen(attrVal);			// Get Length of type value
	size_t elemKeyLen = nameLen + attrValLen + 1;
	char elemKey[elemKeyLen];			// name+attribute key used for element replacement lookup
	strcpy(elemKey, x->elem);
	strcat(elemKey, attrVal);

	rStack.record[rStack.idx].children++;		// Update child count
	if (!checkElem(x, elemKey)) {			// Check for duplicate element
		fwrite(&ff41, sizeof(uint16_t), 1, outFile);	// Write FF41
		checkSym(x->elem, nameLen);			// Write elem name

		checkSym(attrVal, attrValLen);			// Write type value
	}

	fwrite(&numElems, sizeof(char), 1, outFile);	// Write content count

	return;
}

void WriteFF70(yxml_t* x) {
	static uint16_t ff70 = 0x70FF;
	long currWriteAdd;
	uint16_t mapIdNum = rStack.record[rStack.idx].mapId;
	char mapIdStr[sizeof(uint16_t) + 2];			// One for zero, one for slash
	mapIdStr[0] = '/';
	sprintf(mapIdStr + 1, "%x", mapIdNum);

	long writeAdd = rStack.record[rStack.idx].childrenAdd;
	int children = rStack.record[rStack.idx].children;

	if (!checkElem(x, mapIdStr)) {
		fwrite(&ff70, sizeof(uint16_t), 1, outFile);		// Write FF 70
		fwrite(&mapIdNum, sizeof(uint16_t), 1, outFile);	// Write return line
	}

	currWriteAdd = ftell(outFile);				// Save the current write position
	fseek(outFile, writeAdd,  SEEK_SET);			// Move to the child field of the open tag
	fwrite(&children, sizeof(uint32_t), 1, outFile);	// Write the child count
	fseek(outFile, currWriteAdd,  SEEK_SET);		// Return to current write position


	rStack.idx--;					// Pop the records stack

	return;
}

// Check for symbol in map
uint16_t checkSym(char* symbol, size_t length) {
	static int mapIter = 0;					// Value to reference the string, increments every map addition
	static uint16_t ff = 0xFFFF;				// Prelude for not-found symbol write

	int *val = map_get(&symMap, symbol);			// See if symbol is in map
	if (val) { 						// If symbol match found, write that symbol
		fwrite(val, sizeof(uint16_t), 1, outFile);
		return *val;
	} else { 						// If symbol match not found
		fwrite(&ff, sizeof(char), 2, outFile);		// Write prelude
		fwrite(&length, sizeof(uint32_t), 1, outFile);	// Write symbol character length
		fwrite(symbol, sizeof(char), length, outFile);	// Write the symbol
		map_set(&symMap, symbol, mapIter);		// Since this was a map miss, add symbol to map
		return mapIter++;
	}
}

// checkElem takes the concatinated element name and attribute values and, if a duplicate,
// replaces them with the line number where this element is first found.  Returns 1 if found, zero
// if duplicate not found
int checkElem(yxml_t *x, char* symbol) {
	int *val = map_get(&elemMap, symbol);
	if (val) {
		fwrite(val, sizeof(uint8_t), 1, outFile);
		return 1;
	} else {
		map_set(&elemMap, symbol, x->line - 1);		// Indexing of serz starts at 0, yxml a 1
		return 0;
	}
}

contType contentType(char* type) {
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

void contWrite(char* content, contType type, int count) {
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
				checkSym(content, strlen(content));
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
