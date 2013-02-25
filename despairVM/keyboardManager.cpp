/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include <cstring>
#include "keyboardManager.h"

KeyboardManager::KeyboardManager() {
	for (int i = 0; i < 256; ++i) {
		keyStatus[i] = false;
	}
}

bool KeyboardManager::getKeyStatus(uint8 keyIndex) {
	return keyStatus[keyIndex];
}

void KeyboardManager::setKeyStatus(uint8 keyIndex, bool status) {
	keyStatus[keyIndex] = status;
}