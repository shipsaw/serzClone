#include "map.h"
#include "serz.h"

// Global vars
map_str_t symSmap;	// symbol map
map_str_t elemSmap;	// element map
static FILE *outFile;

int binToXml(FILE *binFile, FILE *xmlFile) {
	char *source = NULL;
	source = readInfile(binFile);
	char *sourceIter = source;	// Use original malloc pointer for free later

	map_init(&symSmap);
	map_init(&elemSmap);

	return 0;
}
