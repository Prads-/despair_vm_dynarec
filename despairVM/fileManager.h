/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <fstream>
#include "build.h"
#include "declarations.h"
#include "portAddress.h"

#define FILE_MANAGER_CREATE_OBJ				0
#define FILE_MANAGER_DESTROY_OBJ			1
#define FILE_MANAGER_GET_SIZE				2
#define FILE_MANAGER_SET_OFFSET				3
#define FILE_MANAGER_OPEN_TEXT				4
#define FILE_MANAGER_OPEN_BINARY			5
#define FILE_MANAGER_CLOSE					6
#define FILE_MANAGER_READ_BINARY			7
#define FILE_MANAGER_READ_TEXT				8
#define FILE_MANAGER_WRITE_BINARY			9
#define FILE_MANAGER_WRITE_TEXT				0xA

namespace FileManager {
	void decodeFileCommands(uint8 cmd, uint8 *ports, const std::string *exeFolderPath);
}

#endif