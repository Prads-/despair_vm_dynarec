/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "x86_64Emitter.h"

uint8 modRM(int mod, int reg, int rm) {
	return ((mod << 6) | (reg << 3) | rm);
}

uint8 sib(int scale, int index, int base) {
	return ((scale << 6) | (index << 3) | base);
}

int X86_64Emitter::addReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;
	
	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(3);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::addReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(3);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return 3;
}

int X86_64Emitter::addEAX_Immi32(X86BinBlock *binBlock, uint32 immi) {
	if (binBlock) {
		binBlock->write<uint8>(5);
		binBlock->write<uint32>(immi);
	}

	return 5;
}

int X86_64Emitter::addRAX_Immi32(X86BinBlock *binBlock, uint32 immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x48);
		binBlock->write<uint8>(5);
		binBlock->write<uint32>(immi);
	}

	return 6;
}

int X86_64Emitter::addReg64Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	if (binBlock) {
		int rex;
		
		if (reg > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(3, 0, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return 7;
}

int X86_64Emitter::addReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(3, 0, reg & 7)); 
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}

int X86_64Emitter::addMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	
	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(0, 0, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}

int X86_64Emitter::addMReg32Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0x83);
		binBlock->write<uint8>(modRM(0, 0, reg & 7));
		binBlock->write<uint8>(immi);
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::addMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 4;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(1);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::addMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 4;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(1);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return 3;
}

int X86_64Emitter::addReg32MReg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(3);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::addReg64MReg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 & 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(3);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
	}

	return 3;
}

int X86_64Emitter::addssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x580F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::addssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(0x44);
		}
		binBlock->write<uint16>(0x580F);
		binBlock->write<uint8>(modRM(0, xmm & 7, DISP32));
		binBlock->write<uint32>(disp32);
	}

	return (rex == 0x40) ? 8 : 9;
}

int X86_64Emitter::andReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x23);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::andReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x23);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return 3;
}

int X86_64Emitter::andEAX_Immi32(X86BinBlock *binBlock, uint32 immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x25);
		binBlock->write<uint32>(immi);
	}

	return 5;
}

int X86_64Emitter::andRAX_Immi32(X86BinBlock *binBlock, uint32 immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x48);
		binBlock->write<uint8>(0x25);
		binBlock->write<uint32>(immi);
	}

	return 6;
}

int X86_64Emitter::andMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 4;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x21);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::andMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 4;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x21);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return 3;
}

int X86_64Emitter::andMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(0, 4, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}

int X86_64Emitter::callReg64(X86BinBlock *binBlock, X86_64Register reg) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xFF);
		binBlock->write<uint8>(modRM(3, 2, reg & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::cmpMReg64Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x49);
		} else {
			binBlock->write<uint8>(0x48);
		}
		binBlock->write<uint8>(0x83);
		binBlock->write<uint8>(modRM(0, 7, reg & 7));
		binBlock->write<uint8>(immi);
	}

	return 4;
}

int X86_64Emitter::cmpMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(0, 7, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (reg > 7) ? 7 : 6;
}

int X86_64Emitter::cmpMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 4;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x39);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::cmpMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 4;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x39);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return 3;
}

