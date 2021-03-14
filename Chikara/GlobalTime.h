#pragma once
#include <chrono>

class GlobalTime
{
  public:
    bool paused = false;
    
    GlobalTime(float delay);
    double getTime();
    void pause();
    void resume();
    void changeSpeed(float speed);
    void skipForward(float seconds);
  private:
    std::chrono::steady_clock::time_point real_time;
    double midi_time = 0;
    float speed = 1;
    float time_skip = 0;

    void syncTime();
};

