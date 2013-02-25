/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "build.h"
#include "declarations.h"

#ifdef BUILD_FOR_WINDOWS
#include <Windows.h>
#endif

#include <vector>
#include <cmath>
#include <time.h>
#include "x86DynaRecCore.h"
#include "x86_64Emitter.h"
#include "instructionsSet.h"
using namespace X86_64Emitter;
using namespace std;

#define FD_CYCLE_CONTINUE			1
#define FD_CYCLE_BREAK_MASK			0x80
#define FD_CYCLE_BREAK_JMP			0x80
#define FD_CYCLE_BREAK_JMPR			0x81
#define FD_CYCLE_BREAK_JC			0x82
#define FD_CYCLE_BREAK_JCR			0x83
#define FD_CYCLE_BREAK_CALL			0x84
#define FD_CYCLE_BREAK_RET			0x85
#define FD_CYCLE_END				0x86

DespairTimer X86DynaRecCore::timer;

X86DynaRecCore::X86DynaRecCore(uint8 *codePtr, uint8 *globalDataPtr, uint32 codeStartIndex, uint64 paramAddr, GPUCore *gpuCore, DespairHeader::ExecutableHeader *header, KeyboardManager *keyboardManager)
					: memManager(header->part1.stackSize, header->part1.dataSize, codePtr, globalDataPtr) {
	regs[0xFF] = (uint64)memManager.dataSpace;
	regs[0xFE] = (uint64)memManager.globalDataSpace;
	if (paramAddr != 0) *(uint64*)&memManager.dataSpace[0] = paramAddr;
	sP = 0;
	pC = codeStartIndex;
	this->gpuCore = gpuCore;
	portManager.initializePortManager(gpuCore, memManager.codeSpace, memManager.globalDataSpace, header, keyboardManager);
	immediateFloat = 0;
	//As far as I know, microsoft compiler needs srand to be called in each thread
#ifdef USING_MICROSOFT_COMPILER
	srand(time(0));
#endif
}

X86DynaRecCore::~X86DynaRecCore() {
	//Flush the cache
	for (X86BinBlockCache::iterator it = x86BinBlockCache.begin(); it != x86BinBlockCache.end(); ++it) {
		delete it->second;
	}
}

void X86DynaRecCore::startCPULoop() {
	while (true) {
		//Check if the code is already in cache
		X86BinBlockCache::const_iterator cacheCode = x86BinBlockCache.find(pC);
		if (cacheCode == x86BinBlockCache.end()) {
			//Check if it is an instruction that needs to be interpreted
			int fdCycleRetVal = fdCycle(0);
			if (fdCycleRetVal & FD_CYCLE_BREAK_MASK) {
				switch (fdCycleRetVal) {
					case FD_CYCLE_BREAK_CALL:
						CALL_IMMI();
						break;
					case FD_CYCLE_BREAK_JC:
						JC_R_IMMI();
						break;
					case FD_CYCLE_BREAK_JCR:
						JCR_R_IMMI();
						break;
					case FD_CYCLE_BREAK_JMP:
						JMP_IMMI();
						break;
					case FD_CYCLE_BREAK_JMPR:
						JMPR_IMMI();
						break;
					case FD_CYCLE_BREAK_RET:
						RET();
						break;
					case FD_CYCLE_END:
						return;
				}
			} else {
				createNewBinBlock();
			}
		} else {
			//If the code exists in the cache, run it
			executeBlock(cacheCode->second);
			pC = cacheCode->second->endAddress;
		}
	}
}

void X86DynaRecCore::createNewBinBlock() {
	//Decode the code
	X86BinBlock *binBlock = new X86BinBlock;
	immediateFloat = new vector<ImmediateFloat>;

	subReg64Immi32(binBlock, rsp, 0x20);	//Shadow Space
	binBlock->startAddress = pC;
	while (fdCycle(binBlock) == FD_CYCLE_CONTINUE);
	binBlock->endAddress = pC;
	addReg64Immi32(binBlock, rsp, 0x20);	//Shadow Space
	ret(binBlock);
	putImmediateFloats(binBlock);
	delete immediateFloat;
	immediateFloat = 0;

	//Execute the code
	executeBlock(binBlock);

	//Put it in the cache for future use
	x86BinBlockCache.insert(X86BinBlockCache::value_type(binBlock->startAddress, binBlock));
}

void X86DynaRecCore::executeBlock(X86BinBlock *binBlock) {
	uint8 *bufferPtr = binBlock->getBinBuffer();
	((void(*)())bufferPtr)();
}

void X86DynaRecCore::putImmediateFloats(X86BinBlock *binBlock) {
	for (size_t i = 0; i < immediateFloat->size(); ++i) {
		uint32 floatIndex = binBlock->getCounter();
		binBlock->write<uint32>(*(uint32*)&immediateFloat->at(i).value);
		binBlock->writeAtIndex(immediateFloat->at(i).counter, floatIndex - immediateFloat->at(i).counter - 4);
	}
}

