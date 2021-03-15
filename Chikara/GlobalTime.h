#pragma once
#include <chrono>

class GlobalTime
{
  public:
    GlobalTime(float delay);
    double getTime();
    void pause();
    void resume();
    void changeSpeed(float speed);
    void skipForward(float seconds);
    bool isPaused();
  private:
    bool paused = false;
    std::chrono::steady_clock::time_point real_time;
    double midi_time = 0;
    float speed = 1;

    void syncTime();
};

