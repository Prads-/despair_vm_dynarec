/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef HEADER_H
#define HEADER_H

#include <string>
#include <cstdio>
#include "build.h"
#include "declarations.h"

namespace DespairHeader {
	#pragma pack(push)
	#pragma pack(1)
	
	struct HeaderPart1 {
		uint32 magicNumber, version, headerSize, codeSize, codeOffset, dataSize, stackSize, globalDataSize, frameBufferWidth, frameBufferHeight;
		uint32 signature[8];
	};

	struct HeaderPart2 {
		uint8 *globalPreData;
		uint32 globalPreDataSize;

		HeaderPart2() {
			globalPreData = 0;
		}

		~HeaderPart2() {
			delete [] globalPreData;
			globalPreData = 0;
		}
	};
	struct ExecutableHeader {
		HeaderPart1 part1;
		HeaderPart2 part2;
		std::string exeFolder;
	};
	
	#pragma pack(pop)

	void getMagicNumber(ExecutableHeader *header, FILE **exeFile);
	void getHeaderInfo(ExecutableHeader *header, FILE **exeFile, std::string exeFolder);
}

#endif