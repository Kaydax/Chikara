#include <Windows.h>
#include "KDMAPI.h"

namespace KDMAPI {
  HMODULE dll_handle = nullptr;
  uint32_t(__cdecl* SendDirectData)(unsigned long msg);
  int(__cdecl* TerminateKDMAPIStream)();

  void Init() {
    // This block of code is from WinMMWRP
    wchar_t omnimidi_dir[MAX_PATH];
    memset(omnimidi_dir, 0, sizeof(omnimidi_dir));
    GetSystemDirectoryW(omnimidi_dir, MAX_PATH);
    wcscat_s(omnimidi_dir, L"\\OmniMIDI\\OmniMIDI.dll");
    dll_handle = LoadLibraryW(L"OmniMIDI.dll");
    if (!dll_handle) {
      dll_handle = LoadLibraryW(omnimidi_dir);
      if(!dll_handle)
      {
        MessageBoxA(NULL, "OmniMIDI seems to not be installed.", "Fatal Error", MB_ICONERROR);
        throw "OmniMIDI not found!";
      }   
    }

    auto InitializeKDMAPIStream = (BOOL(WINAPI *)())GetProcAddress(dll_handle, "InitializeKDMAPIStream");
    if(!InitializeKDMAPIStream())
    {
      MessageBoxA(NULL, "KDMAPI appeared to fail initialization.", "KDMAPI Error", MB_ICONERROR);
      throw "KDMAPI init failed!";
    }

    auto IsKDMAPIAvailable = (BOOL(WINAPI*)())GetProcAddress(dll_handle, "IsKDMAPIAvailable");
    if(!IsKDMAPIAvailable())
    {
      MessageBoxA(NULL, "OmniMIDI was found, but KDMAPI isn't enabled.", "KDMAPI Error", MB_ICONERROR);
      throw "OmniMIDI was found, but KDMAPI isn't enabled.";
    }

    SendDirectData = (MMRESULT(*)(DWORD))GetProcAddress(dll_handle, "SendDirectData");
    TerminateKDMAPIStream = (BOOL(*)())GetProcAddress(dll_handle, "TerminateKDMAPIStream");
  }

  void Destroy() {
    TerminateKDMAPIStream();
  }
}