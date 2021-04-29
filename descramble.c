#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

u8 descramble_data8(u8 word)
{
	return    (word & 0x0002) << 6
		| (word & 0x0008) << 3
		| (word & 0x0040) >> 1
		| (word & 0x0080) >> 3
		| (word & 0x0020) >> 2
		| (word & 0x0010) >> 2
		| (word & 0x0001) << 1
		| (word & 0x0004) >> 2;
}

u16 descramble_data16(u16 word)
{
	return    (word & 0x0002) << 6
		| (word & 0x0008) << 3
		| (word & 0x0040) >> 1
		| (word & 0x0080) >> 3
		| (word & 0x0020) >> 2
		| (word & 0x0010) >> 2
		| (word & 0x0001) << 1
		| (word & 0x0004) >> 2
		| (word & 0x0200) << 6
		| (word & 0x0800) << 3
		| (word & 0x4000) >> 1
		| (word & 0x8000) >> 3
		| (word & 0x2000) >> 2
		| (word & 0x1000) >> 2
		| (word & 0x0100) << 1
		| (word & 0x0400) >> 2;
}

u32 descramble_addr(u32 addr, int width)
{
	if(width == 8) {
		return    (addr & 0x00000001) << 1
			| (addr & 0x00000002) << 3
			| (addr & 0x00000004) >> 2
			| (addr & 0x00000008) >> 1
			| (addr & 0x00000010) >> 1
			| (addr & 0x00000020) << 10
			| (addr & 0x00000040) << 4
			| (addr & 0x00000080) << 10
			| (addr & 0x00000100) << 6
			| (addr & 0x00000200) >> 4
			| (addr & 0x00000400) >> 3
			| (addr & 0x00000800) << 1
			| (addr & 0x00001000) << 4
			| (addr & 0x00002000) >> 7
			| (addr & 0x00004000) << 4
			| (addr & 0x00008000) >> 4
			| (addr & 0x00010000) >> 3
			| (addr & 0x00020000) >> 8
			| (addr & 0x00040000) >> 10
			| (addr & 0xFFF80000);
	} else {
		return    (addr & 0x00000002) << 3
			| (addr & 0x00000010) >> 3
			| (addr & 0x00000020) << 3
			| (addr & 0x00000040) << 6
			| (addr & 0x00000080) >> 1
			| (addr & 0x00000100) << 5
			| (addr & 0x00000200) << 2
			| (addr & 0x00000400) >> 1
			| (addr & 0x00000800) << 5
			| (addr & 0x00001000) >> 5
			| (addr & 0x00002000) >> 8
			| (addr & 0x00008000) << 2
			| (addr & 0x00010000) >> 6
			| (addr & 0x00020000) >> 2
			| (addr & 0xFFFC400D);
	}
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
	if(width == 16) {
		// performance improvement: descramble 16bit words
		// this saves half of the address scrambling operations
		u16* buf16 = (u16*) buf;
		u16* outbuf16 = (u16*) outbuf;
		for(size_t i = 0; i < fsize; i += 2) {
			u32 addr = descramble_addr(i, width);
			u16 tmp = descramble_data16(buf16[i >> 1]);
			outbuf16[addr >> 1] = tmp;
		}
	} else {
		// no optimization for 8bit ROMs, because A[0] is scrambled too
		for(size_t i = 0; i < fsize; i++) {
			u32 addr = descramble_addr(i, width);
			u16 tmp = descramble_data8(buf[i]);
			outbuf[addr] = tmp;
		}
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