int X86DynaRecCore::fdCycle(X86BinBlock *binBlock) {
	if (pC < 0) {
		return FD_CYCLE_END;
	}

	uint16 opcode = *(uint16*)&memManager.codeSpace[pC];
	pC += 2;

	switch(opcode) {
		case _MOV_R_M:
			MOV_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOV_M_R:
			MOV_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOV_R_R:
			MOV_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOV_M_M:
			MOV_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _MOV_MR_R:
			MOV_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOV_R_MR:
			MOV_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOV_MR_M:
			MOV_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOV_M_MR:
			MOV_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOV_MR_MR:
			MOV_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOV_R_IMMI:
			MOV_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOV_M_IMMI:
			MOV_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _MOV_MR_IMMI:
			MOV_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _ADD_R_M:
			ADD_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _ADD_M_R:
			ADD_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _ADD_R_R:
			ADD_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _ADD_M_M:
			ADD_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _ADD_MR_R:
			ADD_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _ADD_R_MR:
			ADD_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _ADD_MR_M:
			ADD_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _ADD_M_MR:
			ADD_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _ADD_MR_MR:
			ADD_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _ADD_R_IMMI:
			ADD_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _ADD_M_IMMI:
			ADD_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _ADD_MR_IMMI:
			ADD_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SUB_R_M:
			SUB_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SUB_M_R:
			SUB_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SUB_R_R:
			SUB_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SUB_M_M:
			SUB_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _SUB_MR_R:
			SUB_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SUB_R_MR:
			SUB_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SUB_MR_M:
			SUB_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SUB_M_MR:
			SUB_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SUB_MR_MR:
			SUB_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SUB_R_IMMI:
			SUB_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SUB_M_IMMI:
			SUB_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _SUB_MR_IMMI:
			SUB_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MUL_R_M:
			MUL_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MUL_M_R:
			MUL_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MUL_R_R:
			MUL_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MUL_M_M:
			MUL_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _MUL_MR_R:
			MUL_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MUL_R_MR:
			MUL_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MUL_MR_M:
			MUL_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MUL_M_MR:
			MUL_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MUL_MR_MR:
			MUL_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MUL_R_IMMI:
			MUL_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MUL_M_IMMI:
			MUL_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _MUL_MR_IMMI:
			MUL_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _DIV_R_M:
			DIV_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _DIV_M_R:
			DIV_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _DIV_R_R:
			DIV_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _DIV_M_M:
			DIV_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _DIV_MR_R:
			DIV_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _DIV_R_MR:
			DIV_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _DIV_MR_M:
			DIV_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _DIV_M_MR:
			DIV_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _DIV_MR_MR:
			DIV_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _DIV_R_IMMI:
			DIV_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _DIV_M_IMMI:
			DIV_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _DIV_MR_IMMI:
			DIV_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOD_R_M:
			MOD_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOD_M_R:
			MOD_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOD_R_R:
			MOD_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOD_M_M:
			MOD_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _MOD_MR_R:
			MOD_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOD_R_MR:
			MOD_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOD_MR_M:
			MOD_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOD_M_MR:
			MOD_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOD_MR_MR:
			MOD_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOD_R_IMMI:
			MOD_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOD_M_IMMI:
			MOD_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _MOD_MR_IMMI:
			MOD_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _AND_R_M:
			AND_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _AND_M_R:
			AND_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _AND_R_R:
			AND_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _AND_M_M:
			AND_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _AND_MR_R:
			AND_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _AND_R_MR:
			AND_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _AND_MR_M:
			AND_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _AND_M_MR:
			AND_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _AND_MR_MR:
			AND_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _AND_R_IMMI:
			AND_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _AND_M_IMMI:
			AND_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _AND_MR_IMMI:
			AND_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OR_R_M:
			OR_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OR_M_R:
			OR_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OR_R_R:
			OR_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OR_M_M:
			OR_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _OR_MR_R:
			OR_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OR_R_MR:
			OR_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OR_MR_M:
			OR_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OR_M_MR:
			OR_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OR_MR_MR:
			OR_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OR_R_IMMI:
			OR_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OR_M_IMMI:
			OR_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _OR_MR_IMMI:
			OR_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _XOR_R_M:
			XOR_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _XOR_M_R:
			XOR_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _XOR_R_R:
			XOR_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _XOR_M_M:
			XOR_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _XOR_MR_R:
			XOR_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _XOR_R_MR:
			XOR_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _XOR_MR_M:
			XOR_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _XOR_M_MR:
			XOR_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _XOR_MR_MR:
			XOR_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _XOR_R_IMMI:
			XOR_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _XOR_M_IMMI:
			XOR_M_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _XOR_MR_IMMI:
			XOR_MR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHL_R_IMMI8:
			SHL_R_IMMI8(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHL_M_IMMI8:
			SHL_M_IMMI8(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHL_MR_IMMI8:
			SHL_MR_IMMI8(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHL_R_R:
			SHL_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHL_M_R:
			SHL_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHL_MR_R:
			SHL_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHL_R_M:
			SHL_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHL_M_M:
			SHL_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _SHL_MR_M:
			SHL_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHL_R_MR:
			SHL_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHL_M_MR:
			SHL_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHL_MR_MR:
			SHL_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHR_R_IMMI8:
			SHR_R_IMMI8(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHR_M_IMMI8:
			SHR_M_IMMI8(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHR_MR_IMMI8:
			SHR_MR_IMMI8(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHR_R_R:
			SHR_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHR_M_R:
			SHR_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHR_MR_R:
			SHR_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHR_R_M:
			SHR_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHR_M_M:
			SHR_M_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _SHR_MR_M:
			SHR_MR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHR_R_MR:
			SHR_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _SHR_M_MR:
			SHR_M_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _SHR_MR_MR:
			SHR_MR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _NOP:
			NOP(binBlock);
			break;
		case _JMP_IMMI:
			pC -= 2;
			return FD_CYCLE_BREAK_JMP;
		case _JMPR_IMMI:
			pC -= 2;
			return FD_CYCLE_BREAK_JMPR;
		case _JC_R_IMMI:
			pC -= 2;
			return FD_CYCLE_BREAK_JC;
		case _JCR_R_IMMI:
			pC -= 2;
			return FD_CYCLE_BREAK_JCR;
		case _MOVP_R_M:
			MOVP_R_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOVP_M_R:
			MOVP_M_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _MOVP_R_MR:
			MOVP_R_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _MOVP_MR_R:
			MOVP_MR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _CALL_IMMI:
			pC -= 2;
			return FD_CYCLE_BREAK_CALL;
		case _RET:
			pC -= 2;
			return FD_CYCLE_BREAK_RET;
		case _PUSH_R:
			PUSH_R(binBlock);
			if (binBlock) ++pC;
			break;
		case _POP_R:
			POP_R(binBlock);
			if (binBlock) ++pC;
			break;
		case _DRW_R_R_R:
			DRW_R_R_R(binBlock);
			if (binBlock) pC += 3;
			break;
		case _DRW_R_R_M:
			DRW_R_R_M(binBlock);
			if (binBlock) pC += 6;
			break;
		case _OUT_R_IMMI8:
			OUT_R_IMMI8(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OUT_R_IMMI16:
			OUT_R_IMMI16(binBlock);
			if (binBlock) pC += 3;
			break;
		case _OUT_R_IMMI32:
			OUT_R_IMMI32(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OUT_R_IMMI64:
			OUT_R_IMMI64(binBlock);
			if (binBlock) pC += 9;
			break;
		case _OUT_IMMI_R8:
			OUT_IMMI_R8(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OUT_IMMI_R16:
			OUT_IMMI_R16(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OUT_IMMI_R32:
			OUT_IMMI_R32(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OUT_IMMI_R64:
			OUT_IMMI_R64(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OUT_R_R8:
			OUT_R_R8(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OUT_R_R16:
			OUT_R_R16(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OUT_R_R32:
			OUT_R_R32(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OUT_R_R64:
			OUT_R_R64(binBlock);
			if (binBlock) pC += 2;
			break;
		case _OUT_IMMI_IMMI8:
			OUT_IMMI_IMMI8(binBlock);
			if (binBlock) pC += 5;
			break;
		case _OUT_IMMI_IMMI16:
			OUT_IMMI_IMMI16(binBlock);
			if (binBlock) pC += 6;
			break;
		case _OUT_IMMI_IMMI32:
			OUT_IMMI_IMMI32(binBlock);
			if (binBlock) pC += 8;
			break;
		case _OUT_IMMI_IMMI64:
			OUT_IMMI_IMMI64(binBlock);
			if (binBlock) pC += 12;
			break;
		case _IN_R8_IMMI:
			IN_R8_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _IN_R16_IMMI:
			IN_R16_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _IN_R32_IMMI:
			IN_R32_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _IN_R64_IMMI:
			IN_R64_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _IN_R8_R:
			IN_R8_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _IN_R16_R:
			IN_R16_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _IN_R32_R:
			IN_R32_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _IN_R64_R:
			IN_R64_R(binBlock);
			if (binBlock) pC += 2;
			break;
//////////////////////////////////////////////
		case _FCON_R_FR:
			FCON_R_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FCON_FR_R:
			FCON_FR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FCON_MR_FR:
			FCON_MR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FCON_FR_MR:
			FCON_FR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FCON_M_FR:
			FCON_M_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_FR_M:
			FCON_FR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_R_FM:
			FCON_R_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_FM_R:
			FCON_FM_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_MR_FM:
			FCON_MR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_FM_MR:
			FCON_FM_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_M_FM:
			FCON_M_FM(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FCON_FM_M:
			FCON_FM_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FCON_R_MFR:
			FCON_R_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FCON_MFR_R:
			FCON_MFR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FCON_MR_MFR:
			FCON_MR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FCON_MFR_MR:
			FCON_MFR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FCON_M_MFR:
			FCON_M_MFR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_MFR_M:
			FCON_MFR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_FR_IMMI:
			FCON_FR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FCON_FM_IMMI:
			FCON_FM_IMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FCON_MFR_IMMI:
			FCON_MFR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOV_FR_FR:
			FMOV_FR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOV_FR_FM:
			FMOV_FR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOV_FM_FR:
			FMOV_FM_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOV_FR_MFR:
			FMOV_FR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOV_MFR_FR:
			FMOV_MFR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOV_FM_MFR:
			FMOV_FM_MFR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOV_MFR_FM:
			FMOV_MFR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOV_MFR_MFR:
			FMOV_MFR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOV_FM_FM:
			FMOV_FM_FM(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMOV_FR_FIMMI:
			FMOV_FR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOV_FM_FIMMI:
			FMOV_FM_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMOV_MFR_FIMMI:
			FMOV_MFR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_FR_FR:
			FADD_FR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_FR_FM:
			FADD_FR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_FM_FR:
			FADD_FM_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_FR_MFR:
			FADD_FR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_MFR_FR:
			FADD_MFR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_R_FR:
			FADD_R_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_FR_R:
			FADD_FR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_MR_FR:
			FADD_MR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_FR_MR:
			FADD_FR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_M_FR:
			FADD_M_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_FR_M:
			FADD_FR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_R_FM:
			FADD_R_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_FM_R:
			FADD_FM_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_MR_FM:
			FADD_MR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_FM_MR:
			FADD_FM_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_M_FM:
			FADD_M_FM(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FADD_FM_M:
			FADD_FM_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FADD_R_MFR:
			FADD_R_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_MFR_R:
			FADD_MFR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_MR_MFR:
			FADD_MR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_MFR_MR:
			FADD_MFR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FADD_M_MFR:
			FADD_M_MFR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_MFR_M:
			FADD_MFR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_FR_FIMMI:
			FADD_FR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_FM_FIMMI:
			FADD_FM_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FADD_MFR_FIMMI:
			FADD_MFR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_R_FIMMI:
			FADD_R_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FADD_M_FIMMI:
			FADD_M_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FADD_MR_FIMMI:
			FADD_MR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_FR_FR:
			FSUB_FR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_FR_FM:
			FSUB_FR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_FM_FR:
			FSUB_FM_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_FR_MFR:
			FSUB_FR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_MFR_FR:
			FSUB_MFR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_R_FR:
			FSUB_R_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_FR_R:
			FSUB_FR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_MR_FR:
			FSUB_MR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_FR_MR:
			FSUB_FR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_M_FR:
			FSUB_M_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_FR_M:
			FSUB_FR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_R_FM:
			FSUB_R_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_FM_R:
			FSUB_FM_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_MR_FM:
			FSUB_MR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_FM_MR:
			FSUB_FM_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_M_FM:
			FSUB_M_FM(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FSUB_FM_M:
			FSUB_FM_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FSUB_R_MFR:
			FSUB_R_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_MFR_R:
			FSUB_MFR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_MR_MFR:
			FSUB_MR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_MFR_MR:
			FSUB_MFR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FSUB_M_MFR:
			FSUB_M_MFR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_MFR_M:
			FSUB_MFR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_FR_FIMMI:
			FSUB_FR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_FM_FIMMI:
			FSUB_FM_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FSUB_MFR_FIMMI:
			FSUB_MFR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_R_FIMMI:
			FSUB_R_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FSUB_M_FIMMI:
			FSUB_M_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FSUB_MR_FIMMI:
			FSUB_MR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_FR_FR:
			FMUL_FR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_FR_FM:
			FMUL_FR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_FM_FR:
			FMUL_FM_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_FR_MFR:
			FMUL_FR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_MFR_FR:
			FMUL_MFR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_R_FR:
			FMUL_R_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_FR_R:
			FMUL_FR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_MR_FR:
			FMUL_MR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_FR_MR:
			FMUL_FR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_M_FR:
			FMUL_M_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_FR_M:
			FMUL_FR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_R_FM:
			FMUL_R_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_FM_R:
			FMUL_FM_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_MR_FM:
			FMUL_MR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_FM_MR:
			FMUL_FM_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_M_FM:
			FMUL_M_FM(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMUL_FM_M:
			FMUL_FM_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMUL_R_MFR:
			FMUL_R_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_MFR_R:
			FMUL_MFR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_MR_MFR:
			FMUL_MR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_MFR_MR:
			FMUL_MFR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMUL_M_MFR:
			FMUL_M_MFR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_MFR_M:
			FMUL_MFR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_FR_FIMMI:
			FMUL_FR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_FM_FIMMI:
			FMUL_FM_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMUL_MFR_FIMMI:
			FMUL_MFR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_R_FIMMI:
			FMUL_R_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMUL_M_FIMMI:
			FMUL_M_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMUL_MR_FIMMI:
			FMUL_MR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_FR_FR:
			FDIV_FR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_FR_FM:
			FDIV_FR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_FM_FR:
			FDIV_FM_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_FR_MFR:
			FDIV_FR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_MFR_FR:
			FDIV_MFR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_R_FR:
			FDIV_R_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_FR_R:
			FDIV_FR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_MR_FR:
			FDIV_MR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_FR_MR:
			FDIV_FR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_M_FR:
			FDIV_M_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_FR_M:
			FDIV_FR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_R_FM:
			FDIV_R_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_FM_R:
			FDIV_FM_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_MR_FM:
			FDIV_MR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_FM_MR:
			FDIV_FM_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_M_FM:
			FDIV_M_FM(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FDIV_FM_M:
			FDIV_FM_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FDIV_R_MFR:
			FDIV_R_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_MFR_R:
			FDIV_MFR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_MR_MFR:
			FDIV_MR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_MFR_MR:
			FDIV_MFR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FDIV_M_MFR:
			FDIV_M_MFR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_MFR_M:
			FDIV_MFR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_FR_FIMMI:
			FDIV_FR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_FM_FIMMI:
			FDIV_FM_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FDIV_MFR_FIMMI:
			FDIV_MFR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_R_FIMMI:
			FDIV_R_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FDIV_M_FIMMI:
			FDIV_M_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FDIV_MR_FIMMI:
			FDIV_MR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_FR_FR:
			FMOD_FR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_FR_FM:
			FMOD_FR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_FM_FR:
			FMOD_FM_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_FR_MFR:
			FMOD_FR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_MFR_FR:
			FMOD_MFR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_R_FR:
			FMOD_R_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_FR_R:
			FMOD_FR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_MR_FR:
			FMOD_MR_FR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_FR_MR:
			FMOD_FR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_M_FR:
			FMOD_M_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_FR_M:
			FMOD_FR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_R_FM:
			FMOD_R_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_FM_R:
			FMOD_FM_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_MR_FM:
			FMOD_MR_FM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_FM_MR:
			FMOD_FM_MR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_M_FM:
			FMOD_M_FM(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMOD_FM_M:
			FMOD_FM_M(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMOD_R_MFR:
			FMOD_R_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_MFR_R:
			FMOD_MFR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_MR_MFR:
			FMOD_MR_MFR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_MFR_MR:
			FMOD_MFR_MR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _FMOD_M_MFR:
			FMOD_M_MFR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_MFR_M:
			FMOD_MFR_M(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_FR_FIMMI:
			FMOD_FR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_FM_FIMMI:
			FMOD_FM_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMOD_MFR_FIMMI:
			FMOD_MFR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_R_FIMMI:
			FMOD_R_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FMOD_M_FIMMI:
			FMOD_M_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _FMOD_MR_FIMMI:
			FMOD_MR_FIMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _BMOV_R_BM:
			BMOV_R_BM(binBlock);
			if (binBlock) pC += 5;
			break;
		case _BMOV_BM_R:
			BMOV_BM_R(binBlock);
			if (binBlock) pC += 5;
			break;
		case _BMOV_R_BR:
			BMOV_R_MBR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _BMOV_BR_R:
			BMOV_MBR_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _BMOV_BR_BR:
			BMOV_MBR_MBR(binBlock);
			if (binBlock) pC += 2;
			break;
		case _BMOV_BM_BM:
			BMOV_BM_BM(binBlock);
			if (binBlock) pC += 8;
			break;
		case _BMOV_BR_IMMI8:
			BMOV_MBR_IMMI8(binBlock);
			if (binBlock) pC += 2;
			break;
		case _BMOV_BM_IMMI8:
			BMOV_BM_IMMI8(binBlock);
			if (binBlock) pC += 5;
			break;
		case _CMPE_R_R:
			CMPE_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _CMPNE_R_R:
			CMPNE_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _CMPG_R_R:
			CMPG_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _CMPL_R_R:
			CMPL_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _CMPGE_R_R:
			CMPGE_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _CMPLE_R_R:
			CMPLE_R_R(binBlock);
			if (binBlock) pC += 2;
			break;
		case _CMPE_R_FR_FR:
			FCMPE_R_FR_FR(binBlock);
			if (binBlock) pC += 3;
			break;
		case _CMPNE_R_FR_FR:
			FCMPNE_R_FR_FR(binBlock);
			if (binBlock) pC += 3;
			break;
		case _CMPG_R_FR_FR:
			FCMPG_R_FR_FR(binBlock);
			if (binBlock) pC += 3;
			break;
		case _CMPL_R_FR_FR:
			FCMPL_R_FR_FR(binBlock);
			if (binBlock) pC += 3;
			break;
		case _CMPGE_R_FR_FR:
			FCMPGE_R_FR_FR(binBlock);
			if (binBlock) pC += 3;
			break;
		case _CMPLE_R_FR_FR:
			FCMPLE_R_FR_FR(binBlock);
			if (binBlock) pC += 3;
			break;
		case _FOUT_IMMI_FR:
			FOUT_IMMI_FR(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FIN_IMMI_FR:
			FIN_FR_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _FOUT_IMMI_FIMMI:
			FOUT_IMMI_FIMMI(binBlock);
			if (binBlock) pC += 8;
			break;
		case _CMPE_R_IMMI:
			CMPE_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _CMPNE_R_IMMI:
			CMPNE_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _CMPG_R_IMMI:
			CMPG_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _CMPGE_R_IMMI:
			CMPGE_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _CMPL_R_IMMI:
			CMPL_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _CMPLE_R_IMMI:
			CMPLE_R_IMMI(binBlock);
			if (binBlock) pC += 5;
			break;
		case _TIME:
			TIME(binBlock);
			break;
		case _SLEEP:
			SLEEP(binBlock);
			break;
		case _RAND:
			RAND(binBlock);
	}

	if (!binBlock) {
		pC -= 2;
	}
	return FD_CYCLE_CONTINUE;
}

void X86DynaRecCore::MOV_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MOV_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, regAddr);	//mov eax, (regAddr)
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::MOV_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movMOffsetRAX(binBlock, regAddr1);	//mov (regAddr1), rax
}

void X86DynaRecCore::MOV_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movMOffsetEAX(binBlock, gMemoryAddr1);	//mov (gMemoryAddr1), eax
}

void X86DynaRecCore::MOV_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movEAX_MOffset(binBlock, regAddr);	//mov eax, (regAddr)
	movMReg32Reg32(binBlock, rcx, eax);	//mov (rcx), eax
}

void X86DynaRecCore::MOV_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, eax, rax);	//mov eax, (rax)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MOV_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	movMReg32Reg32(binBlock, rcx, eax);	//mov (rcx), eax
}

void X86DynaRecCore::MOV_M_MR(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, eax, rax);	//mov eax, (rax)
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::MOV_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::MOV_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg32Immi32(binBlock, eax, immiValue);	//mov eax, immiValue
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MOV_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	movMReg32Immi32(binBlock, rax, immiValue);	//mov (rax), immiValue
}

void X86DynaRecCore::MOV_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movMReg32Immi32(binBlock, rax, immiValue);	//mov (rax), immiValue
}

void X86DynaRecCore::ADD_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov rax, (gMemoryAddr)
	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	addMReg64Reg64(binBlock, rcx, rax);	//add (rcx), rax
}

void X86DynaRecCore::ADD_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemoryAddr
	addMReg32Reg32(binBlock, rcx, eax);	//add (rcx), eax
}

void X86DynaRecCore::ADD_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Immi64(binBlock, rcx, regAddr1);	//mov rcx, regAddr1
	addMReg64Reg64(binBlock, rcx, rax);	//add (rcx), rax
}

void X86DynaRecCore::ADD_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg64Immi64(binBlock, rcx, gMemoryAddr1);	//mov rcx, gMemoryAddr1
	addMReg32Reg32(binBlock, rcx, eax);	//add (rcx), eax
}

void X86DynaRecCore::ADD_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	addMReg32Reg32(binBlock, rax, ecx);	//add (rax), ecx
}

void X86DynaRecCore::ADD_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	addMReg64Reg64(binBlock, rax, rcx);	//add (rax), rcx
}

void X86DynaRecCore::ADD_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	addMReg32Reg32(binBlock, rax, ecx);	//add (rax), (ecx)
}

void X86DynaRecCore::ADD_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, gMemoryAddress);	//mov rax, gMemoryAddr
	addMReg32Reg32(binBlock, rax, ecx);	//add (rax), ecx
}

void X86DynaRecCore::ADD_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	addMReg32Reg32(binBlock, rax, ecx);		//add (rax), ecx
}

void X86DynaRecCore::ADD_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	addMReg64Reg64(binBlock, rax, rcx);	//add (rax), rcx
}

void X86DynaRecCore::ADD_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	addMReg32Immi32(binBlock, rax, immiValue);	//add (rax), immiValue
}

void X86DynaRecCore::ADD_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	addMReg32Immi32(binBlock, rax, immiValue);	//add (rax), immiValue
}

void X86DynaRecCore::SUB_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov rax, (gMemoryAddr)
	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	subMReg64Reg64(binBlock, rcx, rax);	//sub (rcx), rax
}

void X86DynaRecCore::SUB_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemoryAddr
	subMReg32Reg32(binBlock, rcx, eax);	//sub (rcx), eax
}

void X86DynaRecCore::SUB_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Immi64(binBlock, rcx, regAddr1);	//mov rcx, regAddr1
	subMReg64Reg64(binBlock, rcx, rax);	//sub (rcx), rax
}

void X86DynaRecCore::SUB_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg64Immi64(binBlock, rcx, gMemoryAddr1);	//mov rcx, gMemoryAddr1
	subMReg32Reg32(binBlock, rcx, eax);	//sub (rcx), eax
}

