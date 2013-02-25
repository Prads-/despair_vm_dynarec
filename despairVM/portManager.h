/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef PORT_MANAGER_H
#define PORT_MANAGER_H

#include <string>
#include <cstring>
#include "build.h"
#include "declarations.h"
#include "portAddress.h"
#include "gpuCore.h"
#include "keyboardManager.h"
#include "despairHeader.h"

#define PORTS_NUMBER				256

class PortManager {
private:
	uint8 ports[PORTS_NUMBER];
	GPUCore *gpuCore;
	uint8 *codePtr, *globalDataPtr;
	DespairHeader::ExecutableHeader *header;
	KeyboardManager *keyboardManager;

public:
	void initializePortManager(GPUCore *gpuCore, uint8 *codePtr, uint8 *globalDataPtr, DespairHeader::ExecutableHeader *header, KeyboardManager *keyboardManager);
	void initializePorts();

	template<typename Type>
	static Type readPort(uint32 address, PortManager *pM);

	template<typename Type>
	static void writePort(Type val, uint32 address, PortManager *pM);

	static float32 readPortAsFloat(uint32 address, PortManager *pM);
	static void writePortAsFloat(float32 val, uint32 address, PortManager *pM);
};

#endif