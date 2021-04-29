#ifndef SERZ_H
#define SERZ_H
#endif

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

typedef enum {ElemOpen, ElemClose, Other} elType ;					// Used to track enums we care about while ignoring others
typedef enum {boolean, sUInt8, sInt32, sFloat32, sUInt64, cDeltaString} contType ;	// Content type


char* readXML(char*);

void WritePrelude();
void WriteFF50(yxml_t*, size_t, char*);
void WriteFF56(yxml_t*, size_t, char*);
void WriteFF41(yxml_t*, size_t, char*, int);
void WriteFF70(yxml_t*);

uint16_t checkSym(char*, size_t);
int checkElem(yxml_t*, char*);


void contWrite(char*, contType, int);
contType contentType(char* type);

char* fileType(char *filename);
void swapFilename(char *filename);
