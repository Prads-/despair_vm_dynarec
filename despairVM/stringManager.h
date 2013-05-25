/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef STRING_MANAGER_H
#define STRING_MANAGER_H

#include <string>
#include <cmath>
#include "build.h"
#include "declarations.h"
#include "portAddress.h"

#define STRING_MANAGER_CREATE_OBJ				0
#define STRING_MANAGER_DESTROY_OBJ				1
#define STRING_MANAGER_GET_SIZE					2
#define STRING_MANAGER_GET_CHAR					3
#define STRING_MANAGER_SET_CHAR					4
#define STRING_MANAGER_APPEND_CHAR_ARRAY		5
#define STRING_MANAGER_APPEND_STRING			6
#define STRING_MANAGER_APPEND_INTEGER			7
#define STRING_MANAGER_APPEND_FLOAT				8
#define STRING_MANAGER_CLEAR_STRING				9
#define STRING_MANAGER_GET_CHAR_ARRAY			10
#define STRING_MANAGER_COMPARE					11
#define STRING_MANAGER_COPY_STR					12
#define STRING_MANAGER_SUBSTRING				13

namespace StringManager {
	void decodeStringCommands(uint8 cmd, uint8 *ports);
}

#endif