void X86DynaRecCore::SUB_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	subMReg32Reg32(binBlock, rax, ecx);	//sub (rax), ecx
}

void X86DynaRecCore::SUB_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	subMReg64Reg64(binBlock, rax, rcx);	//sub (rax), rcx
}

void X86DynaRecCore::SUB_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	subMReg32Reg32(binBlock, rax, ecx);	//sub (rax), (ecx)
}

void X86DynaRecCore::SUB_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, gMemoryAddress);	//mov rax, gMemoryAddr
	subMReg32Reg32(binBlock, rax, ecx);	//sub (rax), ecx
}

void X86DynaRecCore::SUB_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	subMReg32Reg32(binBlock, rax, ecx);		//sub (rax), ecx
}

void X86DynaRecCore::SUB_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	subMReg64Reg64(binBlock, rax, rcx);	//sub (rax), rcx
}

void X86DynaRecCore::SUB_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	subMReg32Immi32(binBlock, rax, immiValue);	//sub (rax), immiValue
}

void X86DynaRecCore::SUB_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	subMReg32Immi32(binBlock, rax, immiValue);	//sub (rax), immiValue
}

void X86DynaRecCore::MUL_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	imulReg64MReg64(binBlock, rax, rcx);	//imul rax, (rcx)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MUL_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	imulReg64MReg64(binBlock, rax, rcx);	//imul rax, (rcx)
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::MUL_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr2);	//mov rcx, regAddr2
	movRAX_MOffset(binBlock, regAddr1);	//mov rax, (regAddr1)
	imulReg64MReg64(binBlock, rax, rcx);	//imul rax, (rcx)
	movMOffsetRAX(binBlock, regAddr1);	//mov (regAddr1), rax
}

void X86DynaRecCore::MUL_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rcx, gMemoryAddr2);	//mov rcx, gMemoryAddr2
	movEAX_MOffset(binBlock, gMemoryAddr1);	//mov eax, (gMemoryAddr1)
	imulReg32MReg32(binBlock, eax, rcx);	//imul eax, (rcx)
	movMOffsetEAX(binBlock, gMemoryAddr2);	//mov (gMemoryAddr2), eax
}

void X86DynaRecCore::MUL_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rdx, regAddr);	//mov rdx, regAddr
	imulReg64MReg64(binBlock, rcx, rdx);	//imul rcx, (rdx)
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::MUL_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	imulRAX_Reg64(binBlock, rcx);	//imul rax, rcx
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MUL_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rdx, gMemoryAddress);	//mov rdx, gMemoryAddress
	imulReg32MReg32(binBlock, ecx, rdx);	//imul rcx, (rdx)
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::MUL_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	imulEAX_Reg32(binBlock, ecx);	//imul eax, ecx
	movMOffsetEAX(binBlock, gMemoryAddress);	//mov (gMemoryAddress), eax
}

void X86DynaRecCore::MUL_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	imulReg32MReg32(binBlock, ecx, rax);	//imul ecx, (rax)
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::MUL_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	imulReg64Reg64Immi32(binBlock, rax, rax, immiValue); //imul rax, rax, immiValue
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MUL_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	imulReg32Reg32Immi32(binBlock, eax, eax, immiValue);	//imul eax, eax, immiValue
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::MUL_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	imulReg32MReg32Immi32(binBlock, ecx, rax, immiValue);	//imul ecx, (rax), immiValue
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::DIV_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::DIV_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::DIV_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, regAddr1);	//mov rax, (regAddr1)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movMOffsetRAX(binBlock, regAddr1);	//mov (regAddr1), rax
}

void X86DynaRecCore::DIV_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movEAX_MOffset(binBlock, gMemoryAddr1);	//mov eax, (gMemoryAddr1)
	xorReg32Reg32(binBlock, edx, edx);	//xor edx, edx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movMOffsetEAX(binBlock, gMemoryAddr2);	//mov (gMemoryAddr2), eax
}

void X86DynaRecCore::DIV_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Reg64(binBlock, rbx, rax);	//mov rbx, rax
	movReg32MReg32(binBlock, eax, rbx);	//mov eax, (rbx)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movMReg32Reg32(binBlock, rbx, eax);	//mov (rbx), eax
}

void X86DynaRecCore::DIV_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::DIV_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Reg64(binBlock, rbx, rax);	//mov rbx, rax
	movReg32MReg32(binBlock, eax, rbx);	//mov eax, (rbx)
	xorReg32Reg32(binBlock, edx, edx);	//xor edx, edx
	idivEAX_Reg32(binBlock, ecx);	//idiv ecx
	movMReg32Reg32(binBlock, rbx, eax);	//mov (rbx), eax
}

void X86DynaRecCore::DIV_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	xorReg32Reg32(binBlock, edx, edx);	//xor edx, edx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movMOffsetEAX(binBlock, gMemoryAddress);	//mov (gMemoryAddress), eax
}

void X86DynaRecCore::DIV_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	movReg64Reg64(binBlock, rbx, rax);	//mov rbx, rax
	movReg32MReg32(binBlock, eax, rbx);	//mov eax, (rbx)
	xorReg32Reg32(binBlock, edx, edx);	//xor edx, edx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movMReg32Reg32(binBlock, rbx, eax);	//mov (rbx), eax
}

void X86DynaRecCore::DIV_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx); //idiv rax, rcx
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::DIV_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::DIV_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Reg64(binBlock, rbx, rax);	//mov rbx, rax
	movReg32MReg32(binBlock, eax, rbx); //mov eax, (rbx)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movMReg32Reg32(binBlock, rbx, eax);	//mov (rbx), eax
}

void X86DynaRecCore::MOD_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movReg64Reg64(binBlock, rax, rdx);	//mov rax, rdx
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MOD_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movReg64Reg64(binBlock, rax, rdx);	//mov rax, rdx
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::MOD_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, regAddr1);	//mov rax, (regAddr1)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movReg64Reg64(binBlock, rax, rdx);	//mov rax, rdx
	movMOffsetRAX(binBlock, regAddr1);	//mov (regAddr1), rax
}

void X86DynaRecCore::MOD_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movEAX_MOffset(binBlock, gMemoryAddr1);	//mov eax, (gMemoryAddr1)
	xorReg32Reg32(binBlock, edx, edx);	//xor edx, edx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movReg32Reg32(binBlock, eax, edx);	//mov eax, edx
	movMOffsetEAX(binBlock, gMemoryAddr2);	//mov (gMemoryAddr2), eax
}

void X86DynaRecCore::MOD_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Reg64(binBlock, rbx, rax);	//mov rbx, rax
	movReg32MReg32(binBlock, eax, rbx);	//mov eax, (rbx)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movReg32Reg32(binBlock, eax, edx);	//mov eax, edx
	movMReg32Reg32(binBlock, rbx, eax);	//mov (rbx), eax
}

void X86DynaRecCore::MOD_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx);	//idiv rax, rcx
	movReg64Reg64(binBlock, rax, rdx);	//mov rax, rdx
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MOD_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Reg64(binBlock, rbx, rax);	//mov rbx, rax
	movReg32MReg32(binBlock, eax, rbx);	//mov eax, (rbx)
	xorReg32Reg32(binBlock, edx, edx);	//xor edx, edx
	idivEAX_Reg32(binBlock, ecx);	//idiv ecx
	movMReg32Reg32(binBlock, rbx, edx);	//mov (rbx), edx
}

void X86DynaRecCore::MOD_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	xorReg32Reg32(binBlock, edx, edx);	//xor edx, edx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movReg32Reg32(binBlock, eax, edx);	//mov eax, edx
	movMOffsetEAX(binBlock, gMemoryAddress);	//mov (gMemoryAddress), eax
}

void X86DynaRecCore::MOD_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	movReg64Reg64(binBlock, rbx, rax);	//mov rbx, rax
	movReg32MReg32(binBlock, eax, rbx);	//mov eax, (rbx)
	xorReg32Reg32(binBlock, edx, edx);	//xor edx, edx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movMReg32Reg32(binBlock, rbx, edx);	//mov (rbx), edx
}

void X86DynaRecCore::MOD_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivRAX_Reg64(binBlock, rcx); //idiv rax, rcx
	movReg64Reg64(binBlock, rax, rdx);	//mov rax, rdx
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MOD_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movReg32Reg32(binBlock, eax, edx);	//mov eax, edx
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::MOD_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Reg64(binBlock, rbx, rax);	//mov rbx, rax
	movReg32MReg32(binBlock, eax, rbx); //mov eax, (rbx)
	xorReg64Reg64(binBlock, rdx, rdx);	//xor rdx, rdx
	idivEAX_Reg32(binBlock, ecx);	//idiv eax, ecx
	movMReg32Reg32(binBlock, rbx, edx);	//mov (rbx), edx
}

void X86DynaRecCore::AND_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov rax, (gMemoryAddr)
	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	andMReg64Reg64(binBlock, rcx, rax);	//and (rcx), rax
}

void X86DynaRecCore::AND_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemoryAddr
	andMReg32Reg32(binBlock, rcx, eax);	//and (rcx), eax
}

