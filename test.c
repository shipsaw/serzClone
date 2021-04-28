#include <stdio.h>
#include <stdint.h>

int main() {
	uint16_t testVal = 321;

	char *pt = &testVal;

	for (int i = 0; i < sizeof(uint16_t); i++) {
		printf("%c\n",*(pt++));
	}
	return 0;
}
