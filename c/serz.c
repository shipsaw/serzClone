#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "yxml.h"

#define BUFSIZE 4096

static int write50(yxml_t*);

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
	void *buf = malloc(BUFSIZE);
	yxml_t x;
	yxml_init(&x, buf, BUFSIZE);
	char *source = NULL;
	recordArr records;



	FILE *xmlDoc, *outFile;
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
	// Parse xml
	int bitCount = 0;
	char *sourceIter = source;
	for (; *sourceIter; sourceIter++) {
		yxml_ret_t r = yxml_parse(&x, *sourceIter);
		if (r < 0)
			exit(1);
		switch (r) {
			case YXML_ATTRVAL:
				if (strcmp(x.attr,"d:id") == 0) {
					if (bitCount == 8) {
						write50(&x);
						bitCount = 0;
						continue;
					}
					bitCount++;
				}
				break;
			case YXML_CONTENT:
				//printf("%c", *(x.data));
				break;
			case YXML_ELEMEND:
				//printf("END:%s", x.elem);
				break;
		}
	}

	if (fwrite("Hello", sizeof(char), sizeof("Hello"), outFile) == 0) {
		printf("Write error\n");
		exit(1);
	}
	printf("Done.\n");
	free(source);
	fclose(xmlDoc);
	fclose(outFile);
	return 0;
}

static int write50(yxml_t *x) {
	printf("FF 50, %lu, %s\n ", strlen(x->elem), x->elem);
	//for (int i = -8; i < 0; i++
	return 0;
}
