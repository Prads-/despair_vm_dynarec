/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef BOOT_MANAGER_H
#define BOOT_MANAGER_H

#include "build.h"
#include "declarations.h"
#include "despairHeader.h"

struct ThreadParameter;

namespace BootManager {
	bool bootUpDespair(uint8 *code, ThreadParameter *mainThreadParameter, DespairHeader::ExecutableHeader *header);
}

#endif