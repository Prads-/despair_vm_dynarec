/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2012 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "x86Emitter.h"

unsigned char modRM(int mod, int reg, int rm) {
	return ((mod << 6) | (reg << 3) | rm);
}

int X86Emitter::addRegImmi8(X86BinBlock *binBlock, X86Register destReg, unsigned char immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x83);
		binBlock->write<uint8>(modRM(3, 0, destReg));
		binBlock->write<uint8>(immi);
	}

	return 3;
}

int X86Emitter::addRegReg(X86BinBlock *binBlock, X86Register destReg, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(3);
		binBlock->write<uint8>(modRM(3, destReg, reg));
	}

	return 2;
}

int X86Emitter::addMem8Immi8(X86BinBlock *binBlock, unsigned int memAddr, unsigned char immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x80);
		binBlock->write<uint8>(modRM(0, 0, DISP32));
		binBlock->write<uint32>(memAddr);
		binBlock->write<uint8>(immi);
	}

	return 7;
}

int X86Emitter::addMemImmi8(X86BinBlock *binBlock, unsigned int memAddr, unsigned char immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x83);
		binBlock->write<uint8>(modRM(0, 0, DISP32));
		binBlock->write<uint32>(memAddr);
		binBlock->write<uint8>(immi);
	}

	return 7;
}

int X86Emitter::addMem8Reg8(X86BinBlock *binBlock, unsigned int memAddr, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(0);
		binBlock->write<uint8>(modRM(0, reg, DISP32));
		binBlock->write<uint32>(memAddr);
	}

	return 6;
}

int X86Emitter::addMemReg(X86BinBlock *binBlock, unsigned int memAddr, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(1);
		binBlock->write<uint8>(modRM(0, reg, DISP32));
		binBlock->write<uint32>(memAddr);
	}

	return 6;
}

int X86Emitter::andReg8Immi8(X86BinBlock *binBlock, X86Register destReg, unsigned char immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x80);
		binBlock->write<uint8>(modRM(3, 4, destReg));
		binBlock->write<uint8>(immi);
	}

	return 3;
}

int X86Emitter::andRegImmi(X86BinBlock *binBlock, X86Register destReg, unsigned int immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(3, 4, destReg));
		binBlock->write<uint32>(immi);
	}

	return 6;
}

int X86Emitter::andMem8Reg8(X86BinBlock *binBlock, unsigned int destMem, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(0x20);
		binBlock->write<uint8>(modRM(0, reg, DISP32));
		binBlock->write<uint32>(destMem);
	}

	return 6;
}

int X86Emitter::callNearAbsolute(X86BinBlock *binBlock, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(0xff);
		binBlock->write<uint8>(modRM(3, 2, reg));
	}

	return 2;
}

int X86Emitter::cmpRegImmi8(X86BinBlock *binBlock, X86Register reg, unsigned char immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x83);
		binBlock->write<uint8>(modRM(3, 7, reg));
		binBlock->write<uint8>(immi);
	}

	return 3;
}

int X86Emitter::cmpReg8Mem8(X86BinBlock *binBlock, X86Register reg, unsigned int memAddr) {
	if (binBlock) {
		binBlock->write<uint8>(0x3A);
		binBlock->write<uint8>(modRM(0, reg, DISP32));
		binBlock->write<uint32>(memAddr);
	}

	return 6;
}

int X86Emitter::cmpMem8Immi8(X86BinBlock *binBlock, unsigned int memAddr, unsigned char immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x80);
		binBlock->write<uint8>(modRM(0, 7, DISP32));
		binBlock->write<uint32>(memAddr);
		binBlock->write<uint8>(immi);
	}

	return 7;
}

int X86Emitter::decMem8(X86BinBlock *binBlock, unsigned int destMem) {
	if (binBlock) {
		binBlock->write<uint8>(0xFE);
		binBlock->write<uint8>(modRM(0, 1, DISP32));
		binBlock->write<uint32>(destMem);
	}

	return 6;
}

int X86Emitter::decMem(X86BinBlock *binBlock, unsigned int destMem) {
	if (binBlock) {
		binBlock->write<uint8>(0xFF);
		binBlock->write<uint8>(modRM(0, 1, DISP32));
		binBlock->write<uint32>(destMem);
	}

	return 6;
}

int X86Emitter::decReg8(X86BinBlock *binBlock, X86Register destReg) {
	if (binBlock) {
		binBlock->write<uint8>(0xFE);
		binBlock->write<uint8>(modRM(3, 1, destReg));
	}

	return 2;
}

