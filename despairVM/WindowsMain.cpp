/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "build.h"

#ifdef BUILD_FOR_WINDOWS
#include <Windows.h>

#ifdef DEBUG_DESPAIR
//For checking Memory leaks
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <time.h>
#include "uiDirectx.h"
#include "despairVM.h"
#include "despairHeader.h"
using namespace UIDirectX;
using namespace DespairHeader;

const char WND_CN[] = "DespairVMCN";
HWND mainHwnd;
DespairVM despairVM;

#include <fstream>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_KEYUP:
			despairVM.setKeyboardKeyStatus((uint8)wParam, false);
			break;
		case WM_KEYDOWN:
			despairVM.setKeyboardKeyStatus((uint8)wParam, true);
			break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
#ifdef DEBUG_DESPAIR
	//To check memory leaks
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	WNDCLASSEX wCEx;

	srand(time(0));

	wCEx.cbClsExtra = 0;
	wCEx.cbSize = sizeof(WNDCLASSEX);
	wCEx.cbWndExtra = 0;
	wCEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wCEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wCEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wCEx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wCEx.hInstance = hInstance;
	wCEx.lpfnWndProc = WndProc;
	wCEx.lpszClassName = WND_CN;
	wCEx.lpszMenuName = 0;
	wCEx.style = 0;

	if (!RegisterClassEx(&wCEx)) {
		MessageBox(NULL, "Error while registering windows class", "Error", MB_ICONERROR);
		return 0;
	}

	mainHwnd = CreateWindowEx(0, WND_CN, "", 0, 0, 0, 512, 512,
		NULL, NULL, hInstance, NULL);

	if (mainHwnd == NULL) {
		MessageBox(NULL, "Error while creating window", "Error", MB_ICONERROR);
		return 0;
	}

	ShowWindow(mainHwnd, nShowCmd);
	UpdateWindow(mainHwnd);

	std::string binPath = lpCmdLine;
	if (binPath.size() == 0) {
		MessageBox(mainHwnd, "No parameter passed", "Error", MB_ICONERROR);
		return 0;
	} else if (binPath[0] == '\"') {
		binPath = binPath.substr(1, binPath.size() - 2);	//Get rid of quotes
	}
	int retVal = despairVM.startUpDespairVM(binPath);
	if (retVal != DPVM_START_UP_OK) {
		switch (retVal) {
			case DPVM_START_UP_ERROR_BOOT_FAILED:
				MessageBox(mainHwnd, "DespairVM failed to boot up", "Error", MB_ICONERROR);
				break;
			case DPVM_START_UP_ERROR_FILE_CORRUPTED:
				MessageBox(mainHwnd, "Binary file is corrupted", "Error", MB_ICONERROR);
				break;
			case DPVM_START_UP_ERROR_FILE_IO:
				MessageBox(mainHwnd, "Couldn't open binary file", "Error", MB_ICONERROR);
				break;
			case DPVM_START_UP_ERROR_FILE_NOT_RECOGNIZED:
				MessageBox(mainHwnd, "Binary file is not recognized", "Error", MB_ICONERROR);
		}
		return 0;
	}
	
	const ExecutableHeader *header = despairVM.getHeader();

	SetWindowPos(mainHwnd, 0, 0, 0, header->part1.frameBufferWidth, header->part1.frameBufferHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	if (!initializeDirectX(&mainHwnd, header->part1.frameBufferWidth, header->part1.frameBufferHeight)) {
		MessageBox(NULL, "Error initializing DirectX", "Error", MB_ICONERROR);
		return 0;
	}

	MSG msg;
	while (true) {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (despairVM.checkMainThreadStatus())
			renderFrame(despairVM.getGPUFrameBuffer());
		else
			break;
	}

	directXCleanUp();

	return 0;
}

#endif