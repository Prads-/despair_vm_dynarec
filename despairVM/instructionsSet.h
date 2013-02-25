/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef INSTRUCTIONS_SET_H
#define INSTRUCTIONS_SET_H

#define _MOV_R_M					0
#define _MOV_M_R					0x0001
#define _MOV_R_R					0x0002
#define _MOV_M_M					0x0003
#define _MOV_MR_R					0x0004
#define _MOV_R_MR					0x0005
#define _MOV_MR_M					0x0006
#define _MOV_M_MR					0x0007
#define _MOV_MR_MR					0x0008
#define _MOV_R_IMMI					0x0009
#define _MOV_M_IMMI					0x000A
#define _MOV_MR_IMMI				0x000B

#define _ADD_R_M					0x000C
#define _ADD_M_R					0x000D
#define _ADD_R_R					0x000E
#define _ADD_M_M					0x000F
#define _ADD_MR_R					0x0010
#define _ADD_R_MR					0x0011
#define _ADD_MR_M					0x0012
#define _ADD_M_MR					0x0013
#define _ADD_MR_MR					0x0014
#define _ADD_R_IMMI					0x0015
#define _ADD_M_IMMI					0x0016
#define _ADD_MR_IMMI				0x0017

#define _SUB_R_M					0x0018
#define _SUB_M_R					0x0019
#define _SUB_R_R					0x001A
#define _SUB_M_M					0x001B
#define _SUB_MR_R					0x001C
#define _SUB_R_MR					0x001D
#define _SUB_MR_M					0x001E
#define _SUB_M_MR					0x001F
#define _SUB_MR_MR					0x0020
#define _SUB_R_IMMI					0x0021
#define _SUB_M_IMMI					0x0022
#define _SUB_MR_IMMI				0x0023

#define _MUL_R_M					0x0024
#define _MUL_M_R					0x0025
#define _MUL_R_R					0x0026
#define _MUL_M_M					0x0027
#define _MUL_MR_R					0x0028
#define _MUL_R_MR					0x0029
#define _MUL_MR_M					0x002A
#define _MUL_M_MR					0x002B
#define _MUL_MR_MR					0x002C
#define _MUL_R_IMMI					0x002D
#define _MUL_M_IMMI					0x002E
#define _MUL_MR_IMMI				0x002F

#define _DIV_R_M					0x0030
#define _DIV_M_R					0x0031
#define _DIV_R_R					0x0032
#define _DIV_M_M					0x0033
#define _DIV_MR_R					0x0034
#define _DIV_R_MR					0x0035
#define _DIV_MR_M					0x0036
#define _DIV_M_MR					0x0037
#define _DIV_MR_MR					0x0038
#define _DIV_R_IMMI					0x0039
#define _DIV_M_IMMI					0x003A
#define _DIV_MR_IMMI				0x003B

#define _MOD_R_M					0x003C
#define _MOD_M_R					0x003D
#define _MOD_R_R					0x003E
#define _MOD_M_M					0x003F
#define _MOD_MR_R					0x0040
#define _MOD_R_MR					0x0041
#define _MOD_MR_M					0x0042
#define _MOD_M_MR					0x0043
#define _MOD_MR_MR					0x0044
#define _MOD_R_IMMI					0x0045
#define _MOD_M_IMMI					0x0046
#define _MOD_MR_IMMI				0x0047

#define _AND_R_M					0x0048
#define _AND_M_R					0x0049
#define _AND_R_R					0x004A
#define _AND_M_M					0x004B
#define _AND_MR_R					0x004C
#define _AND_R_MR					0x004D
#define _AND_MR_M					0x004E
#define _AND_M_MR					0x004F
#define _AND_MR_MR					0x0050
#define _AND_R_IMMI					0x0051
#define _AND_M_IMMI					0x0052
#define _AND_MR_IMMI				0x0053

#define _OR_R_M						0x0054
#define _OR_M_R						0x0055
#define _OR_R_R						0x0056
#define _OR_M_M						0x0057
#define _OR_MR_R					0x0058
#define _OR_R_MR					0x0059
#define _OR_MR_M					0x005A
#define _OR_M_MR					0x005B
#define _OR_MR_MR					0x005C
#define _OR_R_IMMI					0x005D
#define _OR_M_IMMI					0x005E
#define _OR_MR_IMMI					0x005F

#define _XOR_R_M					0x0060
#define _XOR_M_R					0x0061
#define _XOR_R_R					0x0062
#define _XOR_M_M					0x0063
#define _XOR_MR_R					0x0064
#define _XOR_R_MR					0x0065
#define _XOR_MR_M					0x0066
#define _XOR_M_MR					0x0067
#define _XOR_MR_MR					0x0068
#define _XOR_R_IMMI					0x0069
#define _XOR_M_IMMI					0x006A
#define _XOR_MR_IMMI				0x006B

