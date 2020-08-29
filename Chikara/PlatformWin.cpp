#include <string>
#include <Windows.h>
#include "Platform.h"

namespace Platform {
  std::wstring OpenMIDIFileDialog() {
    OPENFILENAMEW ofn = { 0 };
    wchar_t file_name[1024] = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.lpstrFilter = L"MIDI Files\0*.mid\0";
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = sizeof(file_name) / sizeof(wchar_t);
    ofn.lpstrTitle = L"Open a MIDI";
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    GetOpenFileNameW(&ofn);
    return std::wstring(file_name);
  }
}