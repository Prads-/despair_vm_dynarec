/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef INSTRUCTIONS_SET_H
#define INSTRUCTIONS_SET_H

#define _MOV_R_R						0x0000
#define _MOV_MR_IMMI_R					0x0001
#define _MOV_R_MR_IMMI					0x0002
#define _MOV_MR_IMMI_MR_IMMI			0x0003
#define _MOV_R_IMMI						0x0004
#define _MOV_MR_IMMI_IMMI				0x0005
#define _MOV_R_M						0x0006
#define _MOV_M_R						0x0007
#define _MOV_M_M						0x0008
#define _MOV_MR_R						0x0009
#define _MOV_R_MR						0x000a
#define _MOV_MR_M						0x000b
#define _MOV_M_MR						0x000c
#define _MOV_MR_MR						0x000d

#define _MOV_M_IMMI						0x000e
#define _MOV_MR_IMMI					0x000f

#define _ADD_R_R						0x0010
#define _ADD_R_IMMI						0x0011

#define _SUB_R_R						0x0012
#define _SUB_R_IMMI						0x0013

#define _MUL_R_R						0x0014
#define _MUL_R_IMMI						0x0015

#define _DIV_R_R						0x0016
#define _DIV_R_IMMI						0x0017

#define _MOD_R_R						0x0018
#define _MOD_R_IMMI						0x0019

#define _AND_R_R						0x001a
#define _AND_R_IMMI						0x001b

#define _OR_R_R							0x001c
#define _OR_R_IMMI						0x001d

#define _XOR_R_R						0x001e
#define _XOR_R_IMMI						0x001f

#define _SHL_R_IMMI8					0x0020
#define _SHL_R_R						0x0021

#define _SHR_R_IMMI8					0x0022
#define _SHR_R_R						0x0023

#define _NOP							0x0024

#define _JMP_IMMI						0x0025
#define _JMPR_IMMI						0x0026
#define _JC_R_IMMI						0x0027
#define _JCR_R_IMMI						0x0028

#define _MOVP_R_MR_IMMI					0x0029
#define _MOVP_MR_IMMI_R					0x002a
#define _MOVP_MR_IMMI_MR_IMMI			0x002b
#define _MOVP_R_M						0x002c
#define _MOVP_M_R						0x002d
#define _MOVP_R_MR						0x002e
#define _MOVP_MR_R						0x002f

#define _CALL_IMMI						0x0030

#define _RET							0x0031

#define _PUSH_R							0x0032

#define _POP_R							0x0033

#define	_DRW_R_R_MR						0x0034

#define _OUT_R_IMMI8					0x0035
#define _OUT_R_IMMI16					0x0036
#define _OUT_R_IMMI32					0x0037
#define _OUT_R_IMMI64					0x0038
#define _OUT_IMMI_R8					0x0039
#define _OUT_IMMI_R16					0x003a
#define _OUT_IMMI_R32					0x003b
#define _OUT_IMMI_R64					0x003c
#define _OUT_R_R8						0x003d
#define _OUT_R_R16						0x003e
#define _OUT_R_R32						0x003f
#define _OUT_R_R64						0x0040
#define _OUT_IMMI_IMMI8					0x0041
#define _OUT_IMMI_IMMI16				0x0042
#define _OUT_IMMI_IMMI32				0x0043
#define _OUT_IMMI_IMMI64				0x0044

#define _IN_R8_IMMI						0x0045
#define _IN_R16_IMMI					0x0046
#define _IN_R32_IMMI					0x0047
#define _IN_R64_IMMI					0x0048
#define _IN_R8_R						0x0049
#define _IN_R16_R						0x004a
#define _IN_R32_R						0x004b
#define _IN_R64_R						0x004c

#define _FCON_R_FR						0x004d
#define _FCON_FR_R						0x004e
#define _FCON_FR_IMMI					0x004f
#define _FCON_R_FIMMI					0x0050

