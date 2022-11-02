#pragma once
#include <chrono>
#include <string>

class GlobalTime
{
  public:
    GlobalTime(float delay, uint64_t note_count, std::wstring midi_name);
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
    uint64_t note_count;
    std::wstring midi_name;

    void syncTime();
    void updateRPC();
};

