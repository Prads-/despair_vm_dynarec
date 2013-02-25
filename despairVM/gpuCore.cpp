/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include <cstring>
#include "gpuCore.h"

#define WIDTH_INDEX				0
#define HEIGHT_INDEX			1
#define IMAGE_DATA				5

void GPUCore::initializeGPU(int fWidth, int fHeight) {
	frameBuffer1 = new uint32[fWidth * fHeight];
	frameBuffer2 = new uint32[fWidth * fHeight];
	memset(frameBuffer1, 0xFF, fWidth * fHeight * 4);
	memset(frameBuffer2, 0xFF, fWidth * fHeight * 4);
	
	activeFrameBuffer = frameBuffer1;
	inactiveFrameBuffer = frameBuffer2;

	frameWidth = fWidth;
	frameHeight = fHeight;
	
	for (int i = 0; i < 360; ++i) {
		float radian = i * 3.14159265f / 180.0f;
		sinFixedPointTable[i] = sin(radian) * 65536;
		cosFixedPointTable[i] = cos(radian) * 65536;
	}
}

GPUCore::~GPUCore() {
	delete [] frameBuffer1;
	frameBuffer1 = 0;
	delete [] frameBuffer2;
	frameBuffer2 = 0;
	frameWidth = 0;
	frameHeight = 0;
}

void GPUCore::clearFrameBuffer(int bgColor) {
	bgColor &= 0xFFFFFF;
	uint32 frameSize = frameWidth * frameHeight;
	for (uint32 counter = 0; counter < frameSize; ++counter) {
		inactiveFrameBuffer[counter] = bgColor;
	}
}

void GPUCore::draw(int x, int y, uint64 address, int portGPU, int angle) {
	uint32 *imagePtr = (uint32*)address;
	
	if (portGPU & 0x10) {
		drawMonochorme(x, y, imagePtr);
	} else {
		//Checks to see if the image is inside or outside the view
		if (x > frameWidth) return;
		if (y > frameHeight) return;
		if (imagePtr[WIDTH_INDEX] + x < 0) return;
		if (imagePtr[HEIGHT_INDEX] + y < 0) return;

		bool rotation = portGPU & 1;
		bool alpha = portGPU & 2;
	
		if (rotation)
			drawRotation(x, y, imagePtr, alpha, portGPU, angle);
		else
			drawNoRotation(x, y, imagePtr, alpha, portGPU);
	}
}

void GPUCore::drawMonochorme(int x, int y, uint32 *imgPtr) {
	int imageHeight = imgPtr[HEIGHT_INDEX], imageWidth = imgPtr[WIDTH_INDEX];
	char *image = (char*)&imgPtr[IMAGE_DATA];

	if (x < 0 || x + imageWidth > frameHeight || y < 0 || y + imageHeight > frameHeight) {
		return;
	}
	
	imageHeight += y;
	imageWidth += x;
	int shiftValue = 128, imgCounter = 0;

	for (int yCounter = y; yCounter < imageHeight; ++yCounter) {
		y = yCounter * frameWidth;
		for (int xCounter = x; xCounter < imageWidth; ++xCounter) {
			if (image[imgCounter] & shiftValue) {
				inactiveFrameBuffer[y + xCounter] =  monochormeColor;
			}
			if (shiftValue == 1) {
				shiftValue = 128;
				++imgCounter;
			} else {
				shiftValue >>= 1;
			}
		}
	}
}

uint32 *GPUCore::getFrameBuffer() {
	return activeFrameBuffer;
}

void GPUCore::drawNoRotation(int x, int y, uint32 *imgPtr, bool alpha, int portGPU) {
	int fx, imageWidth = imgPtr[WIDTH_INDEX], imageHeight = imgPtr[HEIGHT_INDEX];
	int minX = 0, maxX = imageWidth, minY = 0, maxY = imageHeight;
	bool flipX = portGPU & 4;
	bool flipY = portGPU & 8;
	uint32 *image = &imgPtr[IMAGE_DATA];

	if (x + imageWidth > frameWidth) {
		int difference = x + imageWidth - frameWidth;
		(flipX) ? minX += difference : maxX -= difference;
	}
	if (x < 0) {
		(flipX) ? maxX += x : minX -= x;
		x = 0;
	}

	if (y + imageHeight > frameWidth) {
		int difference = y + imageHeight - frameHeight;
		(flipY) ? minY += difference : maxY -= difference;
	}
	if (y < 0) {
		(flipY) ? maxY += y : minY -= y;
		y = 0;
	}

	if (flipX) {
		x = x + maxX - minX - 1;
	}
	if (flipY) {
		y = y + maxY - minY - 1;
	}

	if (alpha) {
		for (int imgY = minY; imgY < maxY; ++imgY) {
			y *= frameWidth;
			fx = x;
			for (int imgX = minX; imgX < maxX; ++imgX) {
				uint32 pixel = image[imgX + imageWidth * imgY];
				if (pixel & 0xFFFFFF) {
					inactiveFrameBuffer[fx + y] = alphaBlend(image[imgX + imageWidth * imgY], inactiveFrameBuffer[fx + y]);
				}
				(flipX) ? --fx : ++fx;
			}
			y /= frameWidth;
			(flipY) ? --y : ++y;
		}
	} else {
		for (int imgY = minY; imgY < maxY; ++imgY) {
			y *= frameWidth;
			fx = x;
			for (int imgX = minX; imgX < maxX; ++imgX) {
				uint32 pixel = image[imgX + imageWidth * imgY];
				if (pixel & 0xFFFFFF) {
					inactiveFrameBuffer[fx + y] = image[imgX + imageWidth * imgY];
				}
				(flipX) ? --fx : ++fx;
			}
			y /= frameWidth;
			(flipY) ? --y : ++y;
		}
	}
}

