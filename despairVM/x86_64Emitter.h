/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef X86_64_EMITTER_H
#define X86_64_EMITTER_H

#include "build.h"
#include "declarations.h"
#include "x86BinBlock.h"

enum X86_64Register {
	rax = 0,
	rcx = 1,
	rdx = 2,
	rbx = 3,
	rsp	= 4,
	rbp = 5,
	rsi = 6,
	rdi = 7,
	r8 = 8,
	r9 = 9,
	r10 = 10,
	r11 = 11,
	r12 = 12,
	r13 = 13,
	r14 = 14,
	r15 = 15
};

#define SIB_BYTE		4
#define DISP32			5

//32 bit registers
#define eax				rax
#define ecx				rcx
#define edx				rdx
#define ebx				rbx
#define esp				rsp
#define ebp				rbp
#define esi				rsi
#define edi				rdi

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

//XMM Registers
#define xmm0			rax
#define xmm1			rcx
#define xmm2			rdx
#define xmm3			rbx
#define xmm4			rsp
#define xmm5			rbp
#define xmm6			rsi
#define xmm7			rdi
#define xmm8			r8
#define xmm9			r9
#define xmm10			r10
#define xmm11			r11
#define xmm12			r12
#define xmm13			r13
#define xmm14			r14
#define xmm15			r15