void X86DynaRecCore::AND_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Immi64(binBlock, rcx, regAddr1);	//mov rcx, regAddr1
	andMReg64Reg64(binBlock, rcx, rax);	//and (rcx), rax
}

void X86DynaRecCore::AND_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg64Immi64(binBlock, rcx, gMemoryAddr1);	//mov rcx, gMemoryAddr1
	andMReg32Reg32(binBlock, rcx, eax);	//and (rcx), eax
}

void X86DynaRecCore::AND_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	andMReg32Reg32(binBlock, rax, ecx);	//and (rax), ecx
}

void X86DynaRecCore::AND_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	andMReg64Reg64(binBlock, rax, rcx);	//and (rax), rcx
}

void X86DynaRecCore::AND_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	andMReg32Reg32(binBlock, rax, ecx);	//and (rax), (ecx)
}

void X86DynaRecCore::AND_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, gMemoryAddress);	//mov rax, gMemoryAddr
	andMReg32Reg32(binBlock, rax, ecx);	//and (rax), ecx
}

void X86DynaRecCore::AND_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	andMReg32Reg32(binBlock, rax, ecx);		//and (rax), ecx
}

void X86DynaRecCore::AND_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	andMReg64Reg64(binBlock, rax, rcx);	//and (rax), rcx
}

void X86DynaRecCore::AND_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	andMReg32Immi32(binBlock, rax, immiValue);	//add (rax), immiValue
}

void X86DynaRecCore::AND_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	andMReg32Immi32(binBlock, rax, immiValue);	//add (rax), immiValue
}

void X86DynaRecCore::OR_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov rax, (gMemoryAddr)
	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	orMReg64Reg64(binBlock, rcx, rax);	//or (rcx), rax
}

void X86DynaRecCore::OR_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemoryAddr
	orMReg32Reg32(binBlock, rcx, eax);	//or (rcx), eax
}

void X86DynaRecCore::OR_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Immi64(binBlock, rcx, regAddr1);	//mov rcx, regAddr1
	orMReg64Reg64(binBlock, rcx, rax);	//or (rcx), rax
}

void X86DynaRecCore::OR_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg64Immi64(binBlock, rcx, gMemoryAddr1);	//mov rcx, gMemoryAddr1
	orMReg32Reg32(binBlock, rcx, eax);	//or (rcx), eax
}

void X86DynaRecCore::OR_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	orMReg32Reg32(binBlock, rax, ecx);	//or (rax), ecx
}

void X86DynaRecCore::OR_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	orMReg64Reg64(binBlock, rax, rcx);	//or (rax), rcx
}

void X86DynaRecCore::OR_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	orMReg32Reg32(binBlock, rax, ecx);	//or (rax), (ecx)
}

void X86DynaRecCore::OR_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, gMemoryAddress);	//mov rax, gMemoryAddr
	orMReg32Reg32(binBlock, rax, ecx);	//or (rax), ecx
}

void X86DynaRecCore::OR_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	orMReg32Reg32(binBlock, rax, ecx);		//or (rax), ecx
}

void X86DynaRecCore::OR_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	orMReg64Reg64(binBlock, rax, rcx);	//or (rax), rcx
}

void X86DynaRecCore::OR_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	orMReg32Immi32(binBlock, rax, immiValue);	//or (rax), immiValue
}

void X86DynaRecCore::OR_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	orMReg32Immi32(binBlock, rax, immiValue);	//or (rax), immiValue
}

void X86DynaRecCore::XOR_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov rax, (gMemoryAddr)
	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	xorMReg64Reg64(binBlock, rcx, rax);	//xor (rcx), rax
}

void X86DynaRecCore::XOR_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemoryAddr
	xorMReg32Reg32(binBlock, rcx, eax);	//xor (rcx), eax
}

void X86DynaRecCore::XOR_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Immi64(binBlock, rcx, regAddr1);	//mov rcx, regAddr1
	xorMReg64Reg64(binBlock, rcx, rax);	//xor (rcx), rax
}

void X86DynaRecCore::XOR_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg64Immi64(binBlock, rcx, gMemoryAddr1);	//mov rcx, gMemoryAddr1
	xorMReg32Reg32(binBlock, rcx, eax);	//xor (rcx), eax
}

void X86DynaRecCore::XOR_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	xorMReg32Reg32(binBlock, rax, ecx);	//xor (rax), ecx
}

void X86DynaRecCore::XOR_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	xorMReg64Reg64(binBlock, rax, rcx);	//xor (rax), rcx
}

void X86DynaRecCore::XOR_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddress);	//mov eax, (gMemoryAddress)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	xorMReg32Reg32(binBlock, rax, ecx);	//xor (rax), (ecx)
}

void X86DynaRecCore::XOR_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddress = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, gMemoryAddress);	//mov rax, gMemoryAddr
	xorMReg32Reg32(binBlock, rax, ecx);	//xor (rax), ecx
}

void X86DynaRecCore::XOR_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	xorMReg32Reg32(binBlock, rax, ecx);		//xor (rax), ecx
}

void X86DynaRecCore::XOR_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	xorMReg64Reg64(binBlock, rax, rcx);	//xor (rax), rcx
}

void X86DynaRecCore::XOR_M_IMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	xorMReg32Immi32(binBlock, rax, immiValue);	//xor (rax), immiValue
}

void X86DynaRecCore::XOR_MR_IMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	xorMReg32Immi32(binBlock, rax, immiValue);	//xor (rax), immiValue
}

void X86DynaRecCore::SHL_R_IMMI8(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint8 immiValue = memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	shlMReg64Immi8(binBlock, rax, immiValue);	//shl (rax), immiValue
}

void X86DynaRecCore::SHL_M_IMMI8(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint8 immiValue = memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	shlMReg32Immi8(binBlock, rax, immiValue);	//shl (rax), immiValue
}

void X86DynaRecCore::SHL_MR_IMMI8(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint8 immiValue = memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	shlMReg32Immi8(binBlock, rax, immiValue);	//shl (rax), immiValue
}

void X86DynaRecCore::SHL_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg64Immi64(binBlock, rax, regAddr1);	//mov rax, regAddr1
	shlMReg64Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHL_M_R(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//moc rcx, rax
	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	shlMReg32Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHL_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	shlMReg32Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHL_R_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	shlMReg32Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHL_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg32Reg32(binBlock, ecx, eax);	//moc ecx, eax
	movReg64Immi64(binBlock, rax, gMemoryAddr1);	//mov rax, gMemoryAddr1
	shlMReg32Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHL_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	shlMReg32Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHL_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	shlMReg64Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHL_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	shlMReg32Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHL_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	shlMReg32Cl(binBlock, rax);	//shl (rax), cl
}

void X86DynaRecCore::SHR_R_IMMI8(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint8 immiValue = memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	shrMReg64Immi8(binBlock, rax, immiValue);	//shr (rax), immiValue
}

void X86DynaRecCore::SHR_M_IMMI8(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint8 immiValue = memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	shrMReg32Immi8(binBlock, rax, immiValue);	//shr (rax), immiValue
}

void X86DynaRecCore::SHR_MR_IMMI8(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint8 immiValue = memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	shrMReg32Immi8(binBlock, rax, immiValue);	//shr (rax), immiValue
}

void X86DynaRecCore::SHR_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg64Immi64(binBlock, rax, regAddr1);	//mov rax, regAddr1
	shrMReg64Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::SHR_M_R(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//moc rcx, rax
	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	shrMReg32Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::SHR_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	shrMReg32Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::SHR_R_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	shrMReg32Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::SHR_M_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movEAX_MOffset(binBlock, gMemoryAddr2);	//mov eax, (gMemoryAddr2)
	movReg32Reg32(binBlock, ecx, eax);	//moc ecx, eax
	movReg64Immi64(binBlock, rax, gMemoryAddr1);	//mov rax, gMemoryAddr1
	shrMReg32Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::SHR_MR_M(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movEAX_MOffset(binBlock, gMemoryAddr);	//mov eax, (gMemoryAddr)
	movReg32Reg32(binBlock, ecx, eax);	//mov ecx, eax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	shrMReg32Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::SHR_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	shrMReg64Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::SHR_M_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	shrMReg32Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::SHR_MR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 mRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr2);	//mov rax, (mRegAddr2)
	movReg32MReg32(binBlock, ecx, rax);	//mov ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr1);	//mov rax, (mRegAddr1)
	shrMReg32Cl(binBlock, rax);	//shr (rax), cl
}

void X86DynaRecCore::NOP(X86BinBlock *binBlock) {
	nop(binBlock);
}

void X86DynaRecCore::MOVP_R_M(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, gMemoryAddr);	//mov rax, (gMemoryAddr)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MOVP_M_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movMOffsetRAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), rax
}

void X86DynaRecCore::MOVP_R_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64MReg64(binBlock, rax, rax);	//mov rax, (rax)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::MOVP_MR_R(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::PUSH_R(X86BinBlock *binBlock) {
	uint64 stackAddr = (uint64)memManager.stackSpace;
	uint64 stackPointerAddr = (uint64)&sP;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Immi64(binBlock, rdx, stackAddr);	//mov rdx, stackAddr
	movReg64Immi64(binBlock, rcx, stackPointerAddr);	//mov rcx, stackPointerAddr
	movReg32MReg32(binBlock, ebx, rcx);	//mov ebx, (rcx)
	movMReg64Reg64(binBlock, 0, ebx, rdx, rax);	//mov (rdx + ebx), rax
	addMReg32Immi8(binBlock, rcx, 8);	//add (rcx), 8
}

void X86DynaRecCore::POP_R(X86BinBlock *binBlock) {
	uint64 stackAddr = (uint64)memManager.stackSpace;
	uint64 stackPointerAddr = (uint64)&sP;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	
	movReg64Immi64(binBlock, rdx, stackAddr);	//mov rdx, stackAddr
	movReg64Immi64(binBlock, rcx, stackPointerAddr);	//mov rcx, stackPointerAddr
	subMReg32Immi8(binBlock, rcx, 8);	//sub (rcx), 8
	movReg32MReg32(binBlock, ebx, rcx);	//mov ebx, (rcx)
	movReg64MReg64(binBlock, rax, 0, ebx, rdx);	//mov rax, (rdx + ebx)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::draw(int x, int y, uint64 address, X86DynaRecCore *dynarecCore) {
	dynarecCore->gpuCore->draw(x, y, address, PortManager::readPort<uint8>(PORT_GPU_EFFECTS, &dynarecCore->portManager), PortManager::readPort<uint16>(PORT_GPU_ROTATION, &dynarecCore->portManager));
}

void X86DynaRecCore::putDrawOpcode(X86BinBlock *binBlock, uint64 xAddr, uint64 yAddr, uint64 imgAddr) {
void (*drawPtr)(int, int, uint64, X86DynaRecCore*) = draw;

#ifdef USING_MICROSOFT_COMPILER
	movRAX_MOffset(binBlock, xAddr);	//mov rax, (xAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, yAddr);	//mov rax, (yAddr)
	movReg64Reg64(binBlock, rdx, rax);	//mov rdx, rax
	movRAX_MOffset(binBlock, imgAddr);	//mov rax, (imgAddr)
	movReg64Reg64(binBlock, r8, rax);	//mov r8, rax
	movReg64Immi64(binBlock, r9, (uint64)this);	//mov r9, this
	movReg64Immi64(binBlock, rax, (uint64)drawPtr);	//mov rax, drawPtr
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::DRW_R_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 regAddr3 = (uint64)regs + (memManager.codeSpace[pC + 2] << 3);

	putDrawOpcode(binBlock, regAddr1, regAddr2, regAddr3);
}

void X86DynaRecCore::DRW_R_R_M(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 2];

	putDrawOpcode(binBlock, regAddr1, regAddr2, gMemoryAddr);
}