#define _SHL_R_IMMI8				0x006C
#define _SHL_M_IMMI8				0x006D
#define _SHL_MR_IMMI8				0x006E
#define _SHL_R_R					0x006F
#define _SHL_M_R					0x0070
#define _SHL_MR_R					0x0071
#define _SHL_R_M					0x0072
#define _SHL_M_M					0x0073
#define _SHL_MR_M					0x0074
#define _SHL_R_MR					0x0075
#define _SHL_M_MR					0x0076
#define _SHL_MR_MR					0x0077

#define _SHR_R_IMMI8				0x0078
#define _SHR_M_IMMI8				0x0079
#define _SHR_MR_IMMI8				0x007A
#define _SHR_R_R					0x007B
#define _SHR_M_R					0x007C
#define _SHR_MR_R					0x007D
#define _SHR_R_M					0x007E
#define _SHR_M_M					0x007F
#define _SHR_MR_M					0x0080
#define _SHR_R_MR					0x0081
#define _SHR_M_MR					0x0082
#define _SHR_MR_MR					0x0083

#define _NOP						0x0084

#define _JMP_IMMI					0x0085
#define _JMPR_IMMI					0x0086
#define _JC_R_IMMI					0x0087
#define _JCR_R_IMMI					0x0088

#define _MOVP_R_M					0x0089
#define _MOVP_M_R					0x008A
#define _MOVP_R_MR					0x008B
#define _MOVP_MR_R					0x008C

#define _CALL_IMMI					0x008D

#define _RET						0x008E

#define _PUSH_R						0x008F

#define _POP_R						0x0090

#define	_DRW_R_R_R					0x0091
#define _DRW_R_R_M					0x0092

#define _OUT_R_IMMI8				0x0093
#define _OUT_R_IMMI16				0x0094
#define _OUT_R_IMMI32				0x0095
#define _OUT_R_IMMI64				0x0096
#define _OUT_IMMI_R8				0x0097
#define _OUT_IMMI_R16				0x0098
#define _OUT_IMMI_R32				0x0099
#define _OUT_IMMI_R64				0x009A
#define _OUT_R_R8					0x009B
#define _OUT_R_R16					0x009C
#define _OUT_R_R32					0x009D
#define _OUT_R_R64					0x009E
#define _OUT_IMMI_IMMI8				0x009F
#define _OUT_IMMI_IMMI16			0x00A0
#define _OUT_IMMI_IMMI32			0x00A1
#define _OUT_IMMI_IMMI64			0x00A2

#define _IN_R8_IMMI					0x00A3
#define _IN_R16_IMMI				0x00A4
#define _IN_R32_IMMI				0x00A5
#define _IN_R64_IMMI				0x00A6
#define _IN_R8_R					0x00A7
#define _IN_R16_R					0x00A8
#define _IN_R32_R					0x00A9
#define _IN_R64_R					0x00AA

#define _FCON_R_FR					0x00AB
#define _FCON_FR_R					0x00AC
#define _FCON_MR_FR					0x00AD
#define _FCON_FR_MR					0x00AE
#define _FCON_M_FR					0x00AF
#define _FCON_FR_M					0x00B0
#define _FCON_R_FM					0x00B1
#define _FCON_FM_R					0x00B2
#define _FCON_MR_FM					0x00B3
#define _FCON_FM_MR					0x00B4
#define _FCON_M_FM					0x00B5
#define _FCON_FM_M					0x00B6
#define _FCON_R_MFR					0x00B7
#define _FCON_MFR_R					0x00B8
#define _FCON_MR_MFR				0x00B9
#define _FCON_MFR_MR				0x00BA
#define _FCON_M_MFR					0x00BB
#define _FCON_MFR_M					0x00BC
#define _FCON_FR_IMMI				0x00BD
#define _FCON_FM_IMMI				0x00BE
#define _FCON_MFR_IMMI				0x00BF
#define _FCON_R_FIMMI				0x0171
#define _FCON_M_FIMMI				0x0172
#define _FCON_MR_FIMMI				0x0173

#define _FMOV_FR_FR					0x00C0
#define _FMOV_FR_FM					0x00C1
#define _FMOV_FM_FR					0x00C2
#define _FMOV_FR_MFR				0x00C3
#define _FMOV_MFR_FR				0x00C4
#define _FMOV_FM_MFR				0x00C5
#define _FMOV_MFR_FM				0x00C6
#define _FMOV_MFR_MFR				0x00C7
#define _FMOV_FM_FM					0x00C8
#define _FMOV_FR_FIMMI				0x00C9
#define _FMOV_FM_FIMMI				0x00CA
#define _FMOV_MFR_FIMMI				0x00CB

