#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

u16 scramble_data(u16 word)
{
	return    (word & 0x0001) << 2
		| (word & 0x0002) >> 1
		| (word & 0x0004) << 2
		| (word & 0x0008) << 2
		| (word & 0x0010) << 3
		| (word & 0x0020) << 1
		| (word & 0x0040) >> 3
		| (word & 0x0080) >> 6
		| (word & 0x0100) << 2
		| (word & 0x0200) >> 1
		| (word & 0x0400) << 2
		| (word & 0x0800) << 2
		| (word & 0x1000) << 3
		| (word & 0x2000) << 1
		| (word & 0x4000) >> 3
		| (word & 0x8000) >> 6;
}

u32 scramble_addr(u32 addr, int width)
{
	if(width == 8) {
		return    (addr & 0x00000001) << 2
			| (addr & 0x00000002) >> 1
			| (addr & 0x00000004) << 1
			| (addr & 0x00000008) << 1
			| (addr & 0x00000010) >> 3
			| (addr & 0x00000020) << 4
			| (addr & 0x00000040) << 7
			| (addr & 0x00000080) << 3
			| (addr & 0x00000100) << 10
			| (addr & 0x00000200) << 8
			| (addr & 0x00000400) >> 4
			| (addr & 0x00000800) << 4
			| (addr & 0x00001000) >> 1
			| (addr & 0x00002000) << 3
			| (addr & 0x00004000) >> 6
			| (addr & 0x00008000) >> 10
			| (addr & 0x00010000) >> 4
			| (addr & 0x00020000) >> 10
			| (addr & 0x00040000) >> 4
			| (addr & 0xFFF80000);
	} else {
		return    (addr & 0x00000002) << 3
			| (addr & 0x00000010) >> 3
			| (addr & 0x00000020) << 8
			| (addr & 0x00000040) << 1
			| (addr & 0x00000080) << 5
			| (addr & 0x00000100) >> 3
			| (addr & 0x00000200) << 1
			| (addr & 0x00000400) << 6
			| (addr & 0x00000800) >> 2
			| (addr & 0x00001000) >> 6
			| (addr & 0x00002000) >> 5
			| (addr & 0x00008000) << 2
			| (addr & 0x00010000) >> 5
			| (addr & 0x00020000) >> 2
			| (addr & 0xFFFC400D);
	}
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
	for(size_t i = 0; i < 32; i++) {
		u32 addr = scramble_addr(i, 8);
		u16 tmp = scramble_data(buf[i]);
		outbuf[addr] = tmp;
	}

	if(strncmp(outbuf, "Roland", 6)) {
		if(strncmp(outbuf, "JP-800", 6)) {
			// try again with 16bit
			for(size_t i = 0; i < 32; i++) {
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
	if(width == 16) {
		// performance improvement: scramble 16bit words
		// this saves half of the address scrambling operations
		u16* buf16 = (u16*) buf;
		u16* outbuf16 = (u16*) outbuf;
		for(size_t i = 0; i < fsize; i += 2) {
			u32 addr = scramble_addr(i, width);
			u16 tmp = scramble_data(buf16[i >> 1]);
			outbuf16[addr >> 1] = tmp;
		}
	} else {
		// no optimization for 8bit ROMs, because A[0] is scrambled too
		for(size_t i = 0; i < fsize; i++) {
			u32 addr = scramble_addr(i, width);
			u16 tmp = scramble_data(buf[i]);
			outbuf[addr] = tmp;
		}
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
