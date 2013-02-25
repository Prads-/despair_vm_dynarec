/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "memoryDMAController.h"
#include "memoryManager.h"
#include "portAddress.h"

void MemoryDMAController::memoryDMATransfer(uint32 size, uint8 *ports) {
	uint64 src = *(uint64*)&ports[PORT_DMA_ADDR1], dest = *(uint64*)&ports[PORT_DMA_ADDR2];
	memcpy((void*)dest, (void*)src, size);
}