void X86DynaRecCore::OUT_R_IMMI8(X86BinBlock *binBlock) {
	void (*writePort)(uint8, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint8 immi8 = memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movReg8Immi8(binBlock, cl, immi8);	//mov cl, immi8
	movRAX_MOffset(binBlock, regAddr);	//mov RAX, (regAddr)
	movReg64Reg64(binBlock, rdx, rax);	//mov rdx, rax
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_R_IMMI16(X86BinBlock *binBlock) {
	void (*writePort)(uint16, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint16 immi16 = *(uint16*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movReg16Immi16(binBlock, cx, immi16);	//mov cx, immi16
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rdx, rax);	//moc rdx, rax
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_R_IMMI32(X86BinBlock *binBlock) {
	void (*writePort)(uint32, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immi32 = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movReg32Immi32(binBlock, ecx, immi32);	//mov ecx, immi32
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rdx, rax);	//mov rdx, rax
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_R_IMMI64(X86BinBlock *binBlock) {
	void (*writePort)(uint64, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 immi64 = *(uint64*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movReg64Immi64(binBlock, rcx, immi64);	//mov rcx, immi64
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rdx, rax);	//mov rdx, rax
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_IMMI_R8(X86BinBlock *binBlock) {
	void (*writePort)(uint8, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movAL_MOffset(binBlock, regAddr);	//mov al, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg32Immi32(binBlock, edx, immiValue);	//mov edx, immiValue
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//moc rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_IMMI_R16(X86BinBlock *binBlock) {
	void (*writePort)(uint16, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movAX_MOffset(binBlock, regAddr);	//mov ax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg32Immi32(binBlock, edx, immiValue);	//mov edx, immiValue
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_IMMI_R32(X86BinBlock *binBlock) {
	void (*writePort)(uint32, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movEAX_MOffset(binBlock, regAddr);	//mov eax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg32Immi32(binBlock, edx, immiValue);	//mov edx, immiValue
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_IMMI_R64(X86BinBlock *binBlock) {
	void (*writePort)(uint64, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movRAX_MOffset(binBlock, regAddr);	//mov rax, (regAddr)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg32Immi32(binBlock, edx, immiValue);	//mov edx, immiValue
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_R_R8(X86BinBlock *binBlock) {
	void (*writePort)(uint8, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

#ifdef USING_MICROSOFT_COMPILER
	movAL_MOffset(binBlock, regAddr2);	//mov al, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, regAddr1);	//mov rax, (regAddr1)
	movReg64Reg64(binBlock, rdx, rax);	//mov rdx, rax
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_R_R16(X86BinBlock *binBlock) {
	void (*writePort)(uint16, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

#ifdef USING_MICROSOFT_COMPILER
	movAX_MOffset(binBlock, regAddr2);	//mov ax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, regAddr1);	//mov rax, (regAddr1)
	movReg64Reg64(binBlock, rdx, rax);	//mov rdx, rax
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_R_R32(X86BinBlock *binBlock) {
	void (*writePort)(uint32, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

#ifdef USING_MICROSOFT_COMPILER
	movEAX_MOffset(binBlock, regAddr2);	//mov eax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, regAddr1);	//mov rax, (regAddr1)
	movReg64Reg64(binBlock, rdx, rax);	//mov rdx, rax
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_R_R64(X86BinBlock *binBlock) {
	void (*writePort)(uint64, uint32, PortManager*) = PortManager::writePort;
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

#ifdef USING_MICROSOFT_COMPILER
	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movRAX_MOffset(binBlock, regAddr1);	//mov rax, (regAddr1)
	movReg64Reg64(binBlock, rdx, rax);	//mov rdx, rax
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_IMMI_IMMI8(X86BinBlock *binBlock) {
	void (*writePort)(uint8, uint32, PortManager*) = PortManager::writePort;
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC];
	uint8 immi8 = memManager.codeSpace[pC + 4];

#ifdef USING_MICROSOFT_COMPILER
	movReg8Immi8(binBlock, cl, immi8);	//mov cl, immi8
	movReg32Immi32(binBlock, edx, immiValue);	//mov edx, immiValue
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_IMMI_IMMI16(X86BinBlock *binBlock) {
	void (*writePort)(uint16, uint32, PortManager*) = PortManager::writePort;
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC];
	uint16 immi16 = *(uint16*)&memManager.codeSpace[pC + 4];

#ifdef USING_MICROSOFT_COMPILER
	movReg16Immi16(binBlock, cx, immi16);	//mov cx, immi16
	movReg32Immi32(binBlock, edx, immiValue);	//mov edx, immiValue
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_IMMI_IMMI32(X86BinBlock *binBlock) {
	void (*writePort)(uint32, uint32, PortManager*) = PortManager::writePort;
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC];
	uint32 immi32 = *(uint32*)&memManager.codeSpace[pC + 4];

#ifdef USING_MICROSOFT_COMPILER
	movReg32Immi32(binBlock, ecx, immi32);	//mov ecx, immi32
	movReg32Immi32(binBlock, edx, immiValue);	//mov edx, immiValue
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::OUT_IMMI_IMMI64(X86BinBlock *binBlock) {
	void (*writePort)(uint64, uint32, PortManager*) = PortManager::writePort;
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC];
	uint64 immi64 = *(uint64*)&memManager.codeSpace[pC + 4];

#ifdef USING_MICROSOFT_COMPILER
	movReg64Immi64(binBlock, rcx, immi64);	//mov rcx, immi64
	movReg32Immi32(binBlock, edx, immiValue);	//mov edx, immiValue
	movReg64Immi64(binBlock, r8, (uint64)&portManager);	//mov r8, portManager
	movReg64Immi64(binBlock, rax, (uint64)writePort);	//mov rax, writePort
	callReg64(binBlock, rax);	//call rax
#endif
}

void X86DynaRecCore::IN_R8_IMMI(X86BinBlock *binBlock) {
	uint8 (*readPort)(uint32, PortManager*) = PortManager::readPort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);	//mov rdx, portManager
	movReg64Immi64(binBlock, rax, (uint64)readPort);	//mov rax, readPort
	callReg64(binBlock, rax);	//call rax
	andRAX_Immi32(binBlock, 0xFF);	//and rax, 0xFF
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
#endif
}

void X86DynaRecCore::IN_R16_IMMI(X86BinBlock *binBlock) {
	uint16 (*readPort)(uint32, PortManager*) = PortManager::readPort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);	//mov rdx, portManager
	movReg64Immi64(binBlock, rax, (uint64)readPort);	//mov rax, readPort
	callReg64(binBlock, rax);	//call rax
	andRAX_Immi32(binBlock, 0xFFFF);	//and rax, 0xFFFF
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
#endif
}

void X86DynaRecCore::IN_R32_IMMI(X86BinBlock *binBlock) {
	uint32 (*readPort)(uint32, PortManager*) = PortManager::readPort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);	//mov rdx, portManager
	movReg64Immi64(binBlock, rax, (uint64)readPort);	//mov rax, readPort
	callReg64(binBlock, rax);	//call rax
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
#endif
}

void X86DynaRecCore::IN_R64_IMMI(X86BinBlock *binBlock) {
	uint64 (*readPort)(uint32, PortManager*) = PortManager::readPort;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

#ifdef USING_MICROSOFT_COMPILER
	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);	//mov rdx, portManager
	movReg64Immi64(binBlock, rax, (uint64)readPort);	//mov rax, readPort
	callReg64(binBlock, rax);	//call rax
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
#endif
}

void X86DynaRecCore::IN_R8_R(X86BinBlock *binBlock) {
	uint8 (*readPort)(uint32, PortManager*) = PortManager::readPort;
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

#ifdef USING_MICROSOFT_COMPILER
	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);	//mov rdx, portManager
	movReg64Immi64(binBlock, rax, (uint64)readPort);	//mov rax, readPort
	callReg64(binBlock, rax);	//call rax
	andRAX_Immi32(binBlock, 0xFF);	//and rax, 0xFF
	movMOffsetRAX(binBlock, regAddr1);	//mov (regAddr1), rax
#endif
}

void X86DynaRecCore::IN_R16_R(X86BinBlock *binBlock) {
	uint16 (*readPort)(uint32, PortManager*) = PortManager::readPort;
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

#ifdef USING_MICROSOFT_COMPILER
	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);	//mov rdx, portManager
	movReg64Immi64(binBlock, rax, (uint64)readPort);	//mov rax, readPort
	callReg64(binBlock, rax);	//call rax
	andRAX_Immi32(binBlock, 0xFFFF);	//and rax, 0xFFFF
	movMOffsetRAX(binBlock, regAddr1);	//mov (regAddr1), rax
#endif
}

void X86DynaRecCore::IN_R32_R(X86BinBlock *binBlock) {
	uint32 (*readPort)(uint32, PortManager*) = PortManager::readPort;
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

#ifdef USING_MICROSOFT_COMPILER
	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);	//mov rdx, portManager
	movReg64Immi64(binBlock, rax, (uint64)readPort);	//mov rax, readPort
	callReg64(binBlock, rax);	//call rax
	movMOffsetRAX(binBlock, regAddr1);	//mov (regAddr1), rax
#endif
}

void X86DynaRecCore::IN_R64_R(X86BinBlock *binBlock) {
	uint64 (*readPort)(uint32, PortManager*) = PortManager::readPort;
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

#ifdef USING_MICROSOFT_COMPILER
	movRAX_MOffset(binBlock, regAddr2);	//mov rax, (regAddr2)
	movReg64Reg64(binBlock, rcx, rax);	//mov rcx, rax
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);	//mov rdx, portManager
	movReg64Immi64(binBlock, rax, (uint64)readPort);	//mov rax, readPort
	callReg64(binBlock, rax);	//call rax
	movMOffsetRAX(binBlock, regAddr1);	//mov (regAddr1), rax
#endif
}

void X86DynaRecCore::FCON_R_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	cvtss2siReg32MReg32(binBlock, eax, rcx);	//cvtss2si eax, (rcx)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::FCON_FR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, rax
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FCON_MR_FR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	cvtss2siReg32MReg32(binBlock, ecx, rax);	//cvtss2si ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FCON_FR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, (fRegAddr)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FCON_M_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	cvtss2siReg32MReg32(binBlock, eax, rcx);	//cvtss2si eax, (rcx)
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::FCON_FR_M(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FCON_R_FM(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	cvtss2siReg32MReg32(binBlock, eax, rcx);	//cvtss2si eax, (rcx)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::FCON_FM_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FCON_MR_FM(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	cvtss2siReg32MReg32(binBlock, ecx, rcx);	//cvtss2si eax, (rcx)
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FCON_FM_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FCON_M_FM(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	cvtss2siReg32MReg32(binBlock, eax, rcx);	//cvtss2si eax, (rcx)
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::FCON_FM_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FCON_R_MFR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	cvtss2siReg32MReg32(binBlock, eax, rax);	//cvtss2si eax, (rax)
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::FCON_MFR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FCON_MR_MFR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	cvtss2siReg32MReg32(binBlock, ecx, rax);	//cvtss2si ecx, (rax)
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FCON_MFR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FCON_M_MFR(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	cvtss2siReg32MReg32(binBlock, eax, rax);	//cvtss2si eax, (rax)
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::FCON_MFR_M(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FCON_FR_IMMI(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint32 immiValue = *(uint32*)memManager.codeSpace[pC + 1];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	cvtsi2ssXMM_Reg32(binBlock, xmm0, ecx);	//cvtsi2ss xmm0, ecx
	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FCON_FM_IMMI(X86BinBlock *binBlock) {
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 4];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	cvtsi2ssXMM_Reg32(binBlock, xmm0, ecx);	//cvtsi2ss xmm0, ecx
	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FCON_MFR_IMMI(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)memManager.codeSpace[pC + 1];

	movReg32Immi32(binBlock, ecx, immiValue);	//mov ecx, immiValue
	cvtsi2ssXMM_Reg32(binBlock, xmm0, ecx);	//cvtsi2ss xmm0, ecx
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FCON_R_FIMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	cvtss2siReg32Disp32(binBlock, eax, 0);	//cvtss2si eax, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movMOffsetRAX(binBlock, regAddr);	//mov (regAddr), rax
}

void X86DynaRecCore::FCON_M_FIMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	cvtss2siReg32Disp32(binBlock, eax, 0);	//cvtss2si eax, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movMOffsetEAX(binBlock, gMemoryAddr);	//mov (gMemoryAddr), eax
}

void X86DynaRecCore::FCON_MR_FIMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	cvtss2siReg32Disp32(binBlock, ecx, 0);	//cvtss2si ecx, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FMOV_FR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr2);	//mov rax, fRegAddr2
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr1);	//mov rcx, fRegAddr1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMOV_FR_FM(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMOV_FM_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, (gFMemoryAddr)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMOV_FR_MFR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMOV_MFR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMOV_FM_MFR(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMOV_MFR_FM(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMOV_MFR_MFR(X86BinBlock *binBlock) {
	uint64 fMRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, fMRegAddr2);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//mov xmm0, (rax)
	movRAX_MOffset(binBlock, fMRegAddr1);	//mov rax, (fMRegAddr2)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMOV_FM_FM(X86BinBlock *binBlock) {
	uint64 gFMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gFMemoryAddr2);	//mov rax, gFMemoryAddr2
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr1);	//mov rcx, gFMemoryAddr1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMOV_FR_FIMMI(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMOV_FM_FIMMI(X86BinBlock *binBlock) {
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMOV_MFR_FIMMI(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FADD_FR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr2);	//mov rax, fRegAddr2
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr1);	//mov rcx, fRegAddr1
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_FR_FM(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_FM_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_FR_MFR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_MFR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FADD_R_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FADD_FR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_MR_FR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (regAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FADD_FR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (regAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_M_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FADD_FR_M(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_R_FM(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FADD_FM_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_MR_FM(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FADD_FM_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_M_FM(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FADD_FM_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_R_MFR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	cvtss2siReg32XMM(binBlock, eax, xmm0);	//cvtss2si eax, xmm0
	movMReg64Reg64(binBlock, rcx, rax);	//mov (rcx), rax
}

void X86DynaRecCore::FADD_MFR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//mov (rax), xmm0
}

void X86DynaRecCore::FADD_MR_MFR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fMRegAddr);	//mov rcx, fMRegAddr
	movReg64MReg64(binBlock, rcx, rcx);	//mov rcx, (rcx)
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FADD_MFR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fMRegAddr);	//mov rcx, fMRegAddr
	movReg64MReg64(binBlock, rcx, rcx);	//mov rcx, (rcx)
	addssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FADD_M_MFR(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemroyAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	cvtss2siReg32XMM(binBlock, eax, xmm0);	//cvtss2si eax, xmm0
	movMReg32Reg32(binBlock, rcx, eax);	//mov (rcx), eax
}

void X86DynaRecCore::FADD_MFR_M(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemroyAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FADD_FR_FIMMI(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FADD_FM_FIMMI(X86BinBlock *binBlock) {
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FADD_MFR_FIMMI(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, fMRegAddr
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FADD_R_FIMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	addssXMM_Disp32(binBlock, xmm0, 0);	//addss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FADD_M_FIMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	addssXMM_Disp32(binBlock, xmm0, 0);	//addss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FADD_MR_FIMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	addssXMM_Disp32(binBlock, xmm0, 0);	//addss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FSUB_FR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr2);	//mov rax, fRegAddr2
	movReg64Immi64(binBlock, rcx, fRegAddr1);	//mov rcx, fRegAddr1
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_FR_FM(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	subssXMM_MReg32(binBlock, xmm0, rax);	//subss xmm0, (rax)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_FM_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	subssXMM_MReg32(binBlock, xmm0, rax);	//subss xmm0, (rax)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_FR_MFR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	subssXMM_MReg32(binBlock, xmm0, rax);	//subss xmm0, (rax)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_MFR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	subssXMM_MReg32(binBlock, xmm0, rcx);	//subss xmm0, (rcx)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FSUB_R_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	subssXMM_MReg32(binBlock, xmm0, rcx);	//subss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FSUB_FR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_MR_FR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (regAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	subssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FSUB_FR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_M_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	subssXMM_MReg32(binBlock, xmm0, rcx);	//subss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FSUB_FR_M(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_R_FM(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	subssXMM_MReg32(binBlock, xmm0, rcx);	//subss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FSUB_FM_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_MR_FM(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	subssXMM_MReg32(binBlock, xmm0, rcx);	//subss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FSUB_FM_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_M_FM(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	subssXMM_MReg32(binBlock, xmm0, rcx);	//subss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FSUB_FM_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_R_MFR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	subssXMM_MReg32(binBlock, xmm0, rax);	//subss xmm0, (rax)
	cvtss2siReg32XMM(binBlock, eax, xmm0);	//cvtss2si eax, xmm0
	movMReg64Reg64(binBlock, rcx, rax);	//mov (rcx), rax
}

void X86DynaRecCore::FSUB_MFR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);	//cvtsi2ss xmm1, (rcx)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rax, xmm0);	//mov (rax), xmm0
}

void X86DynaRecCore::FSUB_MR_MFR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fMRegAddr);	//mov rcx, fMRegAddr
	movReg64MReg64(binBlock, rcx, rcx);	//mov rcx, (rcx)
	subssXMM_MReg32(binBlock, xmm0, rcx);	//subss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FSUB_MFR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Immi64(binBlock, rcx, fMRegAddr);	//mov rcx, fMRegAddr
	movReg64MReg64(binBlock, rcx, rcx);	//mov rcx, (rcx)
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FSUB_M_MFR(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemroyAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	subssXMM_MReg32(binBlock, xmm0, rax);	//subss xmm0, (rax)
	cvtss2siReg32XMM(binBlock, eax, xmm0);	//cvtss2si eax, xmm0
	movMReg32Reg32(binBlock, rcx, eax);	//mov (rcx), eax
}

void X86DynaRecCore::FSUB_MFR_M(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemroyAddr
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);	//cvtsi2ss xmm1, (rcx)
	subssXMM_XMM(binBlock, xmm0, xmm1);	//subss xmm0, xmm1
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FSUB_FR_FIMMI(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	subssXMM_Disp32(binBlock, xmm0, 0);	//subss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FSUB_FM_FIMMI(X86BinBlock *binBlock) {
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	subssXMM_Disp32(binBlock, xmm0, 0);	//subss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FSUB_MFR_FIMMI(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, fMRegAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	subssXMM_Disp32(binBlock, xmm0, 0);	//subss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FSUB_R_FIMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	subssXMM_Disp32(binBlock, xmm0, 0);	//subss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FSUB_M_FIMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	subssXMM_Disp32(binBlock, xmm0, 0);	//subss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FSUB_MR_FIMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	subssXMM_Disp32(binBlock, xmm0, 0);	//subss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FMUL_FR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr2);	//mov rax, fRegAddr2
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr1);	//mov rcx, fRegAddr1
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_FR_FM(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_FM_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_FR_MFR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_MFR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMUL_R_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FMUL_FR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_MR_FR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (regAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FMUL_FR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (regAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_M_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FMUL_FR_M(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_R_FM(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FMUL_FM_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_MR_FM(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FMUL_FM_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_M_FM(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FMUL_FM_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_R_MFR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	mulssXMM_MReg32(binBlock, xmm0, rax);	//mulss xmm0, (rax)
	cvtss2siReg32XMM(binBlock, eax, xmm0);	//cvtss2si eax, xmm0
	movMReg64Reg64(binBlock, rcx, rax);	//mov (rcx), rax
}

void X86DynaRecCore::FMUL_MFR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	mulssXMM_MReg32(binBlock, xmm0, rax);	//mulss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//mov (rax), xmm0
}

void X86DynaRecCore::FMUL_MR_MFR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fMRegAddr);	//mov rcx, fMRegAddr
	movReg64MReg64(binBlock, rcx, rcx);	//mov rcx, (rcx)
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FMUL_MFR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fMRegAddr);	//mov rcx, fMRegAddr
	movReg64MReg64(binBlock, rcx, rcx);	//mov rcx, (rcx)
	mulssXMM_MReg32(binBlock, xmm0, rcx);	//mulss xmm0, (rcx)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FMUL_M_MFR(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemroyAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	mulssXMM_MReg32(binBlock, xmm0, rax);	//mulss xmm0, (rax)
	cvtss2siReg32XMM(binBlock, eax, xmm0);	//cvtss2si eax, xmm0
	movMReg32Reg32(binBlock, rcx, eax);	//mov (rcx), eax
}

void X86DynaRecCore::FMUL_MFR_M(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemroyAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	mulssXMM_MReg32(binBlock, xmm0, rax);	//mulss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMUL_FR_FIMMI(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rax);	//mulss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMUL_FM_FIMMI(X86BinBlock *binBlock) {
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	mulssXMM_MReg32(binBlock, xmm0, rax);	//mulss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMUL_MFR_FIMMI(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movssXMM_Disp32(binBlock, xmm0, 0);	//movss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, fMRegAddr
	mulssXMM_MReg32(binBlock, xmm0, rax);	//mulss xmm0, (rax)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FMUL_R_FIMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	mulssXMM_Disp32(binBlock, xmm0, 0);	//mulss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FMUL_M_FIMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	mulssXMM_Disp32(binBlock, xmm0, 0);	//mulss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FMUL_MR_FIMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	mulssXMM_Disp32(binBlock, xmm0, 0);	//mulss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FDIV_FR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr2);	//mov rax, fRegAddr2
	movReg64Immi64(binBlock, rcx, fRegAddr1);	//mov rcx, fRegAddr1
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	addssXMM_MReg32(binBlock, xmm0, rax);	//addss xmm0, (rax)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_FR_FM(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	divssXMM_MReg32(binBlock, xmm0, rax);	//divss xmm0, (rax)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_FM_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	divssXMM_MReg32(binBlock, xmm0, rax);	//divss xmm0, (rax)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_FR_MFR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	divssXMM_MReg32(binBlock, xmm0, rax);	//divss xmm0, (rax)
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_MFR_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	divssXMM_MReg32(binBlock, xmm0, rcx);	//divss xmm0, (rcx)
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FDIV_R_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	divssXMM_MReg32(binBlock, xmm0, rcx);	//divss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FDIV_FR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_MR_FR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (regAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	divssXMM_MReg32(binBlock, xmm0, rcx);	//addss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FDIV_FR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_M_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	divssXMM_MReg32(binBlock, xmm0, rcx);	//divss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FDIV_FR_M(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	movReg64Immi64(binBlock, rcx, fRegAddr);	//mov rcx, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_R_FM(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	divssXMM_MReg32(binBlock, xmm0, rcx);	//divss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FDIV_FM_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_MR_FM(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	divssXMM_MReg32(binBlock, xmm0, rcx);	//divss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FDIV_FM_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_M_FM(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	divssXMM_MReg32(binBlock, xmm0, rcx);	//divss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FDIV_FM_M(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);	//mov rcx, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_R_MFR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	divssXMM_MReg32(binBlock, xmm0, rax);	//divss xmm0, (rax)
	cvtss2siReg32XMM(binBlock, eax, xmm0);	//cvtss2si eax, xmm0
	movMReg64Reg64(binBlock, rcx, rax);	//mov (rcx), rax
}

void X86DynaRecCore::FDIV_MFR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);	//mov rcx, regAddr
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);	//cvtsi2ss xmm1, (rcx)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rax, xmm0);	//mov (rax), xmm0
}

void X86DynaRecCore::FDIV_MR_MFR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	movReg64Immi64(binBlock, rcx, fMRegAddr);	//mov rcx, fMRegAddr
	movReg64MReg64(binBlock, rcx, rcx);	//mov rcx, (rcx)
	divssXMM_MReg32(binBlock, xmm0, rcx);	//divss xmm0, (rcx)
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FDIV_MFR_MR(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	movReg64Immi64(binBlock, rcx, fMRegAddr);	//mov rcx, fMRegAddr
	movReg64MReg64(binBlock, rcx, rcx);	//mov rcx, (rcx)
	movssXMM_MReg32(binBlock, xmm0, rcx);	//movss xmm0, (rcx)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);	//cvtsi2ss xmm1, (rax)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rcx, xmm0);	//movss (rcx), xmm0
}

void X86DynaRecCore::FDIV_M_MFR(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemroyAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);	//cvtsi2ss xmm0, (rcx)
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	divssXMM_MReg32(binBlock, xmm0, rax);	//divss xmm0, (rax)
	cvtss2siReg32XMM(binBlock, eax, xmm0);	//cvtss2si eax, xmm0
	movMReg32Reg32(binBlock, rcx, eax);	//mov (rcx), eax
}

void X86DynaRecCore::FDIV_MFR_M(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);	//mov rcx, gMemroyAddr
	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, (fMRegAddr)
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);	//cvtsi2ss xmm1, (rcx)
	divssXMM_XMM(binBlock, xmm0, xmm1);	//divss xmm0, xmm1
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FDIV_FR_FIMMI(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);	//mov rax, fRegAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	divssXMM_Disp32(binBlock, xmm0, 0);	//divss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FDIV_FM_FIMMI(X86BinBlock *binBlock) {
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);	//mov rax, gFMemoryAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	divssXMM_Disp32(binBlock, xmm0, 0);	//divss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FDIV_MFR_FIMMI(X86BinBlock *binBlock) {
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, fMRegAddr);	//mov rax, fMRegAddr
	movssXMM_MReg32(binBlock, xmm0, rax);	//movss xmm0, (rax)
	divssXMM_Disp32(binBlock, xmm0, 0);	//divss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movssMReg32XMM(binBlock, rax, xmm0);	//movss (rax), xmm0
}

void X86DynaRecCore::FDIV_R_FIMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);	//mov rax, regAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	divssXMM_Disp32(binBlock, xmm0, 0);	//divss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg64Reg64(binBlock, rax, rcx);	//mov (rax), rcx
}

void X86DynaRecCore::FDIV_M_FIMMI(X86BinBlock *binBlock) {
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);	//mov rax, gMemoryAddr
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	divssXMM_Disp32(binBlock, xmm0, 0);	//divss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

void X86DynaRecCore::FDIV_MR_FIMMI(X86BinBlock *binBlock) {
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);	//mov rax, (mRegAddr)
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);	//cvtsi2ss xmm0, (rax)
	divssXMM_Disp32(binBlock, xmm0, 0);	//divss xmm0, (rip + 0)
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	cvtss2siReg32XMM(binBlock, ecx, xmm0);	//cvtss2si ecx, xmm0
	movMReg32Reg32(binBlock, rax, ecx);	//mov (rax), ecx
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void X86DynaRecCore::FMOD_FR_FR(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr1);
	movReg64Immi64(binBlock, rcx, fRegAddr2);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, fRegAddr1);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_FR_FM(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, fRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_FM_FR(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movReg64Immi64(binBlock, rcx, fRegAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_FR_MFR(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movReg64Immi64(binBlock, rcx, fRegAddr);
	movRAX_MOffset(binBlock, fMRegAddr);
	movssXMM_MReg32(binBlock, xmm0, rcx);
	movssXMM_MReg32(binBlock, xmm1, rax);
	movReg64Immi64(binBlock, rax, (uint64)fmodPtr);
	callReg64(binBlock, rax);
	movReg64Immi64(binBlock, rcx, fRegAddr);
	movssMReg32XMM(binBlock, rcx, xmm0);
}

void X86DynaRecCore::FMOD_MFR_FR(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);
	movReg64Immi64(binBlock, rcx, fRegAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movRAX_MOffset(binBlock, fMRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_R_FR(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, regAddr);
	movReg64Immi64(binBlock, rcx, fRegAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movReg64Immi64(binBlock, rax, regAddr);
	movMReg64Reg64(binBlock, rax, rcx);
}

void X86DynaRecCore::FMOD_FR_R(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr);
	movReg64Immi64(binBlock, rcx, regAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, fRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_MR_FR(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movRAX_MOffset(binBlock, mRegAddr);
	movReg64Immi64(binBlock, rcx, fRegAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movRAX_MOffset(binBlock, mRegAddr);
	movMReg32Reg32(binBlock, rax, ecx);
}

void X86DynaRecCore::FMOD_FR_MR(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);

	movReg64Immi64(binBlock, rcx, fRegAddr);
	movRAX_MOffset(binBlock, mRegAddr);
	movssXMM_MReg32(binBlock, xmm0, rcx);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);
	movReg64Immi64(binBlock, rax, (uint64)fmodPtr);
	callReg64(binBlock, rax);
	movReg64Immi64(binBlock, rcx, fRegAddr);
	movssMReg32XMM(binBlock, rcx, xmm0);
}

void X86DynaRecCore::FMOD_M_FR(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gMemoryAddr);
	movReg64Immi64(binBlock, rcx, fRegAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gMemoryAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg32Reg32(binBlock, rax, ecx);
}

void X86DynaRecCore::FMOD_FR_M(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);
	movReg64Immi64(binBlock, rcx, gMemoryAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, fRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_R_FM(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	cvtss2siReg32XMM(binBlock, rcx, xmm0);
	movMReg32Reg32(binBlock, rax, rcx);
}

void X86DynaRecCore::FMOD_FM_R(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 regAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movReg64Immi64(binBlock, rcx, regAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_MR_FM(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movRAX_MOffset(binBlock, mRegAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg32Reg32(binBlock, rax, ecx);
}

void X86DynaRecCore::FMOD_FM_MR(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gFMemoryAddr);
	movRAX_MOffset(binBlock, mRegAddr);
	movssXMM_MReg32(binBlock, xmm0, rcx);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rax);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_M_FM(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);
	movReg64Immi64(binBlock, rcx, gFMemoryAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gMemoryAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg32Reg32(binBlock, rax, ecx);
}

void X86DynaRecCore::FMOD_FM_M(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movReg64Immi64(binBlock, rcx, gMemoryAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_R_MFR(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movReg64Immi64(binBlock, rcx, regAddr);
	movRAX_MOffset(binBlock, fMRegAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);
	movssXMM_MReg32(binBlock, xmm1, rax);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg64Reg64(binBlock, rax, rcx);
}

void X86DynaRecCore::FMOD_MFR_R(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);
	movReg64Immi64(binBlock, rcx, regAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movRAX_MOffset(binBlock, fMRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);	
}

void X86DynaRecCore::FMOD_MR_MFR(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, mRegAddr);
	movReg64Immi64(binBlock, rcx, fMRegAddr);
	movReg64MReg64(binBlock, rcx, rcx);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movRAX_MOffset(binBlock, mRegAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg32Reg32(binBlock, rax, ecx);
}

void X86DynaRecCore::FMOD_MFR_MR(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, fMRegAddr);
	movReg64Immi64(binBlock, rcx, mRegAddr);
	movReg64MReg64(binBlock, rcx, rcx);
	movssXMM_MReg32(binBlock, xmm0, rax);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movRAX_MOffset(binBlock, fMRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_M_MFR(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rcx, gMemoryAddr);
	movRAX_MOffset(binBlock, fMRegAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rcx);
	movssXMM_MReg32(binBlock, xmm1, rax);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gMemoryAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg32Reg32(binBlock, rax, ecx);
}

void X86DynaRecCore::FMOD_MFR_M(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, fMRegAddr);
	movReg64Immi64(binBlock, rcx, gMemoryAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	cvtsi2ssXMM_MReg32(binBlock, xmm1, rcx);
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movRAX_MOffset(binBlock, fMRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_FR_FIMMI(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_Disp32(binBlock, xmm1, 0);
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, fRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_FM_FIMMI(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 gFMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_Disp32(binBlock, xmm1, 0);
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gFMemoryAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_MFR_FIMMI(X86BinBlock *binBlock) {
	float (*fmodPtr)(float, float) = fmod;
	uint64 fMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, fMRegAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_Disp32(binBlock, xmm1, 0);
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movRAX_MOffset(binBlock, fMRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FMOD_R_FIMMI(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_Disp32(binBlock, xmm1, 0);
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg64Reg64(binBlock, rax, rcx);
}

void X86DynaRecCore::FMOD_M_FIMMI(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 gMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 4];

	movReg64Immi64(binBlock, rax, gMemoryAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_Disp32(binBlock, xmm1, 0);
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movReg64Immi64(binBlock, rax, gMemoryAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg32Reg32(binBlock, rax, ecx);
}

void X86DynaRecCore::FMOD_MR_FIMMI(X86BinBlock *binBlock) {
	float32 (*fmodPtr)(float32, float32) = fmod;
	uint64 mRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	float32 fImmiValue = *(float32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, mRegAddr);
	cvtsi2ssXMM_MReg32(binBlock, xmm0, rax);
	movssXMM_Disp32(binBlock, xmm1, 0);
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(fImmiValue, binBlock->getCounter() - 4));
	movReg64Immi64(binBlock, rcx, (uint64)fmodPtr);
	callReg64(binBlock, rcx);
	movRAX_MOffset(binBlock, mRegAddr);
	cvtss2siReg32XMM(binBlock, ecx, xmm0);
	movMReg32Reg32(binBlock, rax, ecx);
}

void X86DynaRecCore::BMOV_R_BM(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 bMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movAL_MOffset(binBlock, bMemoryAddr);
	andRAX_Immi32(binBlock, 0xFF);
	movMOffsetRAX(binBlock, regAddr);
}

void X86DynaRecCore::BMOV_BM_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 bMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, regAddr);
	movMOffsetAL(binBlock, bMemoryAddr);
}

void X86DynaRecCore::BMOV_R_MBR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 bMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, bMRegAddr);
	movReg8MReg8(binBlock, al, rax);
	andRAX_Immi32(binBlock, 0xFF);
	movMOffsetRAX(binBlock, regAddr);
}

void X86DynaRecCore::BMOV_MBR_R(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 bMRegAddr = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, regAddr);
	movReg64Immi64(binBlock, rcx, bMRegAddr);
	movReg64MReg64(binBlock, rcx, rcx);
	movMReg8Reg8(binBlock, rcx, al);
}

void X86DynaRecCore::BMOV_MBR_MBR(X86BinBlock *binBlock) {
	uint64 bMRegAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 bMRegAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movRAX_MOffset(binBlock, bMRegAddr2);
	movReg32MReg32(binBlock, eax, rax);
	movReg64Immi64(binBlock, rcx, bMRegAddr1);
	movReg64MReg64(binBlock, rcx, rcx);
	movMReg8Reg8(binBlock, rcx, al);
}

void X86DynaRecCore::BMOV_BM_BM(X86BinBlock *binBlock) {
	uint64 bMemoryAddr1 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint64 bMemoryAddr2 = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC + 4];

	movAL_MOffset(binBlock, bMemoryAddr2);
	movReg64Immi64(binBlock, rcx, bMemoryAddr1);
	movMReg8Reg8(binBlock, rcx, al);
}

void X86DynaRecCore::BMOV_MBR_IMMI8(X86BinBlock *binBlock) {
	uint64 bMRegAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint8 immiValue = memManager.codeSpace[pC + 1];

	movRAX_MOffset(binBlock, bMRegAddr);
	movMReg8Immi8(binBlock, rax, immiValue);
}

void X86DynaRecCore::BMOV_BM_IMMI8(X86BinBlock *binBlock) {
	uint64 bMemoryAddr = (uint64)memManager.globalDataSpace + *(uint32*)&memManager.codeSpace[pC];
	uint8 immiValue = memManager.codeSpace[pC];

	movReg64Immi64(binBlock, rax, bMemoryAddr);
	movMReg8Immi8(binBlock, rax, immiValue);
}

void X86DynaRecCore::CMPE_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movEAX_MOffset(binBlock, regAddr2);
	movReg64Immi64(binBlock, rcx, regAddr1);
	cmpMReg32Reg32(binBlock, rcx, eax);
	jeRel32(binBlock, movMReg64Immi32(0, rcx, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rcx, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rcx, 1));
	movMReg64Immi32(binBlock, rcx, 1);
}

void X86DynaRecCore::CMPNE_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movEAX_MOffset(binBlock, regAddr2);
	movReg64Immi64(binBlock, rcx, regAddr1);
	cmpMReg32Reg32(binBlock, rcx, eax);
	jeRel32(binBlock, movMReg64Immi32(0, rcx, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rcx, 1);
	jmpRel32(binBlock, movMReg64Immi32(0, rcx, 1));
	movMReg64Immi32(binBlock, rcx, 0);
}

void X86DynaRecCore::CMPG_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movEAX_MOffset(binBlock, regAddr2);
	movReg64Immi64(binBlock, rcx, regAddr1);
	cmpMReg32Reg32(binBlock, rcx, eax);
	jgRel32(binBlock, movMReg64Immi32(0, rcx, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rcx, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rcx, 1));
	movMReg64Immi32(binBlock, rcx, 1);
}

void X86DynaRecCore::CMPL_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movEAX_MOffset(binBlock, regAddr2);
	movReg64Immi64(binBlock, rcx, regAddr1);
	cmpMReg32Reg32(binBlock, rcx, eax);
	jlRel32(binBlock, movMReg64Immi32(0, rcx, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rcx, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rcx, 1));
	movMReg64Immi32(binBlock, rcx, 1);
}

void X86DynaRecCore::CMPGE_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movEAX_MOffset(binBlock, regAddr2);
	movReg64Immi64(binBlock, rcx, regAddr1);
	cmpMReg32Reg32(binBlock, rcx, eax);
	jgeRel32(binBlock, movMReg64Immi32(0, rcx, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rcx, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rcx, 1));
	movMReg64Immi32(binBlock, rcx, 1);
}

void X86DynaRecCore::CMPLE_R_R(X86BinBlock *binBlock) {
	uint64 regAddr1 = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 regAddr2 = (uint64)regs + (memManager.codeSpace[pC + 1] << 3);

	movEAX_MOffset(binBlock, regAddr2);
	movReg64Immi64(binBlock, rcx, regAddr1);
	cmpMReg32Reg32(binBlock, rcx, eax);
	jleRel32(binBlock, movMReg64Immi32(0, rcx, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rcx, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rcx, 1));
	movMReg64Immi32(binBlock, rcx, 1);
}

void X86DynaRecCore::FCMPE_R_FR_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 2] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr1);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movReg64Immi64(binBlock, rcx, fRegAddr2);
	ucomissXMM_MReg32(binBlock, xmm0, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	jeRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(binBlock, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rax, 1));
	movMReg64Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::FCMPNE_R_FR_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 2] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr1);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movReg64Immi64(binBlock, rcx, fRegAddr2);
	ucomissXMM_MReg32(binBlock, xmm0, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	jeRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(binBlock, 0));
	movMReg64Immi32(binBlock, rax, 1);
	jmpRel32(binBlock, movMReg64Immi32(0, rax, 1));
	movMReg64Immi32(binBlock, rax, 0);
}

void X86DynaRecCore::FCMPG_R_FR_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 2] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr1);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movReg64Immi64(binBlock, rcx, fRegAddr2);
	ucomissXMM_MReg32(binBlock, xmm0, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	jaRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(binBlock, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rax, 1));
	movMReg64Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::FCMPL_R_FR_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 2] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr1);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movReg64Immi64(binBlock, rcx, fRegAddr2);
	ucomissXMM_MReg32(binBlock, xmm0, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	jbRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(binBlock, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rax, 1));
	movMReg64Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::FCMPGE_R_FR_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 2] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr1);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movReg64Immi64(binBlock, rcx, fRegAddr2);
	ucomissXMM_MReg32(binBlock, xmm0, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	jaeRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(binBlock, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rax, 1));
	movMReg64Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::FCMPLE_R_FR_FR(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint64 fRegAddr1 = (uint64)fRegs + (memManager.codeSpace[pC + 1] << 2);
	uint64 fRegAddr2 = (uint64)fRegs + (memManager.codeSpace[pC + 2] << 2);

	movReg64Immi64(binBlock, rax, fRegAddr1);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movReg64Immi64(binBlock, rcx, fRegAddr2);
	ucomissXMM_MReg32(binBlock, xmm0, rcx);
	movReg64Immi64(binBlock, rax, regAddr);
	jbeRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(binBlock, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg64Immi32(0, rax, 1));
	movMReg64Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::FOUT_IMMI_FR(X86BinBlock *binBlock) {
	void (*writePortPtr)(float32, uint32, PortManager*) = PortManager::writePortAsFloat;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, fRegAddr);
	movssXMM_MReg32(binBlock, xmm0, rax);
	movReg32Immi32(binBlock, edx, immiValue);
	movReg64Immi64(binBlock, r8, (uint64)&portManager);
	movReg64Immi64(binBlock, rax, (uint64)writePortPtr);
	callReg64(binBlock, rax);
}