#define _FADD_FR_FR					0x00CC
#define _FADD_FR_FM					0x00CD
#define _FADD_FM_FR					0x00CE
#define _FADD_FR_MFR				0x00CF
#define _FADD_MFR_FR				0x00D0
#define _FADD_R_FR					0x00D1
#define _FADD_FR_R					0x00D2
#define _FADD_MR_FR					0x00D3
#define _FADD_FR_MR					0x00D4
#define _FADD_M_FR					0x00D5
#define _FADD_FR_M					0x00D6
#define _FADD_R_FM					0x00D7
#define _FADD_FM_R					0x00D8
#define _FADD_MR_FM					0x00D9
#define _FADD_FM_MR					0x00DA
#define _FADD_M_FM					0x00DB
#define _FADD_FM_M					0x00DC
#define _FADD_R_MFR					0x00DD
#define _FADD_MFR_R					0x00DE
#define _FADD_MR_MFR				0x00DF
#define _FADD_MFR_MR				0x00E0
#define _FADD_M_MFR					0x00E1
#define _FADD_MFR_M					0x00E2
#define _FADD_FR_FIMMI				0x00E3
#define _FADD_FM_FIMMI				0x00E4
#define _FADD_MFR_FIMMI				0x00E5
#define _FADD_R_FIMMI				0x00E6
#define _FADD_M_FIMMI				0x00E7
#define _FADD_MR_FIMMI				0x00E8

#define _FSUB_FR_FR					0x00E9
#define _FSUB_FR_FM					0x00EA
#define _FSUB_FM_FR					0x00EB
#define _FSUB_FR_MFR				0x00EC
#define _FSUB_MFR_FR				0x00ED
#define _FSUB_R_FR					0x00EE
#define _FSUB_FR_R					0x00EF
#define _FSUB_MR_FR					0x00F0
#define _FSUB_FR_MR					0x00F1
#define _FSUB_M_FR					0x00F2
#define _FSUB_FR_M					0x00F3
#define _FSUB_R_FM					0x00F4
#define _FSUB_FM_R					0x00F5
#define _FSUB_MR_FM					0x00F6
#define _FSUB_FM_MR					0x00F7
#define _FSUB_M_FM					0x00F8
#define _FSUB_FM_M					0x00F9
#define _FSUB_R_MFR					0x00FA
#define _FSUB_MFR_R					0x00FB
#define _FSUB_MR_MFR				0x00FC
#define _FSUB_MFR_MR				0x00FD
#define _FSUB_M_MFR					0x00FE
#define _FSUB_MFR_M					0x00FF
#define _FSUB_FR_FIMMI				0x0100
#define _FSUB_FM_FIMMI				0x0101
#define _FSUB_MFR_FIMMI				0x0102
#define _FSUB_R_FIMMI				0x0103
#define _FSUB_M_FIMMI				0x0104
#define _FSUB_MR_FIMMI				0x0105

#define _FMUL_FR_FR					0x0106
#define _FMUL_FR_FM					0x0107
#define _FMUL_FM_FR					0x0108
#define _FMUL_FR_MFR				0x0109
#define _FMUL_MFR_FR				0x010A
#define _FMUL_R_FR					0x010B
#define _FMUL_FR_R					0x010C
#define _FMUL_MR_FR					0x010D
#define _FMUL_FR_MR					0x010E
#define _FMUL_M_FR					0x010F
#define _FMUL_FR_M					0x0110
#define _FMUL_R_FM					0x0111
#define _FMUL_FM_R					0x0112
#define _FMUL_MR_FM					0x0113
#define _FMUL_FM_MR					0x0114
#define _FMUL_M_FM					0x0115
#define _FMUL_FM_M					0x0116
#define _FMUL_R_MFR					0x0117
#define _FMUL_MFR_R					0x0118
#define _FMUL_MR_MFR				0x0119
#define _FMUL_MFR_MR				0x011A
#define _FMUL_M_MFR					0x011B
#define _FMUL_MFR_M					0x011C
#define _FMUL_FR_FIMMI				0x011D
#define _FMUL_FM_FIMMI				0x011E
#define _FMUL_MFR_FIMMI				0x011F
#define _FMUL_R_FIMMI				0x0120
#define _FMUL_M_FIMMI				0x0121
#define _FMUL_MR_FIMMI				0x0122