int X86Emitter::decReg(X86BinBlock *binBlock, X86Register destReg) {
	if (binBlock) {
		binBlock->write<uint8>(0x48 + destReg);
	}

	return 1;
}

int X86Emitter::divReg8(X86BinBlock *binBlock, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(0xF6);
		binBlock->write<uint8>(modRM(3, 6, reg));
	}

	return 2;
}

int X86Emitter::incReg32(X86BinBlock *binBlock, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(0xFF);
		binBlock->write<uint8>(modRM(3, 0, reg));
	}

	return 2;
}

int X86Emitter::jc8(X86BinBlock *binBlock, char rel) {
	if (binBlock) {
		binBlock->write<uint8>(0x72);
		binBlock->write<uint8>(rel);
	}

	return 2;
}

int X86Emitter::je8(X86BinBlock *binBlock, char rel) {
	if (binBlock) {
		binBlock->write<uint8>(0x74);
		binBlock->write<uint8>(rel);
	}

	return 2;
}

int X86Emitter::jne8(X86BinBlock *binBlock, char rel) {
	if (binBlock) {
		binBlock->write<uint8>(0x75);
		binBlock->write<uint8>(rel);
	}

	return 2;
}

int X86Emitter::jg8(X86BinBlock *binBlock, char rel) {
	if (binBlock) {
		binBlock->write<uint8>(0x7F);
		binBlock->write<uint8>(rel);
	}

	return 2;
}

int X86Emitter::jmp8(X86BinBlock *binBlock, char rel) {
	if (binBlock) {
		binBlock->write<uint8>(0xEB);
		binBlock->write<uint8>(rel);
	}

	return 2;
}

int X86Emitter::lahf(X86BinBlock *binBlock) {
	if (binBlock) binBlock->write<uint8>(0x9F);

	return 1;
}

int X86Emitter::loop(X86BinBlock *binBlock, char rel) {
	if (binBlock) {
		binBlock->write<uint8>(0xE2);
		binBlock->write<uint8>(rel);
	}

	return 2;
}

int X86Emitter::movReg8Immi8(X86BinBlock *binBlock, X86Register destReg, unsigned char immi) {
	if (binBlock) {
		binBlock->write<uint8>(0xB0 + destReg);
		binBlock->write<uint8>(immi);
	}

	return 2;
}

int X86Emitter::movRegImmi(X86BinBlock *binBlock, X86Register destReg, unsigned int immi) {
	if (binBlock) {
		binBlock->write<uint8>(0xB8 + destReg);
		binBlock->write<uint32>(immi);
	}

	return 5;
}

int X86Emitter::movMem8Immi8(X86BinBlock *binBlock, unsigned int destMem, unsigned char immi) {
	if (binBlock) {
		binBlock->write<uint8>(0xC6);
		binBlock->write<uint8>(modRM(0, 0, DISP32));
		binBlock->write<uint32>(destMem);
		binBlock->write<uint8>(immi);
	}

	return 7;
}

int X86Emitter::movMemImmi(X86BinBlock *binBlock, unsigned int destMem, unsigned int immi) {
	if (binBlock) {
		binBlock->write<uint8>(0xC7);
		binBlock->write<uint8>(modRM(0, 0, DISP32));
		binBlock->write<uint32>(destMem);
		binBlock->write<uint32>(immi);
	}

	return 10;
}

int X86Emitter::movReg8Mem8(X86BinBlock *binBlock, X86Register destReg, unsigned int srcMem) {
	if (binBlock) {
		binBlock->write<uint8>(0x8A);
		binBlock->write<uint8>(modRM(0, destReg, DISP32));
		binBlock->write<uint32>(srcMem);
	}

	return 6;
}

int X86Emitter::movReg8Mem8(X86BinBlock *binBlock, X86Register destReg, X86Register srcMem) {
	if (binBlock) {
		binBlock->write<uint8>(0x8A);
		binBlock->write<uint8>(modRM(0, destReg, srcMem));
	}

	return 2;
}

int X86Emitter::movMem8Reg8(X86BinBlock *binBlock, unsigned int destMem, X86Register srcReg) {
	if (binBlock) {
		binBlock->write<uint8>(0x88);
		binBlock->write<uint8>(modRM(0, srcReg, DISP32));
		binBlock->write<uint32>(destMem);
	}

	return 6;
}

