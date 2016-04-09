/*
Copyright (c) 2016 Steven Arnow <s@rdw.se>
'hexwrite.c' - This file is part of ihextools

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


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


uint8_t *bin_to_hex_lut = (uint8_t *) "0123456789ABCDEF";
static uint32_t next_base;


static void _bin_to_hex(uint8_t bin, uint8_t *hex) {
	hex[0] = bin_to_hex_lut[(bin >> 4)];
	hex[1] = bin_to_hex_lut[(bin & 0xF)];
	return;
}


static void _hex_write_aligned(uint8_t *data, int bytes, FILE *out, int addr_low, int type) {
	int i;
	uint8_t hex[3];
	hex[2] = 0;
	uint8_t checksum = 0;

	fprintf(out, ":");
	checksum += bytes;
	_bin_to_hex(bytes, hex);
	fprintf(out, "%s", hex);

	_bin_to_hex((addr_low >> 8) & 0xFF, hex), fprintf(out, "%s", hex);
	_bin_to_hex(addr_low & 0xFF, hex), fprintf(out, "%s", hex);
	checksum += (addr_low & 0xFF) + ((addr_low >> 8) & 0xFF);
	_bin_to_hex(type & 0xFF, hex), fprintf(out, "%s", hex);
	checksum += (type & 0xFF);

	for (i = 0; i < bytes; i++) {
		_bin_to_hex(data[i], hex);
		fprintf(out, "%s", hex);
		checksum += data[i];
	}

	checksum = ~checksum, checksum++;
	_bin_to_hex(checksum, hex), fprintf(out, "%s\n", hex);
}


static void _write_file(FILE *out, uint32_t base, char *fname) {
	FILE *in;
	int chunk, read;
	uint8_t data[32];

	if (!(in = fopen(fname, "rb"))) {
		fprintf(stderr, "Unable to open file '%s'\n", fname);
		return;
	}

	for (; !feof(in); base += read) {
		chunk = 32 - (base & 0x1F);
		if ((next_base & 0xFFFF0000) != (base & 0xFFFF0000)) {
			uint8_t upper[2];
			upper[0] = base >> 24;
			upper[1] = base >> 16;
			_hex_write_aligned(upper, 2, out, 0, 4);
			next_base = base & 0xFFFF0000;
		}

		read = fread(data, 1, chunk, in);
		if (read)
			_hex_write_aligned(data, read, out, base & 0xFFFF, 0);
	}

	fclose(in);
}


void handle_one_argument(char *arg, FILE *out) {
	char *fname;
	uint32_t base;

	fname = malloc(strlen(arg) + 1);

	if (strstr(arg, "input@") == arg) {
		sscanf(arg, "input@%X=%s", &base, fname);
	} else if (strstr(arg, "input=") == arg) {
		sscanf(arg, "input=%s", fname);
		base = 0;
	} else if (strstr(arg, "entry_point=") == arg) {
		uint8_t addr[4];
		sscanf(arg, "entry_point=%X", &base);
		addr[0] = base >> 24;
		addr[1] = base >> 16;
		addr[2] = base >> 8;
		addr[3] = base;
		_hex_write_aligned(addr, 4, out, 0, 5);
		return;
	} else {
		fprintf(stderr, "Unhandled argument %s\n", arg);
		return;
	}

	_write_file(out, base, fname);
	free(fname);
	return;
}


int usage() {
	fprintf(stdout, "hexwrite - Converts one of more input binaries into intel HEX format\n");
	fprintf(stdout, "hexwrite <output file> [input argument] [input argument] ...\n");
	fprintf(stdout, "List of valid input arguments:\n");
	fprintf(stdout, "\tinput@addr=file - addr = hexadecimal address, file = file name\n");
	fprintf(stdout, "\t\teg. input@4FF0=arne.bin\n");
	fprintf(stdout, "\tinput=file - writes file to address 0\n");
	fprintf(stdout, "\t\teg. input=arne.bin\n");
	fprintf(stdout, "\tentry_point=addr\n");
	fprintf(stdout, "\t\teg. entry_point=deadbeef - Sets entry point to 0xDEADBEEF\n");

	return 1;
}


int main(int argc, char **argv) {
	FILE *out;
	int i;

	if (argc < 2)
		return usage();

	if (!(out = fopen(argv[1], "wb"))) {
		fprintf(stderr, "Unable to open output file %s\n", argv[1]);
		return 1;
	}

	for (i = 2; i < argc; i++)
		handle_one_argument(argv[i], out);

	_hex_write_aligned(NULL, 0, out, 0, 1); // End of file
	
	fclose(out);
	return 0;
}
