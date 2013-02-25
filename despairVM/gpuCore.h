/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef GPU_CORE_H
#define GPU_CORE_H

#include <cmath>
#include "build.h"
#include "declarations.h"

class GPUCore {
private:
	uint32 *frameBuffer1, *frameBuffer2;
	uint32 *activeFrameBuffer, *inactiveFrameBuffer;
	int frameWidth, frameHeight;
	int32 sinFixedPointTable[360], cosFixedPointTable[360];
	int32 monochormeColor;

	//Drawing Routines
	void drawNoRotation(int x, int y, uint32 *imgPtr, bool alpha, int portGPU);
	void drawRotation(int x, int y, uint32 *imgPtr, bool alpha, int portGPU, int angle);
	void drawMonochorme(int x, int y, uint32 *imgPtr);
	uint32 alphaBlend(uint32 src, uint32 dest);
public:
	~GPUCore();

	void initializeGPU(int fWidth, int fHeight);
	void clearFrameBuffer(int bgColor);
	void draw(int x, int y, uint64 address, int portGPU, int angle);
	uint32 *getFrameBuffer();
	void presentBuffer();
	void gpuDecodeCommand(uint32 cmd);
	void gpuDMA_In(uint32 *ptr);
	void gpuDMA_Out(uint32 *ptr);
};

#endif