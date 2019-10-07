#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#pragma pack(push, 1) /* removes padding from structs */

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

double averageOfNeighbors (char** array, int xSize, int ySize, int x, int y, char r) {
	int xOffset, yOffset;
	double sum = 0;
	int members = 0;
	
	if ((x - r >= 0) && (y - r >= 0) && (x + r < xSize) && (y + r < ySize)) {
		for (xOffset = -r; xOffset <= r; xOffset++) {
			for (yOffset = -r; yOffset <= r; yOffset++) {
				sum = sum + array[x + xOffset][y + yOffset];
				members++;
			}
		}
	}
	return (sum / members);
}

void generateMap(char* filename, int size, char max, char seaLevel, char grad) {
	char** randmap;
	char** cloudLayer;
	int i;
	int x, y;
	int avg;
	int p = 0;
	pixel* imData;
	
	imData = (pixel*) malloc(pow(size, 2) * sizeof(pixel));
	
	randmap = (char**) malloc(size* sizeof(char*));
	for (i = 0; i < size; i++) {
		randmap[i] = (char*) malloc(size * sizeof(char));
	}
	
	for (x = 0; x < size; x++) {
		for (y = 0; y < size; y++) {
			randmap[x][y] = rng(0, max);
		}
	}
	
	for (x = 0; x < size; x++) {
		for (y = 0; y < size; y++) {
			
			avg = floor(averageOfNeighbors(randmap, size, size, x, y, grad));
			
			if (pow((x - size / 2), 2) + pow((y - size / 2), 2) < pow(size / 2 - grad, 2)) {
				if (avg <= seaLevel) {
					imData[p].b = 128; /* draw ocean */
				}
				else {
					imData[p].g = 160 + (20 * (avg - (max / 2))); /* draw land */
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
		for (p = 0; p < pow(size, 2); p++) { /* fill lone pixels with ocean/land */
			if (imData[p].g != 0 && imData[p].g != 255) {
				if (imData[p - size].b == 128 && imData[p + size].b == 128) {
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
				imData[p - size].g != 0 && imData[p + size].g != 0 ) {
					imData[p].b = 0;
					imData[p].g = imData[p - 1].g;
				}
			}
		}
	}
	
	generateBitmap(size, size, 0, filename, imData);
	free(imData);
	
	for (i = 0; i < size; i++) {
		free(randmap[i]);
	}
	free(randmap);
}

void overlayClouds(char* filename, int size, char cloudMax, char cloudLevel, char cloudGrad) {
	char** randClouds;
	unsigned char pixelData[3]; /* b, g, r */
	int i;
	int x, y;
	char colors[3];
	char cloudCleared;
	
	FILE* f;
	f = fopen(filename, "rb+");
	
	randClouds = (char**) malloc(pow(size, 2) * sizeof(char*));
	for (i = 0; i < size; i++) {
		randClouds[i] = (char*) malloc(size * sizeof(char));
	}
	
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			randClouds[x][y] = rng(0, cloudMax);
		}
	}
	
	fseek(f, 54, SEEK_SET); /* move pointer past header */
	
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++) {
			fread(pixelData, 3, 1, f);
			if (pixelData[0] == 128 || (pixelData[1] != 0 && pixelData[1] != 255)) { /* if over planet */
				cloudCleared = 0;
				
				if (pixelData[2] == 220) { /* clear previous clouds */
					if (pixelData[0] == 220) {
						colors[2] = 0;
						colors[1] = pixelData[1] - 75;
						colors[0] = 0;
					}
					else if (pixelData[1] == 220) {
						colors[2] = 0;
						colors[1] = 0;
						colors[0] = 128;
					}
					fseek(f, -3, SEEK_CUR);
					fwrite(colors, 3, 1, f);
					cloudCleared = 1;
				}
				
				if (floor(averageOfNeighbors(randClouds, size, size, x, y, cloudGrad)) > cloudLevel) {
					if (cloudCleared == 1) {
						fseek(f, -3, SEEK_CUR); /* get new img data as opposed to the old */
						fread(pixelData, 3, 1, f);
					}
					
					if (pixelData[1] != 0) {
						colors[2] = 220;
						colors[1] = pixelData[1] + 75;
						colors[0] = 220;
					}
					else {
						colors[2] = 220;
						colors[1] = 220;
						colors[0] = 235;
					}
					
					fseek(f, -3, SEEK_CUR);
					fwrite(colors, 3, 1, f);
				}
			}
		}
		fseek(f, (size % 4), SEEK_CUR); /* skip padding bytes */
	}
	for (i = 0; i < size; i++) {
		free(randClouds[i]);
	}
	free(randClouds);
	fclose(f);
}

int main(int argc, char** argv) {
	srand(time(0));
	//generateMap("map.bmp", 127, 16, 7, 8);
	//overlayClouds("map.bmp", 127, 16, 9, 3);
	
	generateMap("map.bmp", atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	overlayClouds("map.bmp", atoi(argv[1]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
	return 0;
}