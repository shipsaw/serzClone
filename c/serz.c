#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "yxml.h"

#define BUFSIZE 4096

static int write50(yxml_t*);
static char* readXML(char*, FILE*);

enum elType{NotSet, FF50, FF56, FF41, FF70};

typedef struct {
	uint32_t children; // uint32 because that's the format it is written in the bin file
	uint32_t linenum;  // uint32 because that's the type of the yxml_t.line attribute
	long childrenAdd;  // long based on the type of bufsize
} record;

typedef struct {
	record records[30];
	int index;
} recordArr;

int main () {
	enum elType elemType;
	void *buf = malloc(BUFSIZE);
	yxml_t x;
	yxml_init(&x, buf, BUFSIZE);
	char *source = NULL;
	recordArr records;



	FILE *xmlDoc, *outFile;
	printf("Opening file..\n");
	// open xml file
	if ((xmlDoc = fopen("test.xml", "r")) == NULL) {
		printf("\nError opening file\n");
		exit(1);
	}
	// Open bin file
	if ((outFile = fopen("oldTest.bin", "wb")) == NULL) {
		printf("\nError opening file\n");
		exit(1);
	}
	source = readXML(source, xmlDoc);
	// Parse xml
	int lastType;
	char attrVal[40];
	char *attrValIndex = attrVal;
	char *sourceIter = source;
	elemType = NotSet;

	for (; *sourceIter; sourceIter++) {
		yxml_ret_t r = yxml_parse(&x, *sourceIter);
		if (r < 0)
			exit(1);
		if (r != 0)
		switch (r) {
			case YXML_ELEMSTART:
				break;
			case YXML_ATTRVAL:
				*(attrValIndex++) = *x.data;
				break;
			case YXML_ATTREND:
				*(attrValIndex) = '\0';
				if (strcmp(x.attr, "d:id") == 0) {
					//printf("FF 50 with id. %s, %s\n", x.elem, attrVal);
					attrValIndex = attrVal;
				} else if (strcmp(x.attr, "d:type") == 0) {
					//printf("FF 56: %s %s\n", x.elem, attrVal);
					attrValIndex = attrVal;
				} else if (strcmp(x.attr, "d:numElements") == 0) {
					//printf("FF 41: %s ", attrVal);
					attrValIndex = attrVal;
				} else if (strcmp(x.attr, "d:elementType") == 0) {
					//printf("%s\n", attrVal);
					attrValIndex = attrVal;
				} else {
					attrValIndex = attrVal;
				}
						
				break;
			case YXML_CONTENT:
				if (*(x.data) == '\n' && lastType == YXML_ELEMSTART)
					//printf("FF 50 EMPTY. %s\n", x.elem);
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
static char* readXML(char* source, FILE* xmlDoc) {
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
