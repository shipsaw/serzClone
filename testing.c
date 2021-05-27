#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

uint16_t conv16(char *bytes);
uint32_t conv32(char *bytes);

int main()
{
    char testArr[] = {0xaa, 0x82, 0xd1, 0x10};
    int result = (int)conv32(testArr);
    printf("result: %d, expected 282165930\n", result);

    return 0;
}

uint16_t conv16(char *bytes) {
	uint16_t x = ((uint8_t)bytes[0] | (uint8_t)bytes[1] << 8);
	return x;
}

uint32_t conv32(char *bytes) {
	//printf("%x, %x, %x, %x\n", source[i], source[i+1], source[i+2], source[i+3]);
	uint32_t x = ((uint8_t)bytes[0] | (uint8_t)bytes[1] << 8 | (uint8_t)bytes[2] << 16 | (uint8_t)bytes[3] << 24);
	return x;
}