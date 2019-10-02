#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#pragma pack(push, 1) /* removes padding from sructs */

int rng(const int min, const int max) {
    return (rand() % (max + 1 - min)) + min;
}

typedef struct rgbData {
	uint8_t b, g, r;
}pixel;

void generateBitmap(int width, int height, float dpi, const char* filename, pixel* imgData) {
	FILE* bitmap;

	struct fHeader {
		uint16_t type; 
		uint32_t size;
		uint16_t reserved1;
		uint16_t reserved2;
		uint32_t offset;
	}bmpFileHeader;

	struct iHeader {
		uint32_t headerSize;
		int32_t  width;
		int32_t  height;
		uint16_t planes;
		uint16_t bitCount;
		uint32_t compression;
		uint32_t imageSize; /* may be 0 if uncompressed */
		int32_t  xPPM;
		int32_t  yPPM;
		uint32_t colorEntriesUsed;
		uint32_t importantColors;
	}bmpImageHeader;
	
	int bytesPerPixel = 3; /* 24 bit color */
	uint32_t imgSize = width * height;
	uint32_t fileSize = sizeof(bmpFileHeader) + sizeof(bmpImageHeader) + (bytesPerPixel * width * height) + (height * 2);
	int32_t ppm = dpi * 39;
	
	bmpFileHeader.type = 0x4D42;
	bmpFileHeader.size = fileSize;
	bmpFileHeader.reserved1 = 0;
	bmpFileHeader.reserved2 = 0;
	bmpFileHeader.offset = (sizeof(bmpFileHeader) + sizeof(bmpImageHeader));

	bmpImageHeader.headerSize = sizeof(bmpImageHeader);
	bmpImageHeader.width = width;
	bmpImageHeader.height = height;
	bmpImageHeader.planes = 1;
	bmpImageHeader.bitCount = 8 * bytesPerPixel;
	bmpImageHeader.compression = 0;
	bmpImageHeader.imageSize = bytesPerPixel * height * width;
	bmpImageHeader.xPPM = ppm; /* typically set these to zero */
	bmpImageHeader.yPPM = ppm;
	bmpImageHeader.colorEntriesUsed = 0;
	bmpImageHeader.importantColors = 0;
	
	bitmap = fopen(filename, "wb");
	fwrite(&bmpFileHeader, 1, sizeof(bmpFileHeader), bitmap);
	fwrite(&bmpImageHeader, 1, sizeof(bmpImageHeader), bitmap);
	
	int i;
	int p;
	for (i = 0; i < (width * height); i++) {
		fwrite(&imgData[i], 3, sizeof(char), bitmap);
		if ((i + 1) % width == 0) { /* if at end of scanline */
			for(p = 0; p < (width % 4); p++) {
				fputc('\0', bitmap); /* append appropriate # of padding bits */
			}
		}
	}
	
	fclose(bitmap);
}

void generateMap(int xSize, int ySize, int max, int seaLevel, int grad) {
	char** randmap;
	char** map;
	int i;
	int x, y;
	int nXOffset, nYOffset;
	double sum;
	int sumIncs;
	int p = 0;
	pixel* imData;
	
	imData = (pixel*) malloc(xSize * ySize * sizeof(pixel));
	
	randmap = (char**) malloc(xSize * sizeof(char*));
	map = (char**) malloc(xSize * sizeof(char*));
	for (i = 0; i < ySize; i++) {
		map[i] = (char*) malloc(ySize * sizeof(char));
		randmap[i] = (char*) malloc(ySize * sizeof(char));
	}
	
	for (x = 0; x < xSize; x++) { /* assign random values */
		for (y = 0; y < ySize; y++) {
			randmap[x][y] = rng(0, max);
		}
	}
	
	for (x = 0; x < xSize; x++) { /* average pixels by neighbors */ 
		for (y = 0; y < ySize; y++) {
			sum = 0;
			sumIncs = 0;
			if ((x - grad >= 0) && (y - grad >= 0) && (x + grad < xSize) && (y + grad < ySize)) {
				for (nXOffset = -grad; nXOffset <= grad; nXOffset++) {
					for (nYOffset = -grad; nYOffset <= grad; nYOffset++) {
						sum = sum + randmap[x + nXOffset][y + nYOffset];
						sumIncs++;
					}
				}
			}
			map[x][y] = floor(sum / sumIncs);
			if (pow((x - xSize/2), 2) + pow((y - ySize/2), 2) < pow((xSize + ySize) / 4 - grad, 2)) {
				if (map[x][y] <= seaLevel) {
					imData[p].b = 128; /* draw ocean */
				}
				else {
					imData[p].g = map[x][y] * 20;
				}
			}
			else { /* fill in outside of planet circle with space + stars */
				if (rng(0, 100) == 1) {
					imData[p].r = 255;
					imData[p].g = 255;
					imData[p].b = 255;
				}
				else {
					imData[p].r = 0;
					imData[p].g = 0;
					imData[p].b = 0;
				}
			}
			p++;
		}
	}
	
	for (i = 0; i < 2; i++){
		for (p = 0; p < (xSize * ySize); p++) { /* fill lone pixels with ocean/land */
			if (imData[p].g != 0 && imData[p].g != 255) {
				if (imData[p - xSize].b == 128 && imData[p + xSize].b == 128) {
					imData[p].b = 128;
					imData[p].g = 0;
				}
				if (imData[p - 1].b == 128 && imData[p + 1].b == 128) {
					imData[p].b = 128;
					imData[p].g = 0;
				}
			}
			if (imData[p].b != 0 && imData[p].b != 255) {
				if (imData[p - 1].g != 0 && imData[p + 1].g !=0 && 
				imData[p - xSize].g != 0 && imData[p + xSize].g != 0 ) {
					imData[p].b = 0;
					imData[p].g = imData[p - 1].g;
				}
			}
		}
	}
	
	generateBitmap(xSize, ySize, 0, "map.bmp", imData);
	free(imData);
	
	for (i = 0; i < ySize; i++) {
		free(map[i]);
		free(randmap[i]);
	}
	free(randmap);
	free(map);
}

int main(void) {
	srand(time(0));
	generateMap(127, 127, 16, 7, 8);
	
	return 0;
}