void X86DynaRecCore::FIN_FR_IMMI(X86BinBlock *binBlock) {
	float32 (*readPortPtr)(uint32, PortManager*) = PortManager::readPortAsFloat;
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg32Immi32(binBlock, rcx, immiValue);
	movReg64Immi64(binBlock, rdx, (uint64)&portManager);
	movReg64Immi64(binBlock, rax, (uint64)readPortPtr);
	callReg64(binBlock, rax);
	movReg64Immi64(binBlock, rax, fRegAddr);
	movssMReg32XMM(binBlock, rax, xmm0);
}

void X86DynaRecCore::FOUT_IMMI_FIMMI(X86BinBlock *binBlock) {
	void (*writePortPtr)(float32, uint32, PortManager*) = PortManager::writePortAsFloat;
	uint32 immiValue1 = *(uint32*)&memManager.codeSpace[pC];
	float32 immiValue2 = *(float32*)&memManager.codeSpace[pC + 4];

	movssXMM_Disp32(binBlock, xmm0, 0);
	if (immediateFloat) immediateFloat->push_back(ImmediateFloat(immiValue2, binBlock->getCounter() - 4));
	movReg32Immi32(binBlock, edx, immiValue1);
	movReg64Immi64(binBlock, r8, (uint64)&portManager);
	movReg64Immi64(binBlock, rax, (uint64)writePortPtr);
	callReg64(binBlock, rax);
}

