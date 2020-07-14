#pragma once

#include <iostream>
#include <filesystem>
#include <discord_rpc.h>

class Utils
{
  public:
    std::string GetFileName(std::filesystem::path file_path);
    void InitDiscord();
    static void UpdatePresence(const char* state, const char* details, std::string file_name, uint64_t start_time, uint64_t end_time);
    void destroyDiscord();
};

