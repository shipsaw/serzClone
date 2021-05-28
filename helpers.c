#include<stdlib.h>
#include<string.h>
#include"serz.h"

// Given a pointer, does proper allocation and reads the file into the memory
char* readInfile(FILE* infile) {
	char* buf;
	// Read xml bytes
	if (fseek(infile, 0L, SEEK_END) == 0) {
		/* Get the size of the file. */
		long bufsize = ftell(infile);
		if (bufsize == -1) {
			exit(1);
		}
		/* Allocate our buffer to that size. */
		buf = malloc(sizeof(char) * (bufsize + 1));

		/* Go back to the start of the file. */
		if (fseek(infile, 0L, SEEK_SET) != 0) {
			exit(1);
		}

		/* Read the entire file into memory. */
		size_t newLen = fread(buf, sizeof(char), bufsize, infile);
		if ( ferror( infile ) != 0 ) {
			fputs("Error reading file", stderr);
		} else {
			buf[newLen++] = '\0'; /* Just to be safe. */
		}
	}
	return buf;
}

// fileType takes a string and determines if it is a .bin or a .xml.  It returns the first character of the filetype extension
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

// swapFilename takes a filename string, and if .xml changes the extension to .bin and vice-versa
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

long getFileSize(FILE *binFile) {
	long bufsize;
	if (fseek(binFile, 0L, SEEK_END) == 0) {		// Find the size of the binary file
		bufsize = ftell(binFile);
		if (bufsize == -1) {
			return -1;
		}
	} else {
		return -1;
	}
	return bufsize;
}

// conv32 converts 4 bytes in sequence (little-endian) into a uint32_t value
uint32_t conv32(fileStatus *fs) {
	char *src = fs->source;
	long i = fs->i;
	//printf("%x, %x, %x, %x\n", source[i], source[i+1], source[i+2], source[i+3]);
	uint32_t x = ((uint8_t)src[i] | (uint8_t)src[i+1] << 8 | (uint8_t)src[i+2] << 16 | (uint8_t)src[i+3] << 24);
	fs->i += 4;
	return x;
}

// conv16 converts 2 bytes in sequence (little-endian) into a uint16_t value
uint16_t conv16(fileStatus *fs) {
	char *src = fs->source;
	long i = fs->i;
	uint16_t x = ((uint8_t)src[i] | (uint8_t)src[i+1] << 8);
	fs->i += 2;
	return x;
}

