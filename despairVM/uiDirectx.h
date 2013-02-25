/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "build.h"
#ifdef BUILD_FOR_WINDOWS

#ifndef UI_DIRECTX_H
#define UI_DIRECTX_H

#include "build.h"
#include "declarations.h"
#include <d3d9.h>
#include <d3dx9.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

namespace UIDirectX {
	bool initializeDirectX(HWND *hwnd, int width, int height);
	void directXCleanUp();
	void renderFrame(const uint32 *frameBuffer);
}

#endif
#endif