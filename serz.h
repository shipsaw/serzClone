
#ifndef SERZ_H
#define SERZ_H
#endif

#include"yxml.h"
#include<stdio.h>

#define BUFSIZE 4096

typedef struct {
	uint32_t children; 	// uint32 because that's the format it is written in the bin file
	uint16_t mapId;		// Id value of element name, used when calling FF 70
	long childrenAdd;  	// long based on the type of bufsize
} record;

typedef struct {
	record record[100];
	uint16_t idx;		// Does double duty as array index and FF70 return address
} recordArr;

// For binToXml, allows passing around of file status
typedef struct {
	long i; 		// Char position in bin file
	int tabPos;
	FILE *outFile;	// xml output file
	char *source;  //  input xml file copied into this buffer
} fileStatus;

// For binToXml, allows passing around of symbol maps
typedef struct {
	char *symArray[65536];
	uint16_t symArrayIdx;
	uint32_t elemArray[256];
	int linenum;
} symbolMaps;

typedef enum {ElemOpen, ElemClose, Other} elType ;					// Used to track enums we care about while ignoring others
typedef enum {boolean, sUInt8, sInt32, sFloat32, sUInt64, cDeltaString} contType ;	// Content type


char* readInfile(FILE*);
int xmlToBin(FILE*, FILE*);
int binToXml(FILE*, FILE*);

// xmlToBin
void WritePrelude();
void WriteFF50(yxml_t*, size_t, char*);
void WriteFF56(yxml_t*, size_t, char*);
void WriteFF41(yxml_t*, size_t, char*, int);
void WriteFF70(yxml_t*);

uint16_t checkSym(char*, size_t);
int checkElem(yxml_t*, char*);


void contWrite(char*, contType, int);
contType contentType(char* type);

// helpers.c
char* fileType(char *filename);
void swapFilename(char *filename);
long getFileSize(FILE *binFile);
uint32_t conv32(char*);
uint16_t conv16(char*);
uint32_t newElem(uint16_t, uint16_t);

// binToXml
int checkPrelude(fileStatus*);
void processFF(fileStatus*, symbolMaps*);
uint16_t newSym(fileStatus*, symbolMaps*);
void addTabs(fileStatus*);
void process50(fileStatus*, symbolMaps*);
void process56(fileStatus*, symbolMaps*);
void process41(fileStatus*, symbolMaps*);
void process70(fileStatus*, symbolMaps*);
