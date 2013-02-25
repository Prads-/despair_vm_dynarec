/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "threadManager.h"
#include "despairThreads.h"
using namespace DespairThreads;

bool ThreadManager::createNewThread(ThreadParameter *params) {
#ifdef BUILD_FOR_WINDOWS
	if (_beginthread(thread, 0, (void*)params) == -1) return false;
#endif

#ifdef BUILD_FOR_UNIX
	pthread_t t;
	if (pthread_create(&t, 0, (void *(*)(void*))thread, (void*)params)) return false;
#endif

	while (!params->threadInitialized);

	return true;
}