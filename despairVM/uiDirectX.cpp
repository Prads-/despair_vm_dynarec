/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "uiDirectx.h"

struct TDVertices {
	float x, y, z;
	float u, v;
};

static IDirect3DDevice9 *d3dDevice = 0;
static IDirect3DTexture9 *texScreen = 0;
static IDirect3DVertexBuffer9 *vertexBuffer = 0;
static int fWidth, fHeight;

void createScreenPlane();

bool UIDirectX::initializeDirectX(HWND *hwnd, int width, int height) {
	IDirect3D9 *d3d;

	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (d3d == NULL) return false;

	D3DCAPS9 d3dCaps;

	if (d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps) != D3D_OK) {
		d3d->Release();
		return false;
	}

	int vP;
	if (d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		vP = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vP = D3DCREATE_HARDWARE_VERTEXPROCESSING;

	D3DPRESENT_PARAMETERS d3dInfo = {0};

	d3dInfo.BackBufferCount = 1;
	d3dInfo.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dInfo.BackBufferHeight = height;
	d3dInfo.BackBufferWidth = width;
	d3dInfo.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dInfo.hDeviceWindow = *hwnd;
	d3dInfo.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dInfo.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dInfo.Windowed = TRUE;

	if (d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, *hwnd, vP, &d3dInfo, &d3dDevice) != 0) {
		d3d->Release();
		return false;
	}

	d3d->Release();

	D3DXVECTOR3 cPos(0.0f, 0.0f, -1.1f);
	D3DXVECTOR3 cAt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 cUp(0.0f, 1.0f, 0.0f);

	D3DXMATRIX cOut;
	D3DXMatrixLookAtLH(&cOut, &cPos, &cAt, &cUp);
	d3dDevice->SetTransform(D3DTS_VIEW, &cOut);

	//Set Projection
	D3DXMATRIX pProj;
	D3DXMatrixPerspectiveFovLH(&pProj, D3DX_PI / 2, 1.0f, 0.1f, 10.0f);
	d3dDevice->SetTransform(D3DTS_PROJECTION, &pProj);
	
	//Set some states
	d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	d3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	d3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	
	if (D3DXCreateTexture(d3dDevice, width, height, 0, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texScreen) != D3D_OK)
		return false;

	if (d3dDevice->CreateVertexBuffer(6 * sizeof(TDVertices), 0, D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_MANAGED, &vertexBuffer, NULL) != D3D_OK)
		return false;

	createScreenPlane();

	d3dDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	d3dDevice->SetTexture(0, texScreen);
	d3dDevice->SetStreamSource(0, vertexBuffer, 0, sizeof(TDVertices));

	fWidth = width;
	fHeight = height;

	return true;
}

void UIDirectX::directXCleanUp() {
	if (d3dDevice) d3dDevice->Release();
	d3dDevice = 0;
	if (texScreen) texScreen->Release();
	texScreen = 0;
	if (vertexBuffer) vertexBuffer->Release();
	vertexBuffer = 0;
}

void UIDirectX::renderFrame(const uint32 *frameBuffer) {
	d3dDevice->Clear(0, 0, D3DCLEAR_STENCIL, 0xFFFFFFFF, 1.0f, 0);
	d3dDevice->BeginScene();

	D3DLOCKED_RECT lockedRect;
	texScreen->LockRect(0, &lockedRect, 0, D3DLOCK_DISCARD);
	for (int y = 0; y < fHeight; ++y) {
		for (int x = 0; x < fWidth; ++x) {
			*(unsigned int*)((char*)lockedRect.pBits + x * (lockedRect.Pitch / fWidth) + y * lockedRect.Pitch) = frameBuffer[x + y * fHeight];
		}
	}
	texScreen->UnlockRect(0);

	d3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

	d3dDevice->EndScene();
	d3dDevice->Present(0, 0, 0, 0);
}

void createScreenPlane() {
	TDVertices *pVData;
	vertexBuffer->Lock(0, 0, (void**)&pVData, 0);

	pVData[0].x = -1.0f;		pVData[0].y = -1.0f;		pVData[0].z = 0.0f; 
	pVData[0].u = 0.0f;			pVData[0].v = 1.0f;
	
	pVData[1].x = -1.0f;		pVData[1].y = 1.0f;			pVData[1].z = 0.0f; 
	pVData[1].u = 0.0f;			pVData[1].v = 0.0f;
	
	pVData[2].x = 1.0f;			pVData[2].y = 1.0f;			pVData[2].z = 0.0f; 
	pVData[2].u = 1.0f;			pVData[2].v = 0.0f;
	
	pVData[3].x = 1.0f;			pVData[3].y = 1.0f;			pVData[3].z = 0.0f; 
	pVData[3].u = 1.0f;			pVData[3].v = 0.0f;
	
	pVData[4].x = 1.0f;			pVData[4].y = -1.0f;		pVData[4].z = 0.0f; 
	pVData[4].u = 1.0f;			pVData[4].v = 1.0f;
	
	pVData[5].x = -1.0f;		pVData[5].y = -1.0f;		pVData[5].z = 0.0f; 
	pVData[5].u = 0.0f;			pVData[5].v = 1.0f;

	vertexBuffer->Unlock();
}