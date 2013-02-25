/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef MEMORY_DMA_CONTROLLER_H
#define MEMORY_DMA_CONTROLLER_H

#include <cstring>
#include "build.h"
#include "declarations.h"

#define MEMORY_DMA_CMD_PTR_PTR		0
#define MEMORY_DMA_CMD_PTR_MEM		1
#define MEMORY_DMA_CMD_MEM_PTR		2
#define MEMORY_DMA_CMD_MEM_MEM		3

namespace MemoryDMAController {
	void memoryDMATransfer(uint32 size, uint8 *ports);
}

#endif