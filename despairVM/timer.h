/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef DESPAIR_TIMER_H
#define DESPAIR_TIMER_H

#ifdef BUILD_FOR_WINDOWS
#include <windows.h>
#endif

#include "declarations.h"
#include "build.h"

class DespairTimer {
private:
	uint32 milliSeconds;
#ifdef BUILD_FOR_WINDOWS
	uint64 frequency;
#endif
public:
	DespairTimer();
	static uint64 getMilliseconds(DespairTimer *timer);
};

#endif