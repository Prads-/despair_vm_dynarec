/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#ifndef SHA256_H
#define SHA256_H

#include <cstring>
#include "build.h"
#include "declarations.h"

namespace SHA256 {
	struct SHA_256_MessageDigest {
		uint32 h[8];
	};

	SHA_256_MessageDigest sha256(const uint8 *message, uint32 mSize);
}

#endif