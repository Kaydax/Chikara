#pragma once

#include <iostream>
#include <filesystem>
#include <codecvt>
#include <discord_rpc.h>

struct rgb
{
  uint32_t r;
  uint32_t g;
  uint32_t b;
};

class Utils
{
public:
  static std::wstring GetFileName(std::filesystem::path file_path);
  static void InitDiscord();
  static void UpdatePresence(const char* state, const char* details, std::string file_name, uint64_t start_time, uint64_t end_time);
  static void DestroyDiscord();
  rgb HSVtoRGB(float H, float S, float V);
  static std::string wstringToUtf8(std::wstring str);
};

