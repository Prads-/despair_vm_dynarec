/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef X86_DYNAMIC_RECOMPILER_H
#define X86_DYNAMIC_RECOMPILER_H

#include <vector>
#include <map>
#include <string>
#include "build.h"
#include "declarations.h"
#include "x86BinBlock.h"
#include "memoryManager.h"
#include "timer.h"
#include "gpuCore.h"
#include "portManager.h"
#include "despairHeader.h"

typedef std::map<int64, X86BinBlock*> X86BinBlockCache;

struct ImmediateFloat {
	float32 value;
	uint32 counter;

	ImmediateFloat(float value, uint32 counter) {
		this->value = value;
		this->counter = counter;
	}
};

class X86DynaRecCore {
private:
	int64 pC;
	int64 sP;
	int64 regs[256];
	float32 fRegs[256];
	MemoryManager memManager;
	static DespairTimer timer;
	GPUCore *gpuCore;
	PortManager portManager;
	X86BinBlockCache x86BinBlockCache;
	std::vector<ImmediateFloat> *immediateFloat;
	
	void putDrawOpcode(X86BinBlock *binBlock, uint64 xAddr, uint64 yAddr, uint64 imgAddr);
	static void draw(int x, int y, uint64 address, X86DynaRecCore *dynarecCore);
	
	void createNewBinBlock();
	void executeBlock(X86BinBlock *binBlock);
	void putImmediateFloats(X86BinBlock *binBlock);
	void makeShadowSpace(X86BinBlock *binBlock);
	int fdCycle(X86BinBlock *binBlock);

	void MOV_R_MR_IMMI(X86BinBlock *binBlock);
	void MOV_MR_IMMI_R(X86BinBlock *binBlock);
	void MOV_MR_IMMI_MR_IMMI(X86BinBlock *binBlock);
	void MOV_MR_IMMI_IMMI(X86BinBlock *binBlock);
	void MOV_R_M(X86BinBlock *binBlock);
	void MOV_M_R(X86BinBlock *binBlock);
	void MOV_R_R(X86BinBlock *binBlock);
	void MOV_M_M(X86BinBlock *binBlock);
	void MOV_MR_R(X86BinBlock *binBlock);
	void MOV_R_MR(X86BinBlock *binBlock);
	void MOV_MR_M(X86BinBlock *binBlock);
	void MOV_M_MR(X86BinBlock *binBlock);
	void MOV_MR_MR(X86BinBlock *binBlock);
	void MOV_R_IMMI(X86BinBlock *binBlock);
	void MOV_M_IMMI(X86BinBlock *binBlock);
	void MOV_MR_IMMI(X86BinBlock *binBlock);

	void ADD_R_R(X86BinBlock *binBlock);
	void ADD_R_IMMI(X86BinBlock *binBlock);
	
	void SUB_R_R(X86BinBlock *binBlock);
	void SUB_R_IMMI(X86BinBlock *binBlock);
	
	void MUL_R_R(X86BinBlock *binBlock);
	void MUL_R_IMMI(X86BinBlock *binBlock);
	
	void DIV_R_R(X86BinBlock *binBlock);
	void DIV_R_IMMI(X86BinBlock *binBlock);
	
	void MOD_R_R(X86BinBlock *binBlock);
	void MOD_R_IMMI(X86BinBlock *binBlock);
	
	void AND_R_R(X86BinBlock *binBlock);
	void AND_R_IMMI(X86BinBlock *binBlock);
	
	void OR_R_R(X86BinBlock *binBlock);
	void OR_R_IMMI(X86BinBlock *binBlock);
	
	void XOR_R_R(X86BinBlock *binBlock);
	void XOR_R_IMMI(X86BinBlock *binBlock);
	
	void SHL_R_IMMI8(X86BinBlock *binBlock);
	void SHL_R_R(X86BinBlock *binBlock);
	
	void SHR_R_IMMI8(X86BinBlock *binBlock);
	void SHR_R_R(X86BinBlock *binBlock);
	
	void NOP(X86BinBlock *binBlock);
	
	void MOVP_R_MR_IMMI(X86BinBlock *binBlock);
	void MOVP_MR_IMMI_R(X86BinBlock *binBlock);
	void MOVP_MR_IMMI_MR_IMMI(X86BinBlock *binBlock);
	void MOVP_R_M(X86BinBlock *binBlock);
	void MOVP_M_R(X86BinBlock *binBlock);
	void MOVP_R_MR(X86BinBlock *binBlock);
	void MOVP_MR_R(X86BinBlock *binBlock);