void GPUCore::drawRotation(int x, int y, uint32 *imgPtr, bool alpha, int portGPU, int angle) {
	int imageWidth = imgPtr[WIDTH_INDEX], imageHeight = imgPtr[HEIGHT_INDEX];
	bool flipX = portGPU & 4;
	bool flipY = portGPU & 8;

	int sinA = sinFixedPointTable[angle % 360], cosA = cosFixedPointTable[angle % 360];
	int diagonalLength = sqrt((float)(imageWidth * imageWidth) + (float)(imageHeight * imageHeight));
	int midLength = diagonalLength >> 1;
	int minX = ~midLength + 1, maxX = midLength, minY = ~midLength + 1, maxY = midLength;
	int fixedPointMidWidth = imageWidth << 15, fixedPointMidHeight = imageHeight << 15; 
	int rX, rY;
	uint32 *image = &imgPtr[IMAGE_DATA];

	//Making sure image will be in right coordinates
	x = x - ((diagonalLength - imageWidth) >> 1);
	y = y - ((diagonalLength - imageHeight) >> 1);

	//Cancel the part that is outside the view - right side of the screen
	if (diagonalLength + x > frameWidth) { 
		int difference = diagonalLength + x - frameWidth;
		(flipX) ? minX += difference : maxX -= difference;
	}
	//Cancel the part that is outside the view - lower side of the screen
	if (diagonalLength + y > frameHeight) {
		int difference = diagonalLength + y - frameHeight;
		(flipY) ? minY += difference : maxY -= difference;
	}

	if (x < 0) {	//If the starting point (x) is outside the view - left side of the screen
		(flipX) ? maxX += x : minX -= x;	//Cancel the part outside view
		x = 0;
	}
	if (y < 0) {	//If the starting point (y) is outside the view - upper side of the screen
		(flipY) ? maxY += y : minY -= y;	//Cancel the part outside view
		y = 0;
	}

	//If there are flips, point to the data at the end
	if (flipX) {
		x = x + maxX - minX - 1;
	}
	if (flipY) {
		y = y + maxY - minY - 1;
	}

	//Loop that draws to frame buffer
	if (alpha) {
		for (int imgY = minY; imgY < maxY; ++imgY) {
			for (int imgX = minX, fx = x; imgX < maxX; ++imgX) {
				rX = (imgX * cosA + imgY * sinA + fixedPointMidWidth) >> 16;
				rY = (imgY * cosA - imgX * sinA + fixedPointMidHeight) >> 16;
				if (rX >= 0 && rX < imageWidth && rY >= 0 && rY < imageHeight) {
					int pixel = image[rX + imageWidth * rY];
					if (pixel & 0xFFFFFF) {
						inactiveFrameBuffer[fx + frameHeight * y] = alphaBlend(image[rX + imageWidth * rY], inactiveFrameBuffer[fx + frameHeight * y]);
					}
				}
				(flipX) ? --fx : ++fx;
			}
			(flipY) ? --y : ++y;
		}
	} else {
		for (int imgY = minY; imgY < maxY; ++imgY) {
			for (int imgX = minX, fx = x; imgX < maxX; ++imgX) {
				rX = (imgX * cosA + imgY * sinA + fixedPointMidWidth) >> 16;
				rY = (imgY * cosA - imgX * sinA + fixedPointMidHeight) >> 16;
				if (rX >= 0 && rX < imageWidth && rY >= 0 && rY < imageHeight) {
					int pixel = image[rX + imageWidth * rY];
					if (pixel & 0xFFFFFF) {
						inactiveFrameBuffer[fx + frameHeight * y] = image[rX + imageWidth * rY];
					}
				}
				(flipX) ? --fx : ++fx;
			}
			(flipY) ? --y : ++y;
		}
	}
}

uint32 GPUCore::alphaBlend(uint32 src, uint32 dest) {
	int srb = src & 0xFF00FF, sg = src & 0xFF00;
	int drb = dest & 0xFF00FF, dg = dest & 0xFF00;
	int sa = ((float)(src >> 24) / 255) * 256;

	drb = (sa * (srb - drb) >> 8) + drb;
	dg = (sa * (sg - dg) >> 8) + dg;

	return (drb & 0xFF00FF) | (dg & 0xFF00);
}

void GPUCore::presentBuffer() {
	uint32 *temp = activeFrameBuffer;
	activeFrameBuffer = inactiveFrameBuffer;
	inactiveFrameBuffer = temp;
}

void GPUCore::gpuDecodeCommand(uint32 cmd) {
	if (cmd & 0xFF000000) {
		if ((cmd & 0xFFFFFF) == 0) {
			presentBuffer();
		} else if ((cmd & 0xFF000000) == 0x1000000) {
			monochormeColor = cmd & 0xFFFFFF;
		}
	} else {
		clearFrameBuffer(cmd);
	}
}

void GPUCore::gpuDMA_In(uint32 *ptr) {
	memcpy(inactiveFrameBuffer, ptr, sizeof(uint32) * frameWidth * frameHeight);
}

void GPUCore::gpuDMA_Out(uint32 *ptr) {
	memcpy(ptr, inactiveFrameBuffer, sizeof(uint32) * frameWidth * frameHeight);
}