void X86DynaRecCore::PUSHES_R_R(X86BinBlock *binBlock) {
	int regIndex1 = memManager.codeSpace[pC];
	int regIndex2 = memManager.codeSpace[pC + 1];
	
	int numOfRegsToPush = regIndex2 - regIndex1 + 1;
	uint64 sourceAddr = (uint64)regs + (regIndex1 << 3);
	
	movReg64Immi64(binBlock, rax, (uint64)&sP);
	movReg64Immi64(binBlock, rsi, sourceAddr);
	movReg64Immi64(binBlock, rdi, (uint64)memManager.stackSpace);
	movReg32MReg32(binBlock, ecx, rax);
	addReg64Reg64(binBlock, rdi, rcx);
	movReg32Immi32(binBlock, ecx, numOfRegsToPush);
	repMovs64(binBlock);
	addMReg32Immi32(binBlock, rax, numOfRegsToPush << 3);
}

void X86DynaRecCore::POPS_R_R(X86BinBlock *binBlock) {
	int regIndex1 = memManager.codeSpace[pC];
	int regIndex2 = memManager.codeSpace[pC + 1];
	
	int numOfRegsToPop = regIndex2 - regIndex1 + 1;
	uint64 destinationAddr = (uint64)regs + (regIndex1 << 3);
	
	movReg64Immi64(binBlock, rax, (uint64)&sP);
	subMReg32Immi32(binBlock, rax, numOfRegsToPop << 3);
	movReg64Immi64(binBlock, rsi, (uint64)memManager.stackSpace);
	movReg32MReg32(binBlock, eax, rax);
	addReg64Reg64(binBlock, rsi, rax);
	movReg64Immi64(binBlock, rdi, destinationAddr);
	movReg32Immi32(binBlock, ecx, numOfRegsToPop);
	repMovs64(binBlock);
}

