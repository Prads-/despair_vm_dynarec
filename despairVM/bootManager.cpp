/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include <cstring>
#include "BootManager.h"
#include "memoryManager.h"
#include "threadManager.h"
#include "despairHeader.h"
#include "threadParameter.h"
using namespace ThreadManager;
using namespace DespairHeader;

bool BootManager::bootUpDespair(uint8 *code, ThreadParameter *mainThreadParameter, ExecutableHeader *header) {
	memcpy(mainThreadParameter->globalDataPtr, header->part2.globalPreData, header->part2.globalPreDataSize);
	return createNewThread(mainThreadParameter);
}

