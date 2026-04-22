// Injector.h

#pragma once

#include <windows.h>
#include <d3d11.h>
#include "imgui.h"

extern wchar_t dllPath[MAX_PATH];
extern DWORD PID, TID;
extern LPVOID rBuffer;
extern HMODULE hKernel32;
extern HANDLE hprocess, hThread;
extern bool Test;
extern char IAMTIRED[256];

extern const char *k, *e, *i;

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern bool                     g_SwapChainOccluded;
extern UINT                     g_ResizeWidth, g_ResizeHeight;
extern ID3D11RenderTargetView* g_mainRenderTargetView;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);