int X86Emitter::movMem8Reg8(X86BinBlock *binBlock, X86Register destMem, X86Register srcReg) {
	if (binBlock) {
		binBlock->write<uint8>(0x88);
		binBlock->write<uint8>(modRM(0, srcReg, destMem));
	}

	return 2;
}

int X86Emitter::movReg32Mem32(X86BinBlock *binBlock, X86Register destReg, unsigned int srcMem) {
	if (binBlock) {
		binBlock->write<uint8>(0x8B);
		binBlock->write<uint8>(modRM(0, destReg, DISP32));
		binBlock->write<uint32>(srcMem);
	}

	return 6;
}

int X86Emitter::movMem32Reg32(X86BinBlock *binBlock, unsigned int destMem, X86Register srcReg) {
	if (binBlock) {
		binBlock->write<uint8>(0x89);
		binBlock->write<uint8>(modRM(0, srcReg, DISP32));
		binBlock->write<uint32>(destMem);
	}

	return 6;
}

int X86Emitter::movzxRegMem8(X86BinBlock *binBlock, X86Register destReg, unsigned int srcMem) {
	if (binBlock) {
		binBlock->write<uint8>(0xF);
		binBlock->write<uint8>(0xB6);
		binBlock->write<uint8>(modRM(0, destReg, DISP32));
		binBlock->write<uint32>(srcMem);
	}

	return 7;
}

int X86Emitter::movzxRegMem16(X86BinBlock *binBlock, X86Register destReg, unsigned int srcMem) {
	if (binBlock) {
		binBlock->write<uint8>(0xF);
		binBlock->write<uint8>(0xB7);
		binBlock->write<uint8>(modRM(0, destReg, DISP32));
		binBlock->write<uint32>(srcMem);
	}

	return 7;
}

int X86Emitter::mulReg8(X86BinBlock *binBlock, X86Register srcReg) {
	if (binBlock) {
		binBlock->write<uint8>(0xF6);
		binBlock->write<uint8>(modRM(3, 4, srcReg));
	}

	return 2;
}

int X86Emitter::notReg8(X86BinBlock *binBlock, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(0xF6);
		binBlock->write<uint8>(modRM(3, 2, reg));
	}

	return 2;
}

int X86Emitter::orMem8Reg8(X86BinBlock *binBlock, unsigned int destMem, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(8);
		binBlock->write<uint8>(modRM(0, reg, DISP32));
		binBlock->write<uint32>(destMem);
	}

	return 6;
}

void X86Emitter::ret(X86BinBlock *binBlock) {
	if (binBlock) binBlock->write<uint8>(0xc3);
}

int X86Emitter::shlMem8(X86BinBlock *binBlock, unsigned int memAddr) {
	if (binBlock) {
		binBlock->write<uint8>(0xD0);
		binBlock->write<uint8>(modRM(0, 4, DISP32));
		binBlock->write<uint32>(memAddr);
	}

	return 6;
}

int X86Emitter::shrReg8Immi8(X86BinBlock *binBlock, X86Register reg, unsigned int immi) {
	if (binBlock) {
		binBlock->write<uint8>(0xC0);
		binBlock->write<uint8>(modRM(3, 5, reg));
		binBlock->write<uint8>(immi);
	}

	return 3;
}

int X86Emitter::shrMem8(X86BinBlock *binBlock, unsigned int memAddr) {
	if (binBlock) {
		binBlock->write<uint8>(0xD0);
		binBlock->write<uint8>(modRM(0, 5, DISP32));
		binBlock->write<uint32>(memAddr);
	}

	return 6;
}

int X86Emitter::subRegReg(X86BinBlock *binBlock, X86Register destReg, X86Register srcReg) {
	if (binBlock) {
		binBlock->write<uint8>(0x2B);
		binBlock->write<uint8>(modRM(3, destReg, srcReg));
	}

	return 2;
}

int X86Emitter::xorRegReg(X86BinBlock *binBlock, X86Register destReg, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(0x31);
		binBlock->write<uint8>(modRM(3, reg, destReg));
	}

	return 2;
}

int X86Emitter::xorMem8Reg8(X86BinBlock *binBlock, unsigned int destMem, X86Register reg) {
	if (binBlock) {
		binBlock->write<uint8>(0x30);
		binBlock->write<uint8>(modRM(0, reg, DISP32));
		binBlock->write<uint32>(destMem);
	}

	return 6;
}