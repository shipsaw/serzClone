#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "map.h"
#include "serz.h"

const char *prolog = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
const char *binPrelude = "SERZ\x00\x00\x01\x00";

int binToXml(FILE *binFile, FILE *xmlFile)
{
	fileStatus fs;
	symbolMaps sm;
	fs.outFile = xmlFile;
	fs.source = readInfile(binFile);

	if (checkPrelude(&fs) == -1)
	{ // Check that the binary file has the correct "SERZ" prelude
		printf("Incorrect Prelude\n");
		exit(1);
	}
	fputs(prolog, fs.outFile); // Write the xml prolog to the output file

	long fileSize = getFileSize(binFile);
	for (long i = 8; i < fileSize; i++)
	{ //TODO once all elements can be processed the i++ should not be neccessary
		switch (fs.source[i])
		{
		case '\xFF': //TODO update, eventually we shouldn't need to look for FF's
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
int checkPrelude(fileStatus *fs)
{
	for (int i = 0; i < 8; i++)
	{
		if (fs->source[i] != binPrelude[i])
		{
			return -1;
		}
	}
	return 1;
}

// ProcessFF is called at the first of the two \xFF's that define the element type
void processFF(fileStatus *fs, symbolMaps *sm)
{
	fs->i++; // Move from the FF to the element type byte
	switch (fs->source[fs->i])
	{
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

// process50 is called when i is at the 0x50, and handles writing an FF50 element to the output file
void process50(fileStatus *fs, symbolMaps *sm)
{
	uint16_t nameSym, attrSym = 0; // 16-bit references to the symbol array
	writeElemBegin(fs, true);
	lookupNameSymbol(fs, sm);

	uint32_t id = conv32(fs); // convert d:id attribute value to int
	fs->i += 4;				  // Skip over # of children bytes
	if (id == 0)
	{ // If zero value of d:id attribute, nothing is written
		fputs(">\n", fs->outFile);
	}
	else
	{ // Else, write d:id and attribute value
		fprintf(fs->outFile, " d:id=\"%u\">\n", id);
	}

	newElem(nameSym, attrSym); // Save line in elemArray if < 255
	return;
}

void process56(fileStatus *fs, symbolMaps *sm)
{
	uint16_t nameSym, attrSym;

	writeElemBegin(fs, false);
	lookupNameSymbol(fs, sm);

	fprintf(fs->outFile, " d:type=\"");
	if (fs->source[fs->i] == '\xFF')
	{
		attrSym = newSym(fs, sm);
	}
	else
	{
		int arrIdx = conv16(fs);
		fputs(sm->symArray[arrIdx], fs->outFile);
	}
	fputs("\">\n", fs->outFile);

	newElem(nameSym, attrSym);
}

void process41(fileStatus *fs, symbolMaps *sm)
{
	uint16_t nameSym, attrSym = 0; // TODO get attribute symbol

	writeElemBegin(fs, false);
	lookupNameSymbol(fs, sm);

	fputs(">\n", fs->outFile);
	newElem(nameSym, attrSym);
	return;
}

void process70(fileStatus *fs, symbolMaps *sm)
{
	fs->tabPos--;
	uint16_t nameSym = 0, attrSym = 0;
	addTabs(fs);
	fputs("</", fs->outFile);

	lookupNameSymbol(fs, sm);

	fputs(">\n", fs->outFile);
	newElem(nameSym, attrSym);
}

void addTabs(fileStatus *fs)
{
	for (int i = 0; i < fs->tabPos; i++)
	{
		fputc('\t', fs->outFile);
	}
	return;
}

void writeElemBegin(fileStatus *fs, bool hasChildren)
{
	addTabs(fs);
	if (hasChildren)
		fs->tabPos++;		 // Increment the tab count, because 0x50 always has children
	fputc('<', fs->outFile); // Write opening < char of element
}

void lookupNameSymbol(fileStatus *fs, symbolMaps *sm)
{
	uint16_t nameSym; 						// 16-bit references to the symbol array
	writeElemBegin(fs, true);
	fs->i++;
	if (fs->source[fs->i] == '\xFF')
	{							  			// If the byte after 0x50 is FF, this is a new symbol
		nameSym = newSym(fs, sm); 			// Save this new symbol in the table, return it's reference number
	}
	else
	{ 										// If the symbol already exists in the symbol array
		int arrIdx = conv16(fs);
		fputs(sm->symArray[arrIdx], fs->outFile); // Write the value at the symbol array to the xml file
	}
}

// newElem takes the 16-bit symbol array values for the name and attribute, concatinating them into a 32-bit
// value that can be stored to save the element signiture
uint32_t newElem(uint16_t nameSym, uint16_t attrSym)
{
	uint32_t elemKey = nameSym << 8 | attrSym;
	printf("ElemKey = %04X\n", elemKey);
	return elemKey;
}

// Called at first FF of two that indicates new symbol, writes symbol to output file and returns 16-bit symbol array key
uint16_t newSym(fileStatus *fs, symbolMaps *sm)
{
	char symbol[20];
	fs->i += 2;						// Advance past two FF bytes
	long x = conv32(fs);			// 4 bytes that represent symbol length in bytes
	char *nameElem = malloc(x + 1); // Name string that will be stored in name array
	long j;
	for (j = 0; j < x; j++)
	{ // Copy letters from source to temp array
		nameElem[j] = fs->source[fs->i + j];
	}
	nameElem[j + 1] = '\0'; // Add null terminator to string

	fputs(nameElem, fs->outFile); // Write the word to the outFile
	fs->i += x;					  // Increment i the length of the word

	sm->symArray[sm->symArrayIdx] = nameElem;
	sm->symArrayIdx++;

	return sm->symArrayIdx;
}
