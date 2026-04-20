#include "IM.h"
#include "IWApiConstants.h"
#include <string>
#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#define headerLength 256
static FILE* filehandle[15];

int IMOpen(int istream, const char* name, const char* attrib) {
	if (istream < 0 || istream >= (int)(sizeof(filehandle) / sizeof(filehandle[0]))) {
		throw std::runtime_error("File index '" + std::to_string(istream) + "' is out of defined range 0-" + std::to_string(sizeof(filehandle) / sizeof(filehandle[0]) - 1) + "!");
	}
	if (!strcmp(attrib, "ro")) {
		FILE* file = fopen(name, "rb");
		if (file == NULL) {
			return -1;
		}
		filehandle[istream] = file;
		printf("\n RO image file on unit: %2d %s\n", istream, name);
		return 0;
	}
	else if (!strcmp(attrib, "new")) {
		FILE* file = fopen(name, "wb");
		if (file == NULL) {
			return -1;
		}
		filehandle[istream] = file;
		printf("\n NEW image file on unit: %2d %s\n", istream, name);
		return 0;
	}
	else {
		std::string mode(attrib);
		throw std::runtime_error("Open mode '" + mode + "' is not defined!");
	}
}

void IMClose(int istream) {
	if (filehandle[istream]) {
		fclose(filehandle[istream]);
		filehandle[istream] = NULL;
	}
}

void IMGetHdr(int istream, void* header) {
	fseek(filehandle[istream], 0, SEEK_SET);
	size_t numread = fread(header, sizeof(int32_t), headerLength, filehandle[istream]);
	(void)numread;
}

void IMPutHdr(int istream, const void* header) {
	fseek(filehandle[istream], 0, SEEK_SET);
	fwrite(header, sizeof(int32_t), headerLength, filehandle[istream]);
}

static int elementSizeForMode(int dataType) {
	switch (dataType) {
		case 0: return 1;  // uint8
		case 1: return 2;  // int16
		case 2: return 4;  // float32
		case 3: return 4;  // complex int16 (2x int16)
		case 4: return 8;  // complex float32 (2x float32)
		case 6: return 2;  // uint16
		default:
			throw std::runtime_error("Data type " + std::to_string(dataType) + " hasn't been implemented!");
	}
}

void IMRdSec(int istream, void* array, int64_t pixelNumb_Sec, int dataType) {
	int elemSize = elementSizeForMode(dataType);
	size_t nread = fread(array, elemSize, pixelNumb_Sec, filehandle[istream]);
	(void)nread;
}

void IMWrSec(int istream, const void* array, int64_t pixelNumb_Sec, int dataType) {
	int elemSize = elementSizeForMode(dataType);
	fseek(filehandle[istream], 0, SEEK_END);
	fwrite(array, elemSize, pixelNumb_Sec, filehandle[istream]);
}
