/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "despairHeader.h"
using namespace std;

void DespairHeader::getMagicNumber(ExecutableHeader *header, FILE **exeFile) {
	fseek(*exeFile, 0, SEEK_SET);
	fread(&header->part1, 1, 4, *exeFile);
}

void DespairHeader::getHeaderInfo(ExecutableHeader *header, FILE **exeFile, std::string exeFolder) {
	fseek(*exeFile, 0, SEEK_SET);
	fread(&header->part1, 1, sizeof(HeaderPart1), *exeFile);
	header->part2.globalPreDataSize = header->part1.headerSize - sizeof(HeaderPart1);
	header->part2.globalPreData = new uint8[header->part2.globalPreDataSize];
	fread(header->part2.globalPreData, 1, header->part2.globalPreDataSize, *exeFile);
	header->exeFolder = exeFolder;
}