	void PUSH_R(X86BinBlock *binBlock);
	void POP_R(X86BinBlock *binBlock);

	void DRW_R_R_MR(X86BinBlock *binBlock);

	void OUT_R_IMMI8(X86BinBlock *binBlock);
	void OUT_R_IMMI16(X86BinBlock *binBlock);
	void OUT_R_IMMI32(X86BinBlock *binBlock);
	void OUT_R_IMMI64(X86BinBlock *binBlock);
	void OUT_IMMI_R8(X86BinBlock *binBlock);
	void OUT_IMMI_R16(X86BinBlock *binBlock);
	void OUT_IMMI_R32(X86BinBlock *binBlock);
	void OUT_IMMI_R64(X86BinBlock *binBlock);
	void OUT_R_R8(X86BinBlock *binBlock);
	void OUT_R_R16(X86BinBlock *binBlock);
	void OUT_R_R32(X86BinBlock *binBlock);
	void OUT_R_R64(X86BinBlock *binBlock);
	void OUT_IMMI_IMMI8(X86BinBlock *binBlock);
	void OUT_IMMI_IMMI16(X86BinBlock *binBlock);
	void OUT_IMMI_IMMI32(X86BinBlock *binBlock);
	void OUT_IMMI_IMMI64(X86BinBlock *binBlock);

	void IN_R8_IMMI(X86BinBlock *binBlock);
	void IN_R16_IMMI(X86BinBlock *binBlock);
	void IN_R32_IMMI(X86BinBlock *binBlock);
	void IN_R64_IMMI(X86BinBlock *binBlock);
	void IN_R8_R(X86BinBlock *binBlock);
	void IN_R16_R(X86BinBlock *binBlock);
	void IN_R32_R(X86BinBlock *binBlock);
	void IN_R64_R(X86BinBlock *binBlock);

	void FCON_R_FR(X86BinBlock *binBlock);
	void FCON_FR_R(X86BinBlock *binBlock);
	void FCON_FR_IMMI(X86BinBlock *binBlock);
	void FCON_R_FIMMI(X86BinBlock *binBlock);
	
	void FMOV_FR_MFR_IMMI(X86BinBlock *binBlock);
	void FMOV_MFR_IMMI_FR(X86BinBlock *binBlock);
	void FMOV_MFR_IMMI_MFR_IMMI(X86BinBlock *binBlock);
	void FMOV_MFR_IMMI_FIMMI(X86BinBlock *binBlock);
	void FMOV_FR_FR(X86BinBlock *binBlock);
	void FMOV_FR_FM(X86BinBlock *binBlock);
	void FMOV_FM_FR(X86BinBlock *binBlock);
	void FMOV_FR_MFR(X86BinBlock *binBlock);
	void FMOV_MFR_FR(X86BinBlock *binBlock);
	void FMOV_FM_MFR(X86BinBlock *binBlock);
	void FMOV_MFR_FM(X86BinBlock *binBlock);
	void FMOV_MFR_MFR(X86BinBlock *binBlock);
	void FMOV_FM_FM(X86BinBlock *binBlock);
	void FMOV_FR_FIMMI(X86BinBlock *binBlock);
	void FMOV_FM_FIMMI(X86BinBlock *binBlock);
	void FMOV_MFR_FIMMI(X86BinBlock *binBlock);

	void FADD_FR_FR(X86BinBlock *binBlock);
	void FADD_R_FR(X86BinBlock *binBlock);
	void FADD_FR_R(X86BinBlock *binBlock);
	void FADD_FR_FIMMI(X86BinBlock *binBlock);
	void FADD_R_FIMMI(X86BinBlock *binBlock);
	
	void FSUB_FR_FR(X86BinBlock *binBlock);
	void FSUB_R_FR(X86BinBlock *binBlock);
	void FSUB_FR_R(X86BinBlock *binBlock);
	void FSUB_FR_FIMMI(X86BinBlock *binBlock);
	void FSUB_R_FIMMI(X86BinBlock *binBlock);
	
	void FMUL_FR_FR(X86BinBlock *binBlock);
	void FMUL_R_FR(X86BinBlock *binBlock);
	void FMUL_FR_R(X86BinBlock *binBlock);
	void FMUL_FR_FIMMI(X86BinBlock *binBlock);
	void FMUL_R_FIMMI(X86BinBlock *binBlock);
	
