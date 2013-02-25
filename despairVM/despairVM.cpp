/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "despairVM.h"
#include "memoryManager.h"
#include "sha256.h"
#include "bootManager.h"
#include "despairThreads.h"
#include "gpuCore.h"
using namespace std;
using namespace DespairHeader;
using namespace SHA256;
using namespace BootManager;

DespairVM::DespairVM() {
	code = 0;
	globalData = 0;
}

DespairVM::~DespairVM() {
	delete [] code;
	code = 0;
	delete [] globalData;
	globalData = 0;
}

int DespairVM::startUpDespairVM(std::string path) {
	string exeFolder;
	int exeFolderSize;
	for (exeFolderSize = path.size() - 1; exeFolderSize >= 0; --exeFolderSize) {
		if (path[exeFolderSize] == '/') {
			break;
		}
	}
	if (exeFolderSize == -1) {
		exeFolder = "";
	} else {
		exeFolder = path.substr(0, exeFolderSize + 1);
	}

	FILE *exeFile;

	exeFile = fopen(path.c_str(), "rb");
	if (exeFile == 0) {
		return DPVM_START_UP_ERROR_FILE_IO;
	}

	//Magic number check
	getMagicNumber(&header, &exeFile);
	if (header.part1.magicNumber != 0x4450564D) {
		fclose(exeFile);
		return DPVM_START_UP_ERROR_FILE_NOT_RECOGNIZED;
	}

	getHeaderInfo(&header, &exeFile, exeFolder);
	
	//Code size check
	fseek(exeFile, 0, SEEK_END);
	int exeFileSize = ftell(exeFile);
	if ((exeFileSize - header.part1.headerSize) < header.part1.codeSize) {
		fclose(exeFile);
		return DPVM_START_UP_ERROR_FILE_CORRUPTED;
	}
	
	//SHA-256 signature check
	code = new uint8[header.part1.codeSize];
	fseek(exeFile, header.part1.headerSize, SEEK_SET);
	fread(code, 1, header.part1.codeSize, exeFile);	//Here we read our binary code
	fclose(exeFile);

	SHA_256_MessageDigest signature = sha256(code, header.part1.codeSize);
	if (memcmp(&signature.h, &header.part1.signature, 32)) {
		return DPVM_START_UP_ERROR_FILE_CORRUPTED;
	}

	//If all checks pass, boot up DespairVM
	gpu.initializeGPU(header.part1.frameBufferWidth, header.part1.frameBufferHeight);
	globalData = new uint8[header.part1.globalDataSize];

	threadParameter.threadStopped = &mainThreadStopped;
	threadParameter.codeStartIndex = header.part1.codeOffset;
	threadParameter.paramAddr = 0;
	threadParameter.codePtr = code;
	threadParameter.globalDataPtr = globalData;
	threadParameter.gpuCore = &gpu;
	threadParameter.keyboardManager = &keyboardManager;
	threadParameter.header = &header;
	
	if (!bootUpDespair(code, &threadParameter, &header)) {
		return DPVM_START_UP_ERROR_BOOT_FAILED;
	}

	return DPVM_START_UP_OK;
}

bool DespairVM::checkMainThreadStatus() {
	return !mainThreadStopped;
}

const uint32 *DespairVM::getGPUFrameBuffer() {
	return gpu.getFrameBuffer();
}

void DespairVM::setKeyboardKeyStatus(uint8 keyCode, bool status) {
	keyboardManager.setKeyStatus(keyCode, status);	
}

const ExecutableHeader *DespairVM::getHeader() {
	return &header;
}