#define _FMOV_FR_FR						0x0051
#define _FMOV_FR_MFR_IMMI				0x0052
#define _FMOV_MFR_IMMI_FR				0x0053
#define _FMOV_MFR_IMMI_MFR_IMMI			0x0054
#define _FMOV_FR_FIMMI					0x0055
#define _FMOV_MFR_IMMI_FIMMI			0x0056
#define _FMOV_FR_FM						0x0057
#define _FMOV_FM_FR						0x0058
#define _FMOV_FR_MFR					0x0059
#define _FMOV_MFR_FR					0x005a
#define _FMOV_FM_MFR					0x005b
#define _FMOV_MFR_FM					0x005c
#define _FMOV_MFR_MFR					0x005d
#define _FMOV_FM_FM						0x005e
#define _FMOV_FM_FIMMI					0x005f
#define _FMOV_MFR_FIMMI					0x0060

#define _FADD_FR_FR						0x0061
#define _FADD_R_FR						0x0062
#define _FADD_FR_R						0x0063
#define _FADD_FR_FIMMI					0x0064
#define _FADD_R_FIMMI					0x0065

#define _FSUB_FR_FR						0x0066
#define _FSUB_R_FR						0x0067
#define _FSUB_FR_R						0x0068
#define _FSUB_FR_FIMMI					0x0069
#define _FSUB_R_FIMMI					0x006a

#define _FMUL_FR_FR						0x006b
#define _FMUL_R_FR						0x006c
#define _FMUL_FR_R						0x006d
#define _FMUL_FR_FIMMI					0x006e
#define _FMUL_R_FIMMI					0x006f

#define _FDIV_FR_FR						0x0070
#define _FDIV_R_FR						0x0071
#define _FDIV_FR_R						0x0072
#define _FDIV_FR_FIMMI					0x0073
#define _FDIV_R_FIMMI					0x0074

#define _FMOD_FR_FR						0x0075
#define _FMOD_R_FR						0x0076
#define _FMOD_FR_R						0x0077
#define _FMOD_FR_FIMMI					0x0078
#define _FMOD_R_FIMMI					0x0079

#define _BMOV_R_BR_IMMI					0x007a
#define _BMOV_BR_IMMI_R					0x007b
#define _BMOV_BR_IMMI_BR_IMMI			0x007c
#define _BMOV_BR_IMMI_IMMI8				0x007d
#define _BMOV_R_BM						0x007e
#define _BMOV_BM_R						0x007f
#define _BMOV_R_BR						0x0080
#define _BMOV_BR_R						0x0081
#define _BMOV_BR_BR						0x0082
#define _BMOV_BM_BM						0x0083
#define _BMOV_BR_IMMI8					0x0084
#define _BMOV_BM_IMMI8					0x0085

#define _CMPE_R_R						0x0086
#define _CMPNE_R_R						0x0087
#define _CMPG_R_R						0x0088
#define _CMPL_R_R						0x0089
#define _CMPGE_R_R						0x008a
#define _CMPLE_R_R						0x008b
#define _CMPE_R_FR_FR					0x008c
#define _CMPNE_R_FR_FR					0x008d
#define _CMPG_R_FR_FR					0x008e
#define _CMPL_R_FR_FR					0x008f
#define _CMPGE_R_FR_FR					0x0090
#define _CMPLE_R_FR_FR					0x0091

#define _FOUT_IMMI_FR					0x0092
#define _FIN_IMMI_FR					0x0093
#define _FOUT_IMMI_FIMMI				0x0094

#define _PUSHES_R_R						0x0095
#define _POPS_R_R						0x0096

#define _FPUSHES_FR_FR					0x0097
#define _FPOPS_FR_FR					0x0098
#define _FPUSH_FR						0x0099
#define _FPOP_FR						0x009a

#define _CMPE_R_IMMI					0x009b
#define _CMPNE_R_IMMI					0x009c
#define _CMPG_R_IMMI					0x009d
#define _CMPGE_R_IMMI					0x009e
#define _CMPL_R_IMMI					0x009f
#define _CMPLE_R_IMMI					0x00a0

#define _TIME							0x00a1
#define _SLEEP							0x00a2

#define _RAND							0x00a3

#endif
