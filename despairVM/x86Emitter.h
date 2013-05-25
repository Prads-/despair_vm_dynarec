/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2012 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef X86_EMITTER_H
#define X86_EMITTER_H

#include "build.h"
#include "declarations.h"
#include "x86BinBlock.h"

enum X86Register {
	eax = 0,
	ecx = 1,
	edx = 2,
	ebx = 3,
	esp	= 4,
	ebp = 5,
	esi = 6,
	edi = 7
};

#define DISP32			5

//16 bit regs
#define ax				eax
#define bx				ebx
#define cx				ecx
#define dx				edx

//lower 8 bit regs
#define al				eax
#define bl				ebx
#define cl				ecx
#define dl				edx

//higher 8 bit regs
//If this looks confusing, you need to look at Intel's instruction set reference volume 2, chapter 2, page 7 (Table 2-2)
#define ah				esp
#define bh				edi
#define ch				ebp
#define dh				esi

namespace X86Emitter {
	int addRegImmi8(X86BinBlock *binBlock, X86Register destReg, unsigned char immi);
	int addRegReg(X86BinBlock *binBlock, X86Register destReg, X86Register reg);
	int addMem8Immi8(X86BinBlock *binBlock, unsigned int memAddr, unsigned char immi);
	int addMemImmi8(X86BinBlock *binBlock, unsigned int memAddr, unsigned char immi);	//For both 16 and 32 bits mem
	int addMem8Reg8(X86BinBlock *binBlock, unsigned int memAddr, X86Register reg);
	int addMemReg(X86BinBlock *binBlock, unsigned int memAddr, X86Register reg);

	int andReg8Immi8(X86BinBlock *binBlock, X86Register destReg, unsigned char immi);
	int andRegImmi(X86BinBlock *binBlock, X86Register destReg, unsigned int immi);		//For both 16 and 32 bits mem
	int andMem8Reg8(X86BinBlock *binBlock, unsigned int destMem, X86Register reg);

	int callNearAbsolute(X86BinBlock *binBlock, X86Register reg);

	int cmpRegImmi8(X86BinBlock *binBlock, X86Register reg, unsigned char immi);
	int cmpReg8Mem8(X86BinBlock *binBlock, X86Register reg, unsigned int memAddr);
	int cmpMem8Immi8(X86BinBlock *binBlock, unsigned int memAddr, unsigned char immi);

	int decMem8(X86BinBlock *binBlock, unsigned int destMem);
	int decMem(X86BinBlock *binBlock, unsigned int destMem);
	int decReg8(X86BinBlock *binBlock, X86Register destReg);
	int decReg(X86BinBlock *binBlock, X86Register destReg);

	int divReg8(X86BinBlock *binBlock, X86Register reg);

	int incReg32(X86BinBlock *binBlock, X86Register reg);

	int jc8(X86BinBlock *binBlock, char rel);
	int je8(X86BinBlock *binBlock, char rel);
	int  jne8(X86BinBlock *binBlock, char rel);
	int jg8(X86BinBlock *binBlock, char rel);
	int jmp8(X86BinBlock *binBlock, char rel);

	int lahf(X86BinBlock *binBlock);
	int loop(X86BinBlock *binBlock, char rel);

	int movReg8Immi8(X86BinBlock *binBlock, X86Register destReg, unsigned char immi);
	int movRegImmi(X86BinBlock *binBlock, X86Register destReg, unsigned int immi);
	int movMem8Immi8(X86BinBlock *binBlock, unsigned int destMem, unsigned char immi);
	int movMemImmi(X86BinBlock *binBlock, unsigned int destMem, unsigned int immi);
	int movReg8Mem8(X86BinBlock *binBlock, X86Register destReg, unsigned int srcMem);
	int movReg8Mem8(X86BinBlock *binBlock, X86Register destReg, X86Register srcMem);
	int movMem8Reg8(X86BinBlock *binBlock, unsigned int destMem, X86Register srcReg);
	int movMem8Reg8(X86BinBlock *binBlock, X86Register destMem, X86Register srcReg);
	int movReg32Mem32(X86BinBlock *binBlock, X86Register destReg, unsigned int srcMem);
	int movMem32Reg32(X86BinBlock *binBlock, unsigned int destMem, X86Register srcReg);
	int movzxRegMem8(X86BinBlock *binBlock, X86Register destReg, unsigned int srcMem);
	int movzxRegMem16(X86BinBlock *binBlock, X86Register destReg, unsigned int srcMem);

	int mulReg8(X86BinBlock *binBlock, X86Register srcReg);

	int notReg8(X86BinBlock *binBlock, X86Register reg);

	int orMem8Reg8(X86BinBlock *binBlock, unsigned int destMem, X86Register reg);

	void ret(X86BinBlock *binBlock);

	int shlMem8(X86BinBlock *binBlock, unsigned int memAddr);
	int shrReg8Immi8(X86BinBlock *binBlock, X86Register reg, unsigned int immi);
	int shrMem8(X86BinBlock *binBlock, unsigned int memAddr);

	int subRegReg(X86BinBlock *binBlock, X86Register destReg, X86Register srcReg);

	int xorRegReg(X86BinBlock *binBlock, X86Register destReg, X86Register reg);
	int xorMem8Reg8(X86BinBlock *binBlock, unsigned int destMem, X86Register reg);
}

#endif