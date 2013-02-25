/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include <string>
#include "despairThreads.h"
#include "x86DynaRecCore.h"
#include "gpuCore.h"
using namespace DespairThreads;
using namespace std;

void DespairThreads::thread(void *arg) {
	ThreadParameter *params = (ThreadParameter*)arg;

	volatile bool *threadStopped = params->threadStopped;
	if (threadStopped) *threadStopped = false;

	X86DynaRecCore core(params->codePtr, params->globalDataPtr, params->codeStartIndex, params->paramAddr, params->gpuCore, params->header, params->keyboardManager);
	params->threadInitialized = true;
	
	core.startCPULoop();

	if (threadStopped) *threadStopped = true;
}