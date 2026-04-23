## DInjector

DInjector is a simple DLL injector for Windows (x86/x64), built with C++17, WinAPI, and ImGui (DX11).  
It uses the standard `LoadLibraryW` method with a clean UI instead of console input.

[<img src="https://img.shields.io/badge/Download-v1.5-D5006D?style=for-the-badge&logo=github">](https://github.com/D1on2k/DInjector/releases/tag/Injector1.5) <img src="https://img.shields.io/badge/platform-Windows-D5006D?style=for-the-badge"> <img src="https://img.shields.io/github/license/D1on2k/DInjector?style=for-the-badge&color=D5006D"> <img src="https://img.shields.io/badge/C%2B%2B-17-D5006D?style=for-the-badge">

---

## Features

- ImGui + DirectX 11 interface (smooth, with no freezing issues)
- Real-time process lookup (`CreateToolhelp32Snapshot`)
- DLL selection using Windows file dialog
- Injection via `CreateRemoteThread` + `LoadLibraryW`
- Unicode path support
- Optional auto-close after injection
- Static build (portable)

---

## Usage

Run: `DInjector.exe`

Steps:
- Enter the target process name (e.g. `notepad.exe`)
- Click **Browse** and select your DLL
- Press **Inject**

---

## Issues / Bugs

If something isn’t working, check:

- DLL path is valid  
- Architecture matches (x86 ↔ x64)  
- Run as Administrator  
- Antivirus not blocking it  
- Target process allows access  

If you find a real bug, open an issue with:
- what you tried  
- target process  
- any error output  

---

## Contributing

Pull requests are welcome.

Ideas for improvements:
- better error handling / UI feedback  
- manual mapping instead of `LoadLibrary`  
- process list dropdown instead of typing  
- proper logging instead of `printf`  

Keep it clean and readable.

---

## Disclaimer

> This tool is provided **for educational purposes only**.  
> By using this code, **you take full responsibility** for how it is used.  
> The developer is **not responsible** for any misuse, legal consequences or damages.