int X86_64Emitter::cvtsi2ssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x2A0F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::cvtsi2ssXMM_MReg64(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	if (binBlock) {
		int rex;

		if (xmm > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(0xF3);
		binBlock->write<uint8>(rex);
		binBlock->write<uint16>(0x2A0F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return 5;
}

int X86_64Emitter::cvtsi2ssXMM_Reg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x2A0F);
		binBlock->write<uint8>(modRM(3, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::cvtss2siReg32MReg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x2D0F);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::cvtss2siReg32Disp32(X86BinBlock *binBlock, X86_64Register reg, uint32 disp32) {
	int rex;

	if (reg > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(0x44);
		}
		binBlock->write<uint16>(0x2D0F);
		binBlock->write<uint8>(modRM(0, reg & 7, DISP32));
		binBlock->write<uint32>(disp32);
	}
	return (rex == 0x40) ? 8 : 9;
}

int X86_64Emitter::cvtss2siReg32XMM(X86BinBlock *binBlock, X86_64Register reg, X86_64Register xmm) {
	int rex;

	if (reg > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (xmm > 0) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x2D0F);
		binBlock->write<uint8>(modRM(3, reg & 7, xmm & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::divssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x5E0F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::divssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	
	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(0x44);
		}
		binBlock->write<uint16>(0x5E0F);
		binBlock->write<uint8>(modRM(0, xmm & 7, DISP32));
		binBlock->write<uint32>(disp32);
	}

	return (rex == 0x40) ? 8 : 9;
}

int X86_64Emitter::divssXMM_XMM(X86BinBlock *binBlock, X86_64Register xmm_1, X86_64Register xmm_2) {
	int rex;

	if (xmm_1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (xmm_2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x5E0F);
		binBlock->write<uint8>(modRM(3, xmm_1 & 7, xmm_2 & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::idivEAX_Reg32(X86BinBlock *binBlock, X86_64Register reg) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0xF7);
		binBlock->write<uint8>(modRM(3, 7, reg & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::idivRAX_Reg64(X86BinBlock *binBlock, X86_64Register reg) {
	if (binBlock) {
		int rex;
		
		if (reg > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0xF7);
		binBlock->write<uint8>(modRM(3, 7, reg & 7));
	}

	return 3;
}

int X86_64Emitter::imulEAX_Reg32(X86BinBlock *binBlock, X86_64Register reg) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		binBlock->write<uint8>(0x41);
		binBlock->write<uint8>(0xF7);
		binBlock->write<uint8>(modRM(3, 5, reg & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::imulRAX_Reg64(X86BinBlock *binBlock, X86_64Register reg) {
	if (binBlock) {
		int rex;

		if (reg > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0xF7);
		binBlock->write<uint8>(modRM(3, 5, reg & 7));
	}

	return 3;
}

int X86_64Emitter::imulReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0xAF0F);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::imulReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint16>(0xAF0F);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return 4;
}

int X86_64Emitter::imulReg64Reg64Immi32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2, uint32 immi) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x69);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
		binBlock->write<uint32>(immi);
	}

	return 7;
}

int X86_64Emitter::imulReg32Reg32Immi32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2, uint32 immi) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x69);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}

int X86_64Emitter::imulReg32MReg32Immi32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2, uint32 immi) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x69);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}

int X86_64Emitter::imulReg32MReg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0xAF0F);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::imulReg64MReg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint16>(0xAF0F);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
	}

	return 4;
}

int X86_64Emitter::jaRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x870F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jaeRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x830F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jbRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x820F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jbeRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x860F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jeRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x840F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jgRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x8F0F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jgeRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x8D0F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jlRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x8C0F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jleRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint16>(0x8E0F);
		binBlock->write<uint32>(rel);
	}

	return 6;
}

int X86_64Emitter::jmpRel32(X86BinBlock *binBlock, uint32 rel) {
	if (binBlock) {
		binBlock->write<uint8>(0xE9);
		binBlock->write<uint32>(rel);
	}

	return 5;
}

int X86_64Emitter::movReg64Immi64(X86BinBlock *binBlock, X86_64Register reg, uint64 immi) {
	if (binBlock) {
		uint8 rex;
		
		if (reg > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}
		
		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0xB8 + (reg & 7));
		binBlock->write<uint64>(immi);
	}

	return 10;
}

int X86_64Emitter::movReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xB8 + (reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 5 : 6;
}

int X86_64Emitter::movReg16Immi16(X86BinBlock *binBlock, X86_64Register reg, uint16 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		binBlock->write<uint8>(0x66);
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xB8 + (reg & 7));
		binBlock->write<uint16>(immi);
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::movReg8Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xB0 + (reg & 7));
		binBlock->write<uint8>(immi);
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::movMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		uint8 rex;
		
		if (reg1 > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 4;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x89);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return 3;
}

