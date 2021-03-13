#include "GlobalTime.h"

GlobalTime::GlobalTime(float delay)
{
  real_time = std::chrono::high_resolution_clock::now();
  midi_time = -delay;
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
}

void GlobalTime::resume()
{
  syncTime();
  paused = false;
}

void GlobalTime::changeSpeed(float speed)
{
  syncTime();
  this->speed = speed;
}

void GlobalTime::syncTime()
{
  midi_time = getTime();
  real_time = std::chrono::high_resolution_clock::now();
}