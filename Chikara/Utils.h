#pragma once

#include <iostream>
#include <filesystem>
#include <codecvt>
#include <discord_rpc.h>

class Utils
{
  public:
    static std::wstring GetFileName(std::filesystem::path file_path);
    static void InitDiscord();
    static void UpdatePresence(const char* state, const char* details, std::string file_name, uint64_t start_time, uint64_t end_time);
    static void DestroyDiscord();
    static std::string wstringToUtf8(std::wstring str);
};

