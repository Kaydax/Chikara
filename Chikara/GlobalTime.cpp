#include "GlobalTime.h"

#include <fmt/locale.h>
#include <fmt/format.h>
#include "Utils.h"
#include "Config.h"

GlobalTime::GlobalTime(float delay, uint64_t note_count, std::wstring midi_name)
{
  real_time = std::chrono::high_resolution_clock::now();
  midi_time = -delay;
  this->note_count = note_count;
  this->midi_name = midi_name;
}

double GlobalTime::getTime()
{
  if(paused) return midi_time;
  return midi_time + std::chrono::duration<double, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - real_time).count() * speed;
}

void GlobalTime::pause()
{
  if(paused) return;
  syncTime();
  paused = true;
  if(Config::GetConfig().discord_rpc) updateRPC();
}

bool GlobalTime::isPaused()
{
  return paused;
}

void GlobalTime::resume()
{
  syncTime();
  paused = false;
  if(Config::GetConfig().discord_rpc) updateRPC();
}

void GlobalTime::changeSpeed(float speed)
{
  syncTime();
  this->speed = speed;
}

void GlobalTime::skipForward(float seconds)
{
  midi_time += seconds;
  syncTime();
}

void GlobalTime::syncTime()
{
  midi_time = getTime();
  real_time = std::chrono::high_resolution_clock::now();
}

void GlobalTime::updateRPC()
{
  auto rpc_text = fmt::format(std::locale(""), "Note Count: {:n}", note_count);
  if(paused)
  {
    Utils::UpdatePresence(rpc_text.c_str(), "Paused: ", Utils::wstringToUtf8(Utils::GetFileName(midi_name)));
  } else {
    Utils::UpdatePresence(rpc_text.c_str(), "Playing: ", Utils::wstringToUtf8(Utils::GetFileName(midi_name)));
  }
}