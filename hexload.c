/*
Copyright (c) 2016 Steven Arnow <s@rdw.se>
'hexload.c' - This file is part of ihextools

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	distribution.
*/

//#include <stdint.h>
#ifdef HIGHLEVEL
#include <stdio.h>
#endif

#define	uint8_t unsigned char
#define	uint32_t unsigned int
#define	uint16_t unsigned short

/* XXX: This loader will only work on 32 bit systems! */

enum HexloadState {
	HEXLOAD_STATE_INIT,
	HEXLOAD_STATE_BYTE,
	HEXLOAD_STATE_ADDRESS,
	HEXLOAD_STATE_TYPE,
	HEXLOAD_STATE_DATA,
	HEXLOAD_STATE_CHECKSUM,
	HEXLOAD_STATE_SKIP,
};


#ifdef HIGHLEVEL
uint8_t buff[384000];
#endif


#ifndef HIGHLEVEL
extern inline uint8_t fetch_byte() {
	while(!(*((volatile uint32_t *) 0x100004) & 0x2));
	
	return *((volatile uint32_t *) 0x100000);
}
#else
static uint8_t fetch_byte() {
	return getc(stdin);
}
#endif


void loadhex() {
	enum HexloadState state = HEXLOAD_STATE_INIT;
	void *entry_point = (void *) 0x10000;
	uint8_t *next = (void *) 0x0;
	uint8_t byte;
	int count;
	uint8_t decoded;
	int bytes_to_read;
	uint16_t addr = 0;
	uint8_t type = 0;


	for (count = 0;; count++) {
		switch (state) {
			case HEXLOAD_STATE_INIT:
				if ((byte = fetch_byte()) != ':') {
					if (byte == '\n')
						continue;
					state = HEXLOAD_STATE_SKIP;
					continue;
				} else {
					state = HEXLOAD_STATE_BYTE;
				}
					
				break;
			case HEXLOAD_STATE_BYTE:
				bytes_to_read = decoded;
				count = 0;
				state = HEXLOAD_STATE_ADDRESS;
				break;
			case HEXLOAD_STATE_ADDRESS:
				if (count == 1)
					addr = decoded << 8;
				else {
					addr |= decoded;
					state = HEXLOAD_STATE_TYPE;
				}

				break;
			case HEXLOAD_STATE_TYPE:
				type = decoded;
				state = HEXLOAD_STATE_DATA;
				count = 0;
				break;
			case HEXLOAD_STATE_DATA:
				if (type == 0)
					next[addr] = decoded, addr++;
				else if (type == 1) {
					goto load_done;
				} else if (type == 4) {
					if (count == 1)
						next = 0, next += (decoded << 24);
					else
						next += (decoded << 16);
				} else if (type == 5) {
					if (count == 1)
						entry_point = 0;
					entry_point += (decoded << ((4 - count) << 3));
				} else
					goto error;
				if (count == bytes_to_read)
					state = HEXLOAD_STATE_CHECKSUM;
				break;
			case HEXLOAD_STATE_CHECKSUM:
				//TODO: Check checksum
				state = HEXLOAD_STATE_SKIP;
				continue;
				break;
			case HEXLOAD_STATE_SKIP:
				if (((byte = fetch_byte())) == '\n')
					state = HEXLOAD_STATE_INIT;
					continue;
				continue;
				break;
		}

		/* Decode a byte */
		byte = fetch_byte();
		if (byte > '9')
			decoded = (byte - 0x37);
		else
			decoded = byte - 0x30;
		decoded <<= 4;
		byte = fetch_byte();
		if (byte > '9')
			decoded |= (byte - 0x37);
		else
			decoded |= byte - 0x30;
	}

#ifndef HIGHLEVEL
	load_done:
	/* Jump to entry point */
	goto *entry_point;

	error:
	(void) *buff;
	for (;;);
#else
	load_done:
	error:
	(void) 0;
#endif
}


#ifdef HIGHLEVEL
int main(int argc, char **argv) {
	int i;
	FILE *fp;
	fprintf(stderr, "%p\n", buff);
	loadhex();
	fp = fopen("/tmp/arne.bin", "w");

	for (i = 0; i < 384000; i++) {
		fputc(buff[i], fp);
	}
}
#endif
