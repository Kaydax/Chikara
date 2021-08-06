#pragma once

#include <iostream>
#include <filesystem>
#include <codecvt>
#ifdef RELEASE
#include <discord_rpc.h>
#endif

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
#ifdef RELEASE
  static void InitDiscord();
  static void UpdatePresence(const char* state, const char* details, std::string file_name);
  static void DestroyDiscord();
#endif
  static void KillAllVoices();
  rgb HSVtoRGB(float H, float S, float V);
  static std::string wstringToUtf8(std::wstring str);
  static std::string format_seconds(double secs);
};

