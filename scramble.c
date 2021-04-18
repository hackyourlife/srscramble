#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

u16 data_scramble_tbl[17] = { 4, 1, 0x10, 0x20, 0x80, 0x40, 0x8, 0x2, 0x400,
		0x100, 0x1000, 0x2000, 0x8000, 0x4000, 0x800, 0x200, 0 };
u32 addr_scramble_tbl_16[19] = { 1, 0x10, 4, 8, 2, 0x2000, 0x80, 0x1000, 0x20,
		0x400, 0x10000, 0x200, 0x40, 0x100, 0x4000, 0x20000, 0x800,
		0x8000, 0 };
u32 addr_scramble_tbl_8[20] = { 4, 1, 8, 0x10, 0x2, 0x200, 0x2000, 0x400,
		0x40000, 0x20000, 0x40, 0x8000, 0x800, 0x10000, 0x100, 0x20,
		0x1000, 0x80, 0x4000, 0 };

u16 scramble_data(u16 word)
{
	u16 out = 0;
	u16 mask = 0xFFFF;
	u16* scramble = data_scramble_tbl;
	u16 bit = 1;

	while(*scramble) {
		if((word & bit) != 0)
			out |= *scramble;
		scramble++;
		bit <<= 1;
		mask <<= 1;
	}

	return out | mask & word;
}

u32 scramble_addr(u32 addr, int width)
{
	u32* scramble;
	u32 out = 0;
	u32 mask = 0xFFFFFFFF;
	u32 bit = 1;

	if(width == 8)
		scramble = addr_scramble_tbl_8;
	else
		scramble = addr_scramble_tbl_16;

	while(*scramble) {
		if((addr & bit) != 0)
			out |= *scramble;
		scramble++;
		bit <<= 1;
		mask <<= 1;
	}

	return out | addr & mask;
}

int main(int argc, char** argv)
{
	if(argc != 3) {
		printf("Usage: %s descrambled.bin scrambled.bin", *argv);
		return 1;
	}

	const char* filename_in = argv[1];
	const char* filename_out = argv[2];

	FILE* f = fopen(filename_in, "rb");
	if(!f) {
		printf("Error: cannot open %s: %s\n", filename_in,
				strerror(errno));
		return 1;
	}

	fseek(f, 0, SEEK_END);
	size_t fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	u8* buf = (u8*) malloc(fsize);
	u8* outbuf = (u8*) malloc(fsize);
	fread(buf, fsize, 1, f);
	fclose(f);

	int width;
	const char* type;

	// figure out ROM type: scramble first 32 bytes
	for(size_t i = 0; i < fsize; i++) {
		u32 addr = scramble_addr(i, 8);
		u16 tmp = scramble_data(buf[i]);
		outbuf[addr] = tmp;
	}

	if(strncmp(outbuf, "Roland", 6)) {
		if(strncmp(outbuf, "JP-800", 6)) {
			// try again with 16bit
			for(size_t i = 0; i < fsize; i++) {
				u32 addr = scramble_addr(i, 16);
				u16 tmp = scramble_data(buf[i]);
				outbuf[addr] = tmp;
			}
			if(!strncmp(outbuf, "Roland", 6) &&
					!strncmp(&outbuf[0xC], "O\xB0X", 3)) {
				width = 16;
				type = "SRX";
			} else {
				printf("Unknown ROM type\n");
				return 1;
			}
		} else {
			width = 8;
			type = "JP-800";
		}
	} else if(!strncmp(&outbuf[0xC], "O\xB0S", 3)) {
		width = 8;
		type = "SR-JV80";
	} else {
		printf("Unknown ROM type\n");
		return 1;
	}

	printf("ROM size: %lu [%d bit data]\n", fsize, width);
	printf("ROM type: %s\n", type);

	// print ROM info
	if(width == 8) {
		// SR-JV80 / JP-800
		printf("ROM ID:   ");
		for(int i = 0x20; i < 0x26; i++) {
			printf("%c", buf[i]);
		}
		printf("\n");
		printf("Date:     ");
		for(int i = 0x30; i < 0x3A; i++) {
			printf("%c", buf[i]);
		}
		printf("\n");
	} else if(width == 16) {
		// SRX
		printf("ROM ID:   ");
		for(int i = 0x20; i < 0x30; i++) {
			printf("%c", buf[i]);
		}
		printf("\n");
		printf("Date:     ");
		for(int i = 0x30; i < 0x3A; i++) {
			printf("%c", buf[i]);
		}
		printf("\n");
	}

	//////////////////////////////////////////////
	// descramble the whole ROM
	for(size_t i = 0; i < fsize; i++) {
		u32 addr = scramble_addr(i, width);
		u16 tmp = scramble_data(buf[i]);
		outbuf[addr] = tmp;
	}
	//////////////////////////////////////////////

	FILE* out = fopen(filename_out, "wb");
	if(!out) {
		printf("Error: cannot open %s: %s\n", filename_out,
				strerror(errno));
		return 1;
	}

	fwrite(outbuf, fsize, 1, out);
	fclose(out);

	return 0;
}
