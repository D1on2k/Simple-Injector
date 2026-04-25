// Injector.cpp

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <vector>
#include <algorithm>
#include <string>
#include <shellapi.h>
#include "Injector.h"
#include <winternl.h>

using namespace std;
using namespace ImGui;

wchar_t dllPath[MAX_PATH] = L"Please Select A DLL File";
DWORD PID = 0, TID = 0;
LPVOID rBuffer = NULL;
HMODULE hKernel32 = NULL;
HANDLE hprocess = NULL, hThread = NULL;
bool Test = false;
char IAMTIRED[256] = "";

const char* k = "[+]";
const char* e = "[-]";
const char* i = "[*]";

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
bool g_SwapChainOccluded = false;
UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

class Injector
{
public:

    void window()
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
        ImGuiIO& io = GetIO();
        io.Fonts->AddFontDefault();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

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
            
            // Change Background Color
            ImVec4 clear_color = ImVec4(0.239f, 0.239f, 0.239f, 1.000f); // Pro Tip use: https://htmlcolorcodes.com/color-picker/ 
                                                                         // to get rgb values and devide it with 255 to get float values
                                                                         // which is the max rgb value 61/255 is 0.239... i used darkish 
                                                                         //grey
            // Start the frame
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            NewFrame();

            // Create A Window
            {
                Begin("DInjector", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus); // Create a window and name it.
                
                SetWindowPos(ImVec2(0, 0));
                SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
                
                // Adding Text
                SetCursorPos(ImVec2(50.0f, 100.0f));
                PushFont(NULL, 16.0f);
                Text("Select Process: "); // Display some text (you can use a format strings too)
                PopFont();

                SetCursorPos(ImVec2(200.0f, 100.0f));
                PushItemWidth(200);
                
                if (InputText("(Select Ur Runing Process)", IAMTIRED, sizeof(IAMTIRED)))
                {
                    wchar_t tempWide[MAX_PATH];
                    mbstowcs(tempWide, IAMTIRED, MAX_PATH);
                    
                    PID = GetProcIdByName(tempWide);
                }
                
                SetCursorPos(ImVec2(50.0f, 145.5f));

                ImVec2 pos = GetCursorScreenPos();
                
                Separator();
                
                SetCursorPos(ImVec2(50.0f, 175.0f));
                
                BeginGroup();
                
                    PushFont(NULL, 16.0f);
                    Text("DLL Path: ");
                    PopFont();
                    SameLine();
                    
                    float idk = GetContentRegionAvail().x - 75.0f;
                    
                    PushTextWrapPos(GetCursorPosX() + idk);
                        TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), " %ls", dllPath);
                    
                    SameLine();
                    
                    if (Button("Browse"))
                    {
                        OPENFILENAMEW EFN;
                        wchar_t szFile[MAX_PATH] = { 0 };
                        ZeroMemory(&EFN, sizeof(EFN));

                        EFN.lStructSize = sizeof(EFN);
                        EFN.lpstrFilter = L"DLL Files\0*.dll\0All\0*.*\0";
                        EFN.lpstrFile = szFile;
                        EFN.nMaxFile = MAX_PATH;
                        EFN.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                        if (GetOpenFileNameW(&EFN)) 
                        {
                            wcscpy_s(dllPath, szFile);
                        }   
                    }
                
                EndGroup();
                
                SetCursorPos(ImVec2(130.0f, 165.0f));
                

                SetCursorPos(ImVec2(50.0f, 200.0f));

                ImVec2 pos1 = GetCursorScreenPos();
                
                Separator();

                SetCursorPos(ImVec2(50.0f, 250.0f));
                PushFont(NULL, 16.0f);
                Checkbox(" Close After Injection", &Test);
                PopFont();

                // Adding the buttons
                SetCursorPos(ImVec2(150.0f, 400.0f)); // Set Possision
                if (Button("Inject", ImVec2(200.0f, 50.0f)))
                {
                    if (PID == 0)
                    {
                        printf("%s Please enter a process!\n", e);
                    }
                    else
                    {
                        // Minimal rights - less handle noise
                        hprocess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ |
                                            PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME, FALSE, PID);
                        if (hprocess == NULL)
                        {
                            printf("%s Failed to open process! %ld\n", e, GetLastError());
                        }
                        else
                        {
                            // A small randomized delay
                            Sleep(30 + (rand() % 120));
                            size_t pathBytes = (wcslen(dllPath) + 1) * sizeof(wchar_t);
                            rBuffer = VirtualAllocEx(hprocess, NULL, pathBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                            if (rBuffer != NULL)
                            {
                                // Simple runtime obfuscation on path
                                wchar_t* obfPath = new wchar_t[wcslen(dllPath) + 1];
                                wcscpy_s(obfPath, wcslen(dllPath) + 1, dllPath);
                                for (size_t i = 0; i < wcslen(obfPath); ++i)
                                    obfPath[i] ^= 0xDEAD;  // If you want to change Sigs u can just change this or the delay
                                WriteProcessMemory(hprocess, rBuffer, obfPath, pathBytes, NULL);
                                delete[] obfPath;
                                hKernel32 = GetModuleHandleW(L"Kernel32.dll");
                                // Fixed cast for g++
                                typedef HMODULE(WINAPI* LoadLibraryW_t)(LPCWSTR lpLibFileName);
                                LoadLibraryW_t pLoadLibraryW = (LoadLibraryW_t)GetProcAddress(hKernel32, "LoadLibraryW");
                                // Get ntdll APC calls
                                HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
                                typedef NTSTATUS(NTAPI* NtQueueApcThreadEx_t)(HANDLE ThreadHandle, HANDLE UserApcReserveHandle,
                                                                            PVOID ApcRoutine, PVOID ApcArgument1, PVOID ApcArgument2, PVOID ApcArgument3);
                                NtQueueApcThreadEx_t pNtQueueApcThreadEx = (NtQueueApcThreadEx_t)GetProcAddress(hNtdll, "NtQueueApcThreadEx");
                                // Find a thread instade of making a new one better for code debugging
                                HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
                                THREADENTRY32 te32 = { sizeof(te32) };
                                DWORD targetTID = 0;
                                int skipped = 0;
                                if (Thread32First(hSnap, &te32))
                                {
                                    do
                                    {
                                        if (te32.th32OwnerProcessID == PID)
                                        {
                                            skipped++;
                                            if (skipped >= 4)  // Skip early threads take later one
                                            {
                                                targetTID = te32.th32ThreadID;
                                                break;
                                            }
                                        }
                                    } while (Thread32Next(hSnap, &te32));
                                }
                                CloseHandle(hSnap);
                                if (targetTID != 0)
                                {
                                    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, targetTID);
                                    if (hThread != NULL)
                                    {
                                        // Queue APC(actually i dont know much abt it i just read somewhere that its good for stealth)
                                        NTSTATUS status = pNtQueueApcThreadEx(hThread, NULL, (PVOID)pLoadLibraryW, rBuffer, NULL, NULL);
                                        if (status >= 0)
                                        {
                                            printf("%s APC is queued on an existing thread please wait for it to load\n", k);
                                            Sleep(350); // Give a bit of time to the dll to initialize
                                            printf("%s Injected sucessfully\n", k);
                                        }
                                        else
                                        {
                                            printf("%s APC queue failed try again %ld\n", e, status);
                                        }
                                        CloseHandle(hThread);
                                    }
                                }
                                else
                                {
                                    printf("%s No thread found for APC\n", e);
                                }
                            }
                            // Quick cleanup
                            if (rBuffer) VirtualFreeEx(hprocess, rBuffer, 0, MEM_RELEASE);
                            CloseHandle(hprocess);
                        }
                    }
                }

                SetCursorPos(ImVec2(450.0f, 400.0f));
                if (Button("Quit", ImVec2(200.0f, 50.0f))) // Name and Resize The Button
                {
                    abort();
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

    DWORD GetProcIdByName(const TCHAR* procName) 
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) 
        {
            return 10;
        }
            
        PROCESSENTRY32W pe32;

        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32First(hSnap, &pe32))
        {
            do 
            {
                if (_tcscmp(pe32.szExeFile, procName) == 0)
                {
                    CloseHandle(hSnap);
                    return pe32.th32ProcessID;
                }
            } 
            while (Process32Next(hSnap, &pe32));
        }
        CloseHandle(hSnap);
        return 0;
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
    
    vector<string> processNames;
    int selectedIndex = -1;

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
        window();
        return 0;
    }
};

int main()
{
    Injector Injector;
    return Injector.run();
}