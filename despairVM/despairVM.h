/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef DESPAIR_VM_H
#define DESPAIR_VM_H

#include <string>
#include <cstdio>
#include "build.h"
#include "declarations.h"
#include "gpuCore.h"
#include "despairHeader.h"
#include "threadParameter.h"
#include "keyboardManager.h"

#define DPVM_START_UP_OK								0
#define DPVM_START_UP_ERROR_FILE_IO						1
#define DPVM_START_UP_ERROR_FILE_NOT_RECOGNIZED			2
#define DPVM_START_UP_ERROR_FILE_CORRUPTED				3
#define DPVM_START_UP_ERROR_BOOT_FAILED					4

//This is the fascade through which despairVM talks to outside world
class DespairVM {
private:
	GPUCore gpu;
	volatile bool mainThreadStopped;
	ThreadParameter threadParameter;
	uint8 *code, *globalData;
	DespairHeader::ExecutableHeader header;
	KeyboardManager keyboardManager;

public:
	DespairVM();
	~DespairVM();

	int startUpDespairVM(std::string path);	//Initialize despairVM and starts to execute the program
	bool checkMainThreadStatus();	//Checks the status of main thread of the program that the VM is running
	const uint32 *getGPUFrameBuffer();	//Gets frame buffer from GPU
	void setKeyboardKeyStatus(uint8 keyCode, bool status);	//Sets keyboard key status

	const DespairHeader::ExecutableHeader *getHeader();	//Gets header file of program
};

#endif