int X86_64Emitter::movReg64MReg64(X86BinBlock *binBlock, X86_64Register reg, int scale, X86_64Register index, X86_64Register base) {
	if (binBlock) {
		int rex;

		if (reg > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (index > 7) {
			rex |= 2;
		}
		if (base > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x8B);
		binBlock->write<uint8>(modRM(0, reg & 7, SIB_BYTE));
		binBlock->write<uint8>(sib(scale, index & 7, base & 7));
	}

	return 4;
}

int X86_64Emitter::movMReg64Reg64(X86BinBlock *binBlock, int scale, X86_64Register index, X86_64Register base, X86_64Register reg) {
	if (binBlock) {
		int rex;

		if (reg > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (index > 7) {
			rex |= 2;
		}
		if (base > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x89);
		binBlock->write<uint8>(modRM(0, reg & 7, SIB_BYTE));
		binBlock->write<uint8>(sib(scale, index & 7, base & 7));
	}

	return 4;
}

int X86_64Emitter::movMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 4;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x89);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::movMReg32Reg32(X86BinBlock *binBlock, int scale, X86_64Register index, X86_64Register base, X86_64Register reg) {
	int rex;

	if (reg > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (index > 7) {
		rex |= 2;
	}
	if (base > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x89);
		binBlock->write<uint8>(modRM(0, reg & 7, SIB_BYTE));
		binBlock->write<uint8>(sib(scale, index & 7, base & 7));
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::movMReg32Reg32(X86BinBlock *binBlock, int scale, X86_64Register index, X86_64Register base, uint8 mImmi, X86_64Register reg) {
	int rex;

	if (reg > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (index > 7) {
		rex |= 2;
	}
	if (base > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x89);
		binBlock->write<uint8>(modRM(1, reg & 7, SIB_BYTE));
		binBlock->write<uint8>(sib(scale, index & 7, base & 7));
		binBlock->write<uint8>(mImmi);
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::movReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex = 0x40;

	if (reg1 > 7) {
		rex |= 4;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x8B);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::movReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;
		
		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x8B);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return 3;
}

int X86_64Emitter::movMOffsetRAX(X86BinBlock *binBlock, uint64 mOffset) {
	if (binBlock) {
		binBlock->write<uint8>(0x48);
		binBlock->write<uint8>(0xA3);
		binBlock->write<uint64>(mOffset);
	}

	return 10;
}

int X86_64Emitter::movMOffsetEAX(X86BinBlock *binBlock, uint64 mOffset) {
	if (binBlock) {
		binBlock->write<uint8>(0xA3);
		binBlock->write<uint64>(mOffset);
	}

	return 9;
}

int X86_64Emitter::movMOffsetAL(X86BinBlock *binBlock, uint64 mOffset) {
	if (binBlock) {
		binBlock->write<uint8>(0x48);
		binBlock->write<uint8>(0xA2);
		binBlock->write<uint64>(mOffset);
	}
	return 10;
}

int X86_64Emitter::movRAX_MOffset(X86BinBlock *binBlock, uint64 mOffset) {
	if (binBlock) {
		binBlock->write<uint8>(0x48);
		binBlock->write<uint8>(0xA1);
		binBlock->write<uint64>(mOffset);
	}

	return 10;
}

int X86_64Emitter::movEAX_MOffset(X86BinBlock *binBlock, uint64 mOffset) {
	if (binBlock) {
		binBlock->write<uint8>(0xA1);
		binBlock->write<uint64>(mOffset);
	}

	return 9;
}

int X86_64Emitter::movAX_MOffset(X86BinBlock *binBlock, uint64 mOffset) {
	if (binBlock) {
		binBlock->write<uint8>(0x66);
		binBlock->write<uint8>(0xA1);
		binBlock->write<uint64>(mOffset);
	}

	return 10;
}

int X86_64Emitter::movAL_MOffset(X86BinBlock *binBlock, uint64 mOffset) {
	if (binBlock) {
		binBlock->write<uint8>(0x48);
		binBlock->write<uint8>(0xA0);
		binBlock->write<uint64>(mOffset);
	}

	return 10;
}

int X86_64Emitter::movReg32MReg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex = 0x40;

	if (reg1 > 7) {
		rex |= 4;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x8B);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::movReg32MReg32(X86BinBlock *binBlock, X86_64Register reg, int scale, X86_64Register index, X86_64Register base) {
	int rex;

	if (reg > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (index > 7) {
		rex |= 2;
	}
	if (base > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x8B);
		binBlock->write<uint8>(modRM(0, reg & 7, SIB_BYTE));
		binBlock->write<uint8>(sib(scale, index & 7, base & 7));
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::movReg64MReg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x8B);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
	}

	return 3;
}

int X86_64Emitter::movReg8MReg8(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x8A);
		binBlock->write<uint8>(modRM(0, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::movMReg8Reg8(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 4;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x88);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::movMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xC7);
		binBlock->write<uint8>(modRM(0, 0, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (reg > 7) ? 7 : 6;
}

int X86_64Emitter::movMReg32Immi32(X86BinBlock *binBlock, int scale, X86_64Register index, X86_64Register base, uint8 mImmi, uint32 immi) {
	int rex;

	if (index > 7) {
		rex = 0x42;
	} else {
		rex = 0x40;
	}
	if (base > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0xC7);
		binBlock->write<uint8>(modRM(1, 0, SIB_BYTE));
		binBlock->write<uint8>(sib(scale, index & 7, base & 7));
		binBlock->write<uint8>(mImmi);
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 8 : 9;
}

int X86_64Emitter::movMReg64Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	if (binBlock) {
		int rex;

		if (reg > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0xC7);
		binBlock->write<uint8>(modRM(0, 0, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return 7;
}

int X86_64Emitter::movMReg8Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xC6);
		binBlock->write<uint8>(modRM(0, 0, reg & 7));
		binBlock->write<uint8>(immi);
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::movssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(0x44);
		}
		binBlock->write<uint16>(0x100F);
		binBlock->write<uint8>(modRM(0, xmm & 7, DISP32));
		binBlock->write<uint32>(disp32);
	}

	return (rex == 0x40) ? 8 : 9;
}

int X86_64Emitter::movssMReg32XMM(X86BinBlock *binBlock, X86_64Register reg, X86_64Register xmm) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (xmm > 7) {
		rex |= 4;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x110F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::movssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x100F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::mulssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x590F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::mulssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(0x44);
		}
		binBlock->write<uint16>(0x590F);
		binBlock->write<uint8>(modRM(0, xmm & 7, DISP32));
		binBlock->write<uint32>(disp32);
	}

	return (rex == 0x40) ? 8 : 9;
}

