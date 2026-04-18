#include <windows.h>
#include <stdio.h>
#include <iostream>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

using namespace std;
using namespace ImGui;

const char*k = "[+]";
const char*e = "[-]";
const char*i = "[*]";

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool DInjector_active = true;
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

DWORD PID, TID = 0; // Because of warning
LPVOID rBuffer = NULL;
HMODULE hKernel32 = NULL;
HANDLE hprocess, hThread = NULL;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

wchar_t dllPath[MAX_PATH] = L"C:\\Users\\Denmp\\Projects\\DInjector\\Injector.dll";
size_t dllPathSize = sizeof(dllPath);

class backend
{
public:
    int main(int argc, char* argv[])
    {
        if (argc < 2)
        {
            
            printf("%s usage: %s", e, argv[0]);
            return 1;
        }

        PID = atoi(argv[1]);

        printf("%s Usage: %s", e, argv[0]);

        hprocess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);

        if (hprocess == NULL)
        {
            printf("%s Failed to get a handle to the process, error: %ld", e, GetLastError()); 
            return 1;
        }

        printf("%s Got a hadnle to the process (%ld)\n\\---0x%\n", k,PID, hprocess);
        
        rBuffer = VirtualAllocEx(hprocess, NULL, dllPathSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE);
        
        printf("%s Allocated buffer to process memory w/ PAGE_READWRITE permissions\n", k);

        if (rBuffer == NULL)
        {
            printf("%s Couldn't create rBuffer, Error: %ld", e, GetLastError());
            return 1;
        }
        
        WriteProcessMemory(hprocess, rBuffer, dllPath, dllPathSize, NULL);

        printf("%s wrote [%S] to process memory\n", k, dllPath);

        hKernel32 = GetModuleHandleW(L"Kernel32");

        if (hKernel32 == NULL)
        {
            printf("% Failed to get a handle to Kernel32.dll, error: %ld", e, GetLastError());
            CloseHandle(hprocess);

            return 1;
        }

        printf("%s got a handle to Kernel32.dll\n\\---0x%p\n", k, hKernel32);

        LPTHREAD_START_ROUTINE startFinding = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW"); // Give a handle to the module and get back the address
        printf("%s Got the address of LoadLibraryW()\n\\---0x%p\n", k, startFinding);

        hThread = CreateRemoteThread(hprocess, NULL, 0, startFinding, rBuffer, 0, &TID);

        if (hThread == NULL)
        {
            printf("% Failed to get a handle to Thread, error: %ld", e, GetLastError());
            CloseHandle(hprocess);
            return 1;
        }

        printf("%s Got a handle to the newly created thread (%ld)\n\\---0x%p\n", k, TID, hThread);
        printf("%s Waiting for the thread to finish execution\n", i);

        WaitForSingleObject(hThread, INFINITE);

        printf("%s Thread finished executing, Successfully\n", k);

        CloseHandle(hThread);
        CloseHandle(hprocess);

        return EXIT_SUCCESS;
    }

};

class FrontEnd
{
public:

    void Window()
    {
        // Make process DPI aware and obtain main monitor scale
        ImGui_ImplWin32_EnableDpiAwareness();
        float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

        // Create window for the application
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
        ::RegisterClassExW(&wc);
        HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"DInjector", WS_OVERLAPPEDWINDOW, 100, 100, (int)(800 * main_scale), (int)(600 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

        // Initialize Direct3D
        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
            
            abort();
        }

        // Show the window
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        // Setup ImGui context
        IMGUI_CHECKVERSION();
        CreateContext();
        ImGuiIO& io = GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        // Setup ImGui style I can use Light() as well
        StyleColorsDark();

        // Setup scaling
        ImGuiStyle& style = GetStyle();
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;

        // Set up renderers
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        // Main Window loop
        bool done = false;
        while (!done)
        {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            // Handle window being minimized or screen locked
            if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
            {
                ::Sleep(10);
                continue;
            }
            g_SwapChainOccluded = false;

            // Handle window resize
            if (g_ResizeWidth != 430 && g_ResizeHeight != 330)
            {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
                g_ResizeWidth = g_ResizeHeight = 0;
                CreateRenderTarget();
            }

            ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f); // add color

            // Start the frame
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            NewFrame();

            // Create A Window
            {
                Begin("DInjector", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus); // Create a window and name it.
                
                SetWindowPos(ImVec2(0, 0));
                SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
                
                SetCursorPos(ImVec2(400.0f, 400.0f));
                Text("Select Process: "); // Display some text (you can use a format strings too)

                SetCursorPos(ImVec2(400.0f, 250.0f));
                Text("DLL Path: ");

                if (Button("Choose The Program You Want To Inject"))
                {
                    // Do something
                }
                
                End();
            }

            // Rendering
            Render();
            const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            // Present
            HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
            //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
            g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
            
        }
        
        // Cleanup
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }
    
    bool CreateDeviceD3D(HWND hWnd)
    {
        // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
        //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
        HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
        if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
             res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
        if (res != S_OK)
            return false;

        CreateRenderTarget();
        return true;
    }

    void CleanupDeviceD3D()
    {
        CleanupRenderTarget();
        if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
        if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    }

    void CreateRenderTarget()
    {
        ID3D11Texture2D* pBackBuffer;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    void CleanupRenderTarget()
    {
        if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    }

private:
    
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                return true;

            switch (msg)
            {
            case WM_SIZE:
                if (wParam == SIZE_MINIMIZED)
                    return 0;
                g_ResizeWidth = (UINT)LOWORD(lParam);
                g_ResizeHeight = (UINT)HIWORD(lParam);
                return 0;
            case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU)
                    return 0;
                break;
            case WM_DESTROY:
                ::PostQuitMessage(0);
                return 0;
            }
                return ::DefWindowProcW(hWnd, msg, wParam, lParam);
        }

public:

    int run()
    {
        Window();
        CleanupDeviceD3D();
        CreateRenderTarget();
        CleanupRenderTarget();

        return 0;
    }
};

int main()
{
    FrontEnd frontend;
    return frontend.run();
}