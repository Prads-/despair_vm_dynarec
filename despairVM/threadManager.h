/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "build.h"
#ifdef BUILD_FOR_WINDOWS
#include <process.h>
#endif
#ifdef BUILD_FOR_UNIX
#include <pthread.h>
#endif
#include "declarations.h"

struct ThreadParameter;

namespace ThreadManager {
	bool createNewThread(ThreadParameter *params);
}

#endif