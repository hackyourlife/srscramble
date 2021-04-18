#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

u16 data_descramble_tbl[17] = { 2, 0x80, 1, 0x40, 4, 8, 0x20, 0x10, 0x200,
		0x8000, 0x100, 0x4000, 0x400, 0x800, 0x2000, 0x1000, 0 };
u32 addr_descramble_tbl_8[20] = { 2, 0x10, 1, 4, 8, 0x8000, 0x400, 0x20000,
		0x4000, 0x20, 0x80, 0x1000, 0x10000, 0x40, 0x40000, 0x800,
		0x2000, 0x200, 0x100, 0 };
u32 addr_descramble_tbl_16[19] = { 1, 0x10, 4, 8, 2, 0x100, 0x1000, 0x40,
		0x2000, 0x800, 0x200, 0x10000, 0x80, 0x20, 0x4000, 0x20000,
		0x400, 0x8000, 0 };

u16 descramble_data(u16 word)
{
	u16 out = 0;
	u16 mask = 0xFFFF;
	u16* scramble = data_descramble_tbl;
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

u32 descramble_addr(u32 addr, int width)
{
	u32* scramble;
	u32 out = 0;
	u32 mask = 0xFFFFFFFF;
	u32 bit = 1;

	if(width == 8)
		scramble = addr_descramble_tbl_8;
	else
		scramble = addr_descramble_tbl_16;

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
		printf("Usage: %s scrambled.bin descrambled.bin", *argv);
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

	// figure out ROM type
	if(strncmp(buf, "Roland", 6)) {
		if(strncmp(buf, "JP-800", 6)) {
			printf("Invalid ROM: %c%c%c%c%c%c\n", buf[0], buf[1],
					buf[2], buf[3], buf[4], buf[5]);
			return 1;
		} else {
			width = 8;
			type = "JP-800";
		}
	} else if(!strncmp(&buf[0xC], "O\xB0S", 3)) {
		width = 8;
		type = "SR-JV80";
	} else if(!strncmp(&buf[0xC], "O\xB0X", 3)) {
		width = 16;
		type = "SRX";
	} else {
		printf("Unknown ROM type: %c%c%c\n", buf[0xC], buf[0xD],
				buf[0xE]);
		return 1;
	}

	printf("ROM size: %lu [%d bit data]\n", fsize, width);
	printf("ROM type: %s\n", type);

	//////////////////////////////////////////////
	// descramble the whole ROM
	for(size_t i = 0; i < fsize; i++) {
		u32 addr = descramble_addr(i, width);
		u16 tmp = descramble_data(buf[i]);
		outbuf[addr] = tmp;
	}
	//////////////////////////////////////////////

	// print ROM info
	if(width == 8) {
		// SR-JV80 / JP-800
		printf("ROM ID:   ");
		for(int i = 0x20; i < 0x26; i++) {
			printf("%c", outbuf[i]);
		}
		printf("\n");
		printf("Date:     ");
		for(int i = 0x30; i < 0x3A; i++) {
			printf("%c", outbuf[i]);
		}
		printf("\n");
	} else if(width == 16) {
		// SRX
		printf("ROM ID:   ");
		for(int i = 0x20; i < 0x30; i++) {
			printf("%c", outbuf[i]);
		}
		printf("\n");
		printf("Date:     ");
		for(int i = 0x30; i < 0x3A; i++) {
			printf("%c", outbuf[i]);
		}
		printf("\n");
	}

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
