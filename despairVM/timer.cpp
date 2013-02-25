/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "timer.h"
#ifdef BUILD_FOR_WINDOWS
#include <windows.h>
#endif
#ifdef BUILD_FOR_UNIX
#include <sys/time.h>
#endif

DespairTimer::DespairTimer() {
#ifdef BUILD_FOR_WINDOWS
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	frequency = freq.QuadPart / 1000;
#endif
	milliSeconds = 0;
}

uint64 DespairTimer::getMilliseconds(DespairTimer *timer) {
#ifdef BUILD_FOR_WINDOWS
	static LARGE_INTEGER prevTicks = {0};
	LARGE_INTEGER currentTicks;

	QueryPerformanceCounter(&currentTicks);
	if ((currentTicks.QuadPart - prevTicks.QuadPart) > timer->frequency) {
		timer->milliSeconds += (currentTicks.QuadPart - prevTicks.QuadPart) / timer->frequency;
		prevTicks.QuadPart = currentTicks.QuadPart - (currentTicks.QuadPart - prevTicks.QuadPart) % timer->frequency;
	}
#endif
#ifdef BUILD_FOR_UNIX
    timeval tv;
    gettimeofday(&tv, 0);
    milliSeconds = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

	return timer->milliSeconds;
}