/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef X86_BIN_BUFFER_H
#define X86_BIN_BUFFER_H

#include "build.h"
#include "declarations.h"
#ifdef BUILD_FOR_WINDOWS
#include <Windows.h>
#endif

class X86BinBlock {
private:
	uint8 *binBlock;
	int size, counter;
#ifdef BUILD_FOR_WINDOWS
	HANDLE heapHandle;
#endif

	void checkBinBufferBoundary(int size);

public:
	int64 startAddress, endAddress;

	X86BinBlock();
	~X86BinBlock();

	template<typename Type>
	void write(Type val);

	void writeAtIndex(uint32 val, uint32 index);

	uint8 *getBinBuffer();
	uint32 getCounter();
};

#endif