int X86_64Emitter::nop(X86BinBlock *binBlock) {
	if (binBlock) {
		binBlock->write<uint8>(0x90);
	}

	return 1;
}

int X86_64Emitter::orMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 4;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(9);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::orMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 4;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(9);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return 3;
}

int X86_64Emitter::orMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex =  0x40;
	}
	
	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(0, 1, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}

int X86_64Emitter::popReg64(X86BinBlock *binBlock, X86_64Register reg) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0x58 + (reg & 7));
	}

	return (reg > 7) ? 2 : 1;
}

int X86_64Emitter::pushf(X86BinBlock *binBlock) {
	if (binBlock) {
		binBlock->write<uint8>(0x9C);
	}

	return 1;
}

int X86_64Emitter::repMovs64(X86BinBlock *binBlock) {
	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		binBlock->write<uint8>(0x48);
		binBlock->write<uint8>(0xA5);
	}

	return 3;
}

int X86_64Emitter::repMovs32(X86BinBlock *binBlock) {
	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		binBlock->write<uint8>(0xA5);
	}

	return 2;
}

int X86_64Emitter::ret(X86BinBlock *binBlock) {
	if (binBlock) {
		binBlock->write<uint8>(0xC3);
	}

	return 1;
}

int X86_64Emitter::shlMReg32Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xC1);
		binBlock->write<uint8>(modRM(0, 4, reg & 7));
		binBlock->write<uint8>(immi);
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::shlMReg64Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x49);
		} else {
			binBlock->write<uint8>(0x48);
		}
		binBlock->write<uint8>(0xC1);
		binBlock->write<uint8>(modRM(0, 4, reg & 7));
		binBlock->write<uint8>(immi);
	}

	return 4;
}

int X86_64Emitter::shlMReg32Cl(X86BinBlock *binBlock, X86_64Register reg) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xD3);
		binBlock->write<uint8>(modRM(0, 4, reg & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::shlMReg64Cl(X86BinBlock *binBlock, X86_64Register reg) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x49);
		} else {
			binBlock->write<uint8>(0x48);
		}
		binBlock->write<uint8>(0xD3);
		binBlock->write<uint8>(modRM(0, 4, reg & 7));
	}

	return 3;
}

int X86_64Emitter::shrMReg32Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	int rex;
	
	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xC1);
		binBlock->write<uint8>(modRM(0, 5, reg & 7));
		binBlock->write<uint8>(immi);
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::shrMReg64Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x49);
		} else {
			binBlock->write<uint8>(0x48);
		}
		binBlock->write<uint8>(0xC1);
		binBlock->write<uint8>(modRM(0, 5, reg & 7));
		binBlock->write<uint8>(immi);
	}

	return 4;
}