	void FDIV_FR_FR(X86BinBlock *binBlock);
	void FDIV_R_FR(X86BinBlock *binBlock);
	void FDIV_FR_R(X86BinBlock *binBlock);
	void FDIV_FR_FIMMI(X86BinBlock *binBlock);
	void FDIV_R_FIMMI(X86BinBlock *binBlock);
	
	void FMOD_FR_FR(X86BinBlock *binBlock);
	void FMOD_R_FR(X86BinBlock *binBlock);
	void FMOD_FR_R(X86BinBlock *binBlock);
	void FMOD_FR_FIMMI(X86BinBlock *binBlock);
	void FMOD_R_FIMMI(X86BinBlock *binBlock);
	
	void BMOV_R_MBR_IMMI(X86BinBlock *binBlock);
	void BMOV_MBR_IMMI_R(X86BinBlock *binBlock);
	void BMOV_MBR_IMMI_MBR_IMMI(X86BinBlock *binBlock);
	void BMOV_MBR_IMMI_IMMI8(X86BinBlock *binBlock);
	void BMOV_R_BM(X86BinBlock *binBlock);
	void BMOV_BM_R(X86BinBlock *binBlock);
	void BMOV_R_MBR(X86BinBlock *binBlock);
	void BMOV_MBR_R(X86BinBlock *binBlock);
	void BMOV_MBR_MBR(X86BinBlock *binBlock);
	void BMOV_BM_BM(X86BinBlock *binBlock);
	void BMOV_MBR_IMMI8(X86BinBlock *binBlock);
	void BMOV_BM_IMMI8(X86BinBlock *binBlock);

	void CMPE_R_R(X86BinBlock *binBlock);
	void CMPNE_R_R(X86BinBlock *binBlock);
	void CMPG_R_R(X86BinBlock *binBlock);
	void CMPL_R_R(X86BinBlock *binBlock);
	void CMPGE_R_R(X86BinBlock *binBlock);
	void CMPLE_R_R(X86BinBlock *binBlock);
	void FCMPE_R_FR_FR(X86BinBlock *binBlock);
	void FCMPNE_R_FR_FR(X86BinBlock *binBlock);
	void FCMPG_R_FR_FR(X86BinBlock *binBlock);
	void FCMPL_R_FR_FR(X86BinBlock *binBlock);
	void FCMPGE_R_FR_FR(X86BinBlock *binBlock);
	void FCMPLE_R_FR_FR(X86BinBlock *binBlock);

	void FOUT_IMMI_FR(X86BinBlock *binBlock);
	void FIN_FR_IMMI(X86BinBlock *binBlock);
	void FOUT_IMMI_FIMMI(X86BinBlock *binBlock);

	void PUSHES_R_R(X86BinBlock *binBlock);
	void POPS_R_R(X86BinBlock *binBlock);

	void FPUSHES_FR_FR(X86BinBlock *binBlock);
	void FPOPS_FR_FR(X86BinBlock *binBlock);
	void FPUSH_FR(X86BinBlock *binBlock);
	void FPOP_FR(X86BinBlock *binBlock);

	void CMPE_R_IMMI(X86BinBlock *binBlock);
	void CMPNE_R_IMMI(X86BinBlock *binBlock);
	void CMPG_R_IMMI(X86BinBlock *binBlock);
	void CMPGE_R_IMMI(X86BinBlock *binBlock);
	void CMPL_R_IMMI(X86BinBlock *binBlock);
	void CMPLE_R_IMMI(X86BinBlock *binBlock);

	void TIME(X86BinBlock *binBlock);
	void SLEEP(X86BinBlock *binBlock);

	void RAND(X86BinBlock *binBlock);

	//These instructions are interpreted
	void JMP_IMMI();
	void JMPR_IMMI();
	void JC_R_IMMI();
	void JCR_R_IMMI();
	void CALL_IMMI();
	void RET();

public:
	X86DynaRecCore(uint8 *codePtr, uint8 *globalDataPtr, uint32 codeStartIndex, uint64 paramAddr, GPUCore *gpuCore, DespairHeader::ExecutableHeader *header, KeyboardManager *keyboardManager);
	~X86DynaRecCore();

	void startCPULoop();
};

#endif