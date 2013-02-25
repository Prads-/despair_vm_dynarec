/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <new>
#include "build.h"
#include "declarations.h"

class MemoryManager {
public:
	uint8 *stackSpace, *dataSpace;
	uint8 *codeSpace, *globalDataSpace;

	MemoryManager(uint32 stackSize, uint32 dataSize, uint8 *codePtr, uint8 *globalDataPtr);
	~MemoryManager();

	static uint64 makeHeap(uint32 size);
	static void destroyHeap(uint64 ptr);
};

#endif