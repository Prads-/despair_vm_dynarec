/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "build.h"
#include "declarations.h"

class KeyboardManager {
private:
	bool keyStatus[256];
public:
	KeyboardManager();

	bool getKeyStatus(uint8 keyIndex);
	void setKeyStatus(uint8 keyIndex, bool status);
};

#endif