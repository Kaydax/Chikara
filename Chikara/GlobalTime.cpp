#include "GlobalTime.h"

GlobalTime::GlobalTime(float delay)
{
  real_time = std::chrono::high_resolution_clock::now();
  midi_time = -delay;
}

double GlobalTime::getTime()
{
  if(paused) return midi_time + time_skip;
  return midi_time + std::chrono::duration<double, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - real_time).count() * speed + time_skip;
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

void GlobalTime::skipForward(float seconds)
{
  time_skip = seconds; //Set this to the seconds to skip ahead
  syncTime(); //Sync the time so the skip works
  time_skip = 0; //Reset to 0 since the skip is done, also allows us to skip when paused without issues
}

void GlobalTime::syncTime()
{
  midi_time = getTime();
  real_time = std::chrono::high_resolution_clock::now();
}