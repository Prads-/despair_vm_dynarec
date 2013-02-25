/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "memoryManager.h"

MemoryManager::MemoryManager(uint32 stackSize, uint32 dataSize, uint8 *codePtr, uint8 *globalDataPtr) {
	stackSpace = new (std::nothrow) uint8 [stackSize];
	dataSpace = new (std::nothrow) uint8 [dataSize];
	codeSpace = codePtr;
	globalDataSpace = globalDataPtr;
}

MemoryManager::~MemoryManager() {
	delete [] stackSpace;
	stackSpace = 0;
	delete [] dataSpace;
	dataSpace = 0;
}

uint64 MemoryManager::makeHeap(uint32 size) {
	uint8 *heap = new uint8[size];
	return (uint64)heap;
}

void MemoryManager::destroyHeap(uint64 ptr) {
	delete [] (uint8*)ptr;
}