namespace X86_64Emitter {
	//add reg, reg
	int addReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int addReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//add rax, immi32
	int addEAX_Immi32(X86BinBlock *binBlock, uint32 immi);
	int addRAX_Immi32(X86BinBlock *binBlock, uint32 immi);
	//add reg, immi
	int addReg64Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	int addReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	//add (reg), immi
	int addMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	int addMReg32Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);
	//add (reg), reg
	int addMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int addMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//add reg, (reg)
	int addReg32MReg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int addReg64MReg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);

	//addss xmm, (reg)
	int addssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);
	//addss xmm, disp32
	int addssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32);

	//and reg, reg
	int andReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int andReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//and rax, immi32
	int andEAX_Immi32(X86BinBlock *binBlock, uint32 immi);
	int andRAX_Immi32(X86BinBlock *binBlock, uint32 immi);
	//and (reg), reg
	int andMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int andMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//and (reg), immi
	int andMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);

	//call reg
	int callReg64(X86BinBlock *binBlock, X86_64Register reg);

	//cmp (reg), immi
	int cmpMReg64Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);
	int cmpMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	//cmp (reg), reg
	int cmpMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int cmpMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);

	//cvtsi2ss xmm, (reg)
	int cvtsi2ssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);
	int cvtsi2ssXMM_MReg64(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);
	//cvtsi2ss xmm, reg
	int cvtsi2ssXMM_Reg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);

	//cvtss2si reg, (reg)
	int cvtss2siReg32MReg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//cvtss2si reg, (rip + disp32)
	int cvtss2siReg32Disp32(X86BinBlock *binBlock, X86_64Register reg, uint32 disp32);
	//cvtss2si reg, xmm
	int cvtss2siReg32XMM(X86BinBlock *binBlock, X86_64Register reg, X86_64Register xmm);

	//divss xmm, (reg)
	int divssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);
	//divss xmm, disp32
	int divssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32);
	//divss xmm, xmm
	int divssXMM_XMM(X86BinBlock *binBlock, X86_64Register xmm_1, X86_64Register xmm_2);

	//idiv RAX, reg
	int idivEAX_Reg32(X86BinBlock *binBlock, X86_64Register reg);
	int idivRAX_Reg64(X86BinBlock *binBlock, X86_64Register reg);

	//imul RAX, reg
	int imulEAX_Reg32(X86BinBlock *binBlock, X86_64Register reg);
	int imulRAX_Reg64(X86BinBlock *binBlock, X86_64Register reg);
	//imul reg, reg
	int imulReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int imulReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//imul reg, reg, immi
	int imulReg64Reg64Immi32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2, uint32 immi);
	int imulReg32Reg32Immi32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2, uint32 immi);
	//imul reg, (reg), immi
	int imulReg32MReg32Immi32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2, uint32 immi);
	//imul reg, (reg)
	int imulReg32MReg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int imulReg64MReg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);

	//ja rel
	int jaRel32(X86BinBlock *binBlock, uint32 rel);
	//jae rel
	int jaeRel32(X86BinBlock *binBlock, uint32 rel);
	//jb rel
	int jbRel32(X86BinBlock *binBlock, uint32 rel);
	//jbe rel
	int jbeRel32(X86BinBlock *binBlock, uint32 rel);
	//je rel
	int jeRel32(X86BinBlock *binBlock, uint32 rel);
	//jg rel
	int jgRel32(X86BinBlock *binBlock, uint32 rel);
	//jge rel
	int jgeRel32(X86BinBlock *binBlock, uint32 rel);
	//jl rel
	int jlRel32(X86BinBlock *binBlock, uint32 rel);
	//jle rel
	int jleRel32(X86BinBlock *binBlock, uint32 rel);
	//jmp rel
	int jmpRel32(X86BinBlock *binBlock, uint32 rel);

	//mov reg, immi
	int movReg64Immi64(X86BinBlock *binBlock, X86_64Register reg, uint64 immi);
	int movReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	int movReg16Immi16(X86BinBlock *binBlock, X86_64Register reg, uint16 immi);
	int movReg8Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);
	//mov (reg), reg
	int movMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int movMReg64Reg64(X86BinBlock *binBlock, int scale, X86_64Register index, X86_64Register base, X86_64Register reg);
	int movMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int movMReg32Reg32(X86BinBlock *binBlock, int scale, X86_64Register index, X86_64Register base, X86_64Register reg);
	int movMReg32Reg32(X86BinBlock *binBlock, int scale, X86_64Register index, X86_64Register base, uint8 mImmi, X86_64Register reg);
	//mov reg, reg
	int movReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int movReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//mov moffset, rax
	int movMOffsetRAX(X86BinBlock *binBlock, uint64 mOffset);
	int movMOffsetEAX(X86BinBlock *binBlock, uint64 mOffset);
	int movMOffsetAL(X86BinBlock *binBlock, uint64 mOffset);
	//mov rax, moffset
	int movRAX_MOffset(X86BinBlock *binBlock, uint64 mOffset);
	int movEAX_MOffset(X86BinBlock *binBlock, uint64 mOffset);
	int movAX_MOffset(X86BinBlock *binBlock, uint64 mOffset);
	int movAL_MOffset(X86BinBlock *binBlock, uint64 mOffset);
	//mov reg, (reg)
	int movReg32MReg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int movReg32MReg32(X86BinBlock *binBlock, X86_64Register reg, int scale, X86_64Register index, X86_64Register base);
	int movReg64MReg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int movReg64MReg64(X86BinBlock *binBlock,  X86_64Register reg, int scale, X86_64Register index, X86_64Register base);
	int movReg8MReg8(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//mov (reg), reg
	int movMReg8Reg8(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//mov (reg), immi
	int movMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	int movMReg32Immi32(X86BinBlock *binBlock, int scale, X86_64Register index, X86_64Register base, uint8 mImmi, uint32 immi);
	int movMReg64Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	int movMReg8Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);

	//movss xmm, (RIP + disp32)
	int movssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32);
	//movss (reg), xmm
	int movssMReg32XMM(X86BinBlock *binBlock, X86_64Register reg, X86_64Register xmm);
	//movss xmm, (reg)
	int movssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);

	//mulss xmm, (reg)
	int mulssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);
	//mulss xmm, disp32
	int mulssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32);

	//nop
	int nop(X86BinBlock *binBlock);

	//or (reg), reg
	int orMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int orMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//or (reg), immi
	int orMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);

	//pop reg
	int popReg64(X86BinBlock *binBlock, X86_64Register reg);

	//pushf
	int pushf(X86BinBlock *binBlock);

	//repMovs
	int repMovs64(X86BinBlock *binBlock);
	int repMovs32(X86BinBlock *binBlock);

	//ret
	int ret(X86BinBlock *binBlock);

	//shl (reg), immi
	int shlMReg32Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);
	int shlMReg64Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);
	//shl (reg), cl
	int shlMReg32Cl(X86BinBlock *binBlock, X86_64Register reg);
	int shlMReg64Cl(X86BinBlock *binBlock, X86_64Register reg);

	//shr (reg), immi
	int shrMReg32Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);
	int shrMReg64Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);
	//shr (reg), cl
	int shrMReg32Cl(X86BinBlock *binBlock, X86_64Register reg);
	int shrMReg64Cl(X86BinBlock *binBlock, X86_64Register reg);

	//sub reg, reg
	int subReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int subReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//sub rax, immi
	int subEAX_Immi32(X86BinBlock *binBlock, uint32 immi);
	int subRAX_Immi32(X86BinBlock *binBlock, uint32 immi);
	//sub reg, immi
	int subReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	int subReg64Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	//sub (reg), reg
	int subMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int subMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//sub (reg), immi
	int subMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
	int subMReg32Immi8(X86BinBlock *binBlock, X86_64Register reg, uint8 immi);

	//subss xmm, (reg)
	int subssXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);
	//subss xmm, disp32
	int subssXMM_Disp32(X86BinBlock *binBlock, X86_64Register xmm, uint32 disp32);
	//subss xmm, xmm
	int subssXMM_XMM(X86BinBlock *binBlock, X86_64Register xmm_1, X86_64Register xmm_2);
	
	//comiss xmm, (reg)
	int ucomissXMM_MReg32(X86BinBlock *binBlock, X86_64Register xmm, X86_64Register reg);

	//xor reg, reg
	int xorReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int xorReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//xor (reg), reg
	int xorMReg32Reg32(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	int xorMReg64Reg64(X86BinBlock *binBlock, X86_64Register reg1, X86_64Register reg2);
	//xor (reg), immi
	int xorMReg32Immi32(X86BinBlock *binBlock, X86_64Register reg, uint32 immi);
}

#endif