int X86_64Emitter::shrMReg32Cl(X86BinBlock *binBlock, X86_64Register reg) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0xD3);
		binBlock->write<uint8>(modRM(0, 5, reg & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::shrMReg64Cl(X86BinBlock *binBlock, X86_64Register reg) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x49);
		} else {
			binBlock->write<uint8>(0x48);
		}
		binBlock->write<uint8>(0xD3);
		binBlock->write<uint8>(modRM(0, 5, reg & 7));
	}

	return 3;
}

int X86_64Emitter::subReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex = 0x40;

	if (reg1 > 7) {
		rex |= 4;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x2B);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::subReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x2B);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return 3;
}

int X86_64Emitter::subEAX_Immi32(X86BinBlock *binBlock, uint32 immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x2D);
		binBlock->write<uint32>(immi);
	}

	return 5;
}

int X86_64Emitter::subRAX_Immi32(X86BinBlock *binBlock, uint32 immi) {
	if (binBlock) {
		binBlock->write<uint8>(0x48);
		binBlock->write<uint8>(0x2D);
		binBlock->write<uint32>(immi);
	}

	return 6;
}

int X86_64Emitter::subReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		binBlock->write<uint8>(0x41);
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(3, 5, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}

int X86_64Emitter::subReg64Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	if (binBlock) {
		if (reg > 7) {
			binBlock->write<uint8>(0x49);
		} else {
			binBlock->write<uint8>(0x48);
		}
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(3, 5, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return 7;
}

int X86_64Emitter::subMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 4;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x29);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return 3;
}

int X86_64Emitter::subMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 4;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x29);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::subMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(0, 5, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}

int X86_64Emitter::subMReg32Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	
	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0x83);
		binBlock->write<uint8>(modRM(0, 5, reg & 7));
		binBlock->write<uint8>(immi);
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::subssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x5C0F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::subssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(0x44);
		}
		binBlock->write<uint16>(0x5C0F);
		binBlock->write<uint8>(modRM(0, xmm & 7, DISP32));
		binBlock->write<uint32>(disp32);
	}

	return (rex == 0x40) ? 8 : 9;
}

int X86_64Emitter::subssXMM_XMM(X86BinBlock *binBlock, X86_64Register xmm_1, X86_64Register xmm_2) {
	int rex;

	if (xmm_1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (xmm_2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		binBlock->write<uint8>(0xF3);
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x5C0F);
		binBlock->write<uint8>(modRM(3, xmm_1 & 7, xmm_2 & 7));
	}

	return (rex == 0x40) ? 4 : 5;
}

int X86_64Emitter::ucomissXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg) {
	int rex;

	if (xmm > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint16>(0x2E0F);
		binBlock->write<uint8>(modRM(0, xmm & 7, reg & 7));
	}

	return (rex == 0x40) ? 3 : 4;
}

int X86_64Emitter::xorReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x44;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 1;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x33);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::xorReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x4C;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 1;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x33);
		binBlock->write<uint8>(modRM(3, reg1 & 7, reg2 & 7));
	}

	return 3;
}

int X86_64Emitter::xorMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	int rex;

	if (reg1 > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	if (reg2 > 7) {
		rex |= 4;
	}

	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(rex);
		}
		binBlock->write<uint8>(0x31);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return (rex == 0x40) ? 2 : 3;
}

int X86_64Emitter::xorMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2) {
	if (binBlock) {
		int rex;

		if (reg1 > 7) {
			rex = 0x49;
		} else {
			rex = 0x48;
		}
		if (reg2 > 7) {
			rex |= 4;
		}

		binBlock->write<uint8>(rex);
		binBlock->write<uint8>(0x31);
		binBlock->write<uint8>(modRM(0, reg2 & 7, reg1 & 7));
	}

	return 3;
}

int X86_64Emitter::xorMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi) {
	int rex;

	if (reg > 7) {
		rex = 0x41;
	} else {
		rex = 0x40;
	}
	
	if (binBlock) {
		if (rex != 0x40) {
			binBlock->write<uint8>(0x41);
		}
		binBlock->write<uint8>(0x81);
		binBlock->write<uint8>(modRM(0, 6, reg & 7));
		binBlock->write<uint32>(immi);
	}

	return (rex == 0x40) ? 6 : 7;
}