#define _FDIV_FR_FR					0x0123
#define _FDIV_FR_FM					0x0124
#define _FDIV_FM_FR					0x0125
#define _FDIV_FR_MFR				0x0126
#define _FDIV_MFR_FR				0x0127
#define _FDIV_R_FR					0x0128
#define _FDIV_FR_R					0x0129
#define _FDIV_MR_FR					0x012A
#define _FDIV_FR_MR					0x012B
#define _FDIV_M_FR					0x012C
#define _FDIV_FR_M					0x012D
#define _FDIV_R_FM					0x012E
#define _FDIV_FM_R					0x012F
#define _FDIV_MR_FM					0x0130
#define _FDIV_FM_MR					0x0131
#define _FDIV_M_FM					0x0132
#define _FDIV_FM_M					0x0133
#define _FDIV_R_MFR					0x0134
#define _FDIV_MFR_R					0x0135
#define _FDIV_MR_MFR				0x0136
#define _FDIV_MFR_MR				0x0137
#define _FDIV_M_MFR					0x0138
#define _FDIV_MFR_M					0x0139
#define _FDIV_FR_FIMMI				0x013A
#define _FDIV_FM_FIMMI				0x013B
#define _FDIV_MFR_FIMMI				0x013C
#define _FDIV_R_FIMMI				0x013D
#define _FDIV_M_FIMMI				0x013E
#define _FDIV_MR_FIMMI				0x013F

#define _FMOD_FR_FR					0x0140
#define _FMOD_FR_FM					0x0141
#define _FMOD_FM_FR					0x0142
#define _FMOD_FR_MFR				0x0143
#define _FMOD_MFR_FR				0x0144
#define _FMOD_R_FR					0x0145
#define _FMOD_FR_R					0x0146
#define _FMOD_MR_FR					0x0147
#define _FMOD_FR_MR					0x0148
#define _FMOD_M_FR					0x0149
#define _FMOD_FR_M					0x014A
#define _FMOD_R_FM					0x014B
#define _FMOD_FM_R					0x014C
#define _FMOD_MR_FM					0x014D
#define _FMOD_FM_MR					0x014E
#define _FMOD_M_FM					0x014F
#define _FMOD_FM_M					0x0150
#define _FMOD_R_MFR					0x0151
#define _FMOD_MFR_R					0x0152
#define _FMOD_MR_MFR				0x0153
#define _FMOD_MFR_MR				0x0154
#define _FMOD_M_MFR					0x0155
#define _FMOD_MFR_M					0x0156
#define _FMOD_FR_FIMMI				0x0157
#define _FMOD_FM_FIMMI				0x0158
#define _FMOD_MFR_FIMMI				0x0159
#define _FMOD_R_FIMMI				0x015A
#define _FMOD_M_FIMMI				0x015B
#define _FMOD_MR_FIMMI				0x015C

#define _BMOV_R_BM					0x015D
#define _BMOV_BM_R					0x015E
#define _BMOV_R_BR					0x015F
#define _BMOV_BR_R					0x0160
#define _BMOV_BR_BR					0x0161
#define _BMOV_BM_BM					0x0162
#define _BMOV_BR_IMMI8				0x0163
#define _BMOV_BM_IMMI8				0x0164

#define _CMPE_R_R					0x0165
#define _CMPNE_R_R					0x0166
#define _CMPG_R_R					0x0167
#define _CMPL_R_R					0x0168
#define _CMPGE_R_R					0x0169
#define _CMPLE_R_R					0x016A
#define _CMPE_R_FR_FR				0x016B
#define _CMPNE_R_FR_FR				0x016C
#define _CMPG_R_FR_FR				0x016D
#define _CMPL_R_FR_FR				0x016E
#define _CMPGE_R_FR_FR				0x016F
#define _CMPLE_R_FR_FR				0x0170

#define _FOUT_IMMI_FR				0x0174
#define _FIN_IMMI_FR				0x0175
#define _FOUT_IMMI_FIMMI			0x0176

#define _PUSHES_R_R					0x0177
#define _POPS_R_R					0x0178

#define _FPUSHES_FR_FR				0x0179
#define _FPOPS_FR_FR				0x017A
#define _FPUSH_FR					0x017B
#define _FPOP_FR					0x017C

#define _CMPE_R_IMMI				0x017D
#define _CMPNE_R_IMMI				0x017E
#define _CMPG_R_IMMI				0x017F
#define _CMPGE_R_IMMI				0x0180
#define _CMPL_R_IMMI				0x0181
#define _CMPLE_R_IMMI				0x0182

#define _TIME						0x0183
#define _SLEEP						0x0184

#define _RAND						0x0185

#endif