void X86DynaRecCore::FPUSHES_FR_FR(X86BinBlock *binBlock) {
	int fRegIndex1 = memManager.codeSpace[pC];
	int fRegIndex2 = memManager.codeSpace[pC + 1];
	int numOfRegsToPush = fRegIndex2 - fRegIndex1 + 1;
	uint64 sourceAddr = (uint64)fRegs + (fRegIndex1 << 2);

	movReg64Immi64(binBlock, rax, (uint64)&sP);
	movReg64Immi64(binBlock, rsi, sourceAddr);
	movReg64Immi64(binBlock, rdi, (uint64)memManager.stackSpace);
	movReg32MReg32(binBlock, ecx, rax);
	addReg64Reg64(binBlock, rdi, rcx);
	movReg32Immi32(binBlock, ecx, numOfRegsToPush);
	repMovs32(binBlock);
	addMReg32Immi32(binBlock, rax, numOfRegsToPush << 2);
}

void X86DynaRecCore::FPOPS_FR_FR(X86BinBlock *binBlock) {
	int fRegIndex1 = memManager.codeSpace[pC];
	int fRegIndex2 = memManager.codeSpace[pC + 1];
	
	int numOfRegsToPop = fRegIndex2 - fRegIndex1 + 1;
	uint64 destinationAddr = (uint64)fRegs + (fRegIndex1 << 2);

	movReg64Immi64(binBlock, rax, (uint64)&sP);
	subMReg32Immi32(binBlock, rax, numOfRegsToPop << 2);
	movReg64Immi64(binBlock, rsi, (uint64)memManager.stackSpace);
	movReg32MReg32(binBlock, eax, rax);
	addReg64Reg64(binBlock, rsi, rax);
	movReg64Immi64(binBlock, rdi, destinationAddr);
	movReg32Immi32(binBlock, ecx, numOfRegsToPop);
	repMovs32(binBlock);
}

void X86DynaRecCore::FPUSH_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 stackAddr = (uint64)memManager.stackSpace;
	uint64 stackPointerAddr = (uint64)&sP;

	movEAX_MOffset(binBlock, fRegAddr);	//mov rax, (fRegAddr)
	movReg64Immi64(binBlock, rdx, stackAddr);	//mov rdx, stackAddr
	movReg64Immi64(binBlock, rcx, stackPointerAddr);	//mov rcx, stackPointerAddr
	movReg32MReg32(binBlock, ebx, rcx);	//mov ebx, (rcx)
	movMReg32Reg32(binBlock, 0, ebx, rdx, eax);	//mov (rdx + ebx), eax
	addMReg32Immi8(binBlock, rcx, 4);	//add (rcx), 4
}

void X86DynaRecCore::FPOP_FR(X86BinBlock *binBlock) {
	uint64 fRegAddr = (uint64)fRegs + (memManager.codeSpace[pC] << 2);
	uint64 stackAddr = (uint64)memManager.stackSpace;
	uint64 stackPointerAddr = (uint64)&sP;

	movReg64Immi64(binBlock, rdx, stackAddr);	//mov rdx, stackAddr
	movReg64Immi64(binBlock, rcx, stackPointerAddr);	//mov rcx, stackPointerAddr
	subMReg32Immi8(binBlock, rcx, 4);	//sub (rcx), 4
	movReg32MReg32(binBlock, ebx, rcx);	//mov ebx, (rcx)
	movReg32MReg32(binBlock, eax, 0, ebx, rdx);	//mov eax, (rdx + ebx)
	movMOffsetEAX(binBlock, fRegAddr);	//mov (fRegAddr), rax
}

void X86DynaRecCore::CMPE_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);
	cmpMReg32Immi32(binBlock, rax, immiValue);
	jeRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg32Immi32(0, rax, 1));
	movMReg32Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::CMPNE_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);
	cmpMReg32Immi32(binBlock, rax, immiValue);
	jeRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rax, 1);
	jmpRel32(binBlock, movMReg32Immi32(0, rax, 1));
	movMReg32Immi32(binBlock, rax, 0);
}

void X86DynaRecCore::CMPG_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);
	cmpMReg32Immi32(binBlock, rax, immiValue);
	jgRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg32Immi32(0, rax, 1));
	movMReg32Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::CMPGE_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);
	cmpMReg32Immi32(binBlock, rax, immiValue);
	jgeRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg32Immi32(0, rax, 1));
	movMReg32Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::CMPL_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);
	cmpMReg32Immi32(binBlock, rax, immiValue);
	jlRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg32Immi32(0, rax, 1));
	movMReg32Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::CMPLE_R_IMMI(X86BinBlock *binBlock) {
	uint64 regAddr = (uint64)regs + (memManager.codeSpace[pC] << 3);
	uint32 immiValue = *(uint32*)&memManager.codeSpace[pC + 1];

	movReg64Immi64(binBlock, rax, regAddr);
	cmpMReg32Immi32(binBlock, rax, immiValue);
	jleRel32(binBlock, movMReg64Immi32(0, rax, 0) + jmpRel32(0, 0));
	movMReg64Immi32(binBlock, rax, 0);
	jmpRel32(binBlock, movMReg32Immi32(0, rax, 1));
	movMReg32Immi32(binBlock, rax, 1);
}

void X86DynaRecCore::TIME(X86BinBlock *binBlock) {
	uint64 (*timerPtr)(DespairTimer*) = timer.getMilliseconds;

	movReg64Immi64(binBlock, rcx, (uint64)&timer);
	movReg64Immi64(binBlock, rax, (uint64)timerPtr);
	callReg64(binBlock, rax);
	movReg64Immi64(binBlock, rcx, (uint64)regs);
	movMReg64Reg64(binBlock, rcx, rax);
}

void X86DynaRecCore::SLEEP(X86BinBlock *binBlock) {
#ifdef BUILD_FOR_WINDOWS
	void (_stdcall *sleepPtr)(DWORD) = Sleep;

	movReg64Immi64(binBlock, rcx, 1);
	movReg64Immi64(binBlock, rax, (uint64)sleepPtr);
	callReg64(binBlock, rax);
#endif
}

void X86DynaRecCore::RAND(X86BinBlock *binBlock) {
	int (_cdecl *randPtr)() = rand;
	uint64 regAddr0 = (uint64)&regs[0];

	movReg64Immi64(binBlock, rax, (uint64)randPtr);
	callReg64(binBlock, rax);
	movMOffsetRAX(binBlock, regAddr0);
}

void X86DynaRecCore::JMP_IMMI() {
	pC += 2;
	pC = *(uint32*)&memManager.codeSpace[pC];
}

void X86DynaRecCore::JMPR_IMMI() {
	pC += 2;
	pC += *(int32*)&memManager.codeSpace[pC] + 4;
}

void X86DynaRecCore::JC_R_IMMI() {
	pC += 2;
	if (regs[memManager.codeSpace[pC]] == 0) {
		pC = *(uint32*)&memManager.codeSpace[pC + 1];
	} else {
		pC += 5;
	}
}

void X86DynaRecCore::JCR_R_IMMI() {
	pC += 2;
	if (regs[memManager.codeSpace[pC]] == 0) {
		pC += *(int32*)&memManager.codeSpace[pC + 1] + 5;
	} else {
		pC += 5;
	}
}

void X86DynaRecCore::CALL_IMMI() {
	*(uint32*)&memManager.stackSpace[sP] = pC + 6;
	sP += 4;
	pC = *(uint32*)&memManager.codeSpace[pC + 2];
}

void X86DynaRecCore::RET() {
	sP -= 4;
	if (sP < 0) {
		pC = -1;
	} else {
		pC = *(uint32*)&memManager.stackSpace[sP];
	}
}