# Injector 

A process Injector built with ImGui(dx3d11) Win32 API and C++17.

# Features
- Dx3d11 Backend: Uses hardware acceleration to prevent UI freezing during injection.
- Instant PID Lookup: Trys to detect process while you type.
- Native File Explorer: Uses Windows Explorer to make DLL selection easier.
- Auto Close Checkbox: It has a checkbox where it auto closes after injection.
- Static Build: It's optimized for portability with minimal external dependencies.

[Screenshot/Preview]
<img width="780" height="587" alt="image" src="https://github.com/user-attachments/assets/b3400318-6ff4-4f84-b86a-0253b476c7eb" />

# How To Build 
Just Paste The ImGui Foldier In Your Foldier And Compile With: 
```bash
g++ -std=c++17 -O2 -DUNICODE -D_UNICODE -I. -IImGui -IImGui/backends Injector.cpp ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp ImGui/backends/imgui_impl_win32.cpp ImGui/backends/imgui_impl_dx11.cpp -ld3d11 -ld3dcompiler -lgdi32 -ldwmapi -luser32 -static -lcomdlg32 -o DInjector.exe
```

# How to Use
- Process: Enter the name of the target executable (e.g., notepad.exe).
- DLL: Click Browse to select your .dll file.
- Inject: Click Inject to finalize the process.

# Disclaimer
> This tool is provided **for educational purposes only**.
> By using this code, **you take full responsibility for how it is used**.
> **The developer is not responsible for any misuse**, legal consequences or damages.
