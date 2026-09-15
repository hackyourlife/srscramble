#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

static inline u8 descramble_data8(const u8 word)
{
	return    (word & 0x0002) << 6
		| (word & 0x0008) << 3
		| (word & 0x0040) >> 1
		| (word & 0x0080) >> 3
		| (word & 0x0034) >> 2
		| (word & 0x0001) << 1;
}

static inline u16 descramble_data16(const u16 word)
{
	return    (word & 0x0202) << 6
		| (word & 0x0808) << 3
		| (word & 0x4040) >> 1
		| (word & 0x8080) >> 3
		| (word & 0x3434) >> 2
		| (word & 0x0101) << 1;
}

static inline u32 descramble_addr8(const u32 addr)
{
	return    (addr & 0x00000801) << 1
		| (addr & 0x00000002) << 3
		| (addr & 0x00000004) >> 2
		| (addr & 0x00000018) >> 1
		| (addr & 0x000000A0) << 10
		| (addr & 0x00005040) << 4
		| (addr & 0x00000100) << 6
		| (addr & 0x00008200) >> 4
		| (addr & 0x00010400) >> 3
		| (addr & 0x00002000) >> 7
		| (addr & 0x00020000) >> 8
		| (addr & 0x00040000) >> 10
		| (addr & 0xFFF80000);
}

static inline u32 descramble_addr16(const u32 addr)
{
	return    (addr & 0x00000022) << 3
		| (addr & 0x00000010) >> 3
		| (addr & 0x00000040) << 6
		| (addr & 0x00000480) >> 1
		| (addr & 0x00000900) << 5
		| (addr & 0x00008200) << 2
		| (addr & 0x00001000) >> 5
		| (addr & 0x00002000) >> 8
		| (addr & 0x00010000) >> 6
		| (addr & 0x00020000) >> 2
		| (addr & 0xFFFC400D);
}

int main(int argc, char** argv)
{
	if(argc != 3) {
		printf("Usage: %s scrambled.bin descrambled.bin\n", *argv);
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
			if(strncmp(buf, "RK-10E", 6)) {
				printf("Invalid ROM: %c%c%c%c%c%c\n", buf[0], buf[1],
						buf[2], buf[3], buf[4], buf[5]);
				return 1;
			} else {
				width = 8;
				type = "JD-990";
			}
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
	} else if(!strncmp(&buf[0x7], "JV80", 4)) {
		width = 8;
		type = "JV880";
	} else if(!strncmp(&buf[0x7], "XP-GS", 5)) {
		width = 16;
		type = "SC88";
	} else {
		printf("Unknown ROM type: ");
		for(int i = 0; i < 16; i++) {
			printf("%c", buf[i]);
		}
		printf("\n");
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
			u32 addr = descramble_addr16(i);
			u16 tmp = descramble_data16(buf16[i >> 1]);
			outbuf16[addr >> 1] = tmp;
		}
	} else {
		// no optimization for 8bit ROMs, because A[0] is scrambled too
		for(size_t i = 0; i < fsize; i++) {
			u32 addr = descramble_addr8(i);
			u16 tmp = descramble_data8(buf[i]);
			outbuf[addr] = tmp;
		}
	}
	//////////////////////////////////////////////

	// print ROM info
	if(width == 8) {
		// SR-JV80 / JV880 / JP-800 / JD-990
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
		// SRX / SC88
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
