/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include <cstdlib>
#include "x86BinBlock.h"

template void X86BinBlock::write(uint8 val);
template void X86BinBlock::write(uint16 val);
template void X86BinBlock::write(uint32 val);
template void X86BinBlock::write(uint64 val);

X86BinBlock::X86BinBlock() {
	counter = 0;
	size = 10240;	//10 KB

#ifdef BUILD_FOR_UNIX
	binBlock = (uint8*)malloc(binBlockSize);
#endif

#ifdef BUILD_FOR_WINDOWS
	heapHandle = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
	binBlock = (uint8*)HeapAlloc(heapHandle, 0, size);
#endif
}

X86BinBlock::~X86BinBlock() {
#ifdef BUILD_FOR_UNIX
	free(binBlock);
#endif

#ifdef BUILD_FOR_WINDOWS
	HeapFree(heapHandle, 0, (void*)binBlock);
	HeapDestroy(heapHandle);
#endif
}

void X86BinBlock::checkBinBufferBoundary(int typeSize) {
	if (counter + typeSize >= size) {
		size += 10240; //Another 10 KB

#ifdef BUILD_FOR_UNIX
		binBlock = (uint8*)realloc((void*)binBlock, binBlockSize);
#endif

#ifdef BUILD_FOR_WINDOWS
		binBlock = (uint8*)HeapReAlloc(heapHandle, 0, (void*)binBlock, size);
#endif
	}
}

template<typename Type>
void X86BinBlock::write(Type val) {
	checkBinBufferBoundary(sizeof(Type));
	*(Type*)&binBlock[counter] = val;
	counter += sizeof(Type);
}

void X86BinBlock::writeAtIndex(uint32 val, uint32 index) {
	*(uint32*)&binBlock[index] = val;
}

uint8 *X86BinBlock::getBinBuffer() {
	return binBlock;
}

uint32 X86BinBlock::getCounter() {
	return counter;
}