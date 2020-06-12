#pragma once

#include <array>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <ppl.h>
#include <mutex>
#include <shared_mutex>
#include "readerwriterqueue.h"
#include "Misc.h"

#define PLAYBACK_TERMINATE_EVENT 0xDEADBEEF

struct MidiChunk
{
  size_t start;
  size_t length;
};

// TODO (Khang): save 3 bytes per note by putting the key in the unused alpha part of color
struct Note
{
  float start;
  float end;
  uint32_t track; // actually (track * 16) + channel
  unsigned char key;
  bool hidden;
};

struct Tempo
{
  uint64_t pos;
  uint32_t tempo;
};

// Note On/Off, After-touch, Control Change, Pitch Wheel, Program Change, Channel after-touch
struct MidiEvent
{
  float time;
  uint32_t msg;
};

enum class NoteEventType : char {
  NoteOff,
  NoteOn,
  TrackEnded, // important note: does not follow the same (track * 16) + channel format as the others
};

// separate from MidiEvent, used to start and end notes on the renderer
// also used to tell the renderer that a track has ended
struct NoteEvent
{
  float time;
  uint32_t track; // actually (track * 16) + channel, max of 268435455 tracks so it'll never overflow
  NoteEventType type;
  // key isn't needed
};

// TODO (Khang): constexpr meta-programming is too hard, using macros for now...
//                         C      C#    D      D#    E      F      F#    G      G#    A      A#    B
#define SHARP_TABLE_OCTAVE false, true, false, true, false, false, true, false, true, false, true, false,
#define SEVEN_SHARP_TABLE_OCTAVES SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE

// 21 full octaves, 5 notes left over
// 257 because 256 key mode needs 257 keys displayed
constexpr std::array<bool, 257> g_sharp_table = {
  SEVEN_SHARP_TABLE_OCTAVES
  SEVEN_SHARP_TABLE_OCTAVES
  SEVEN_SHARP_TABLE_OCTAVES
//C      C#    D      D#    E
  false, true, false, true, false
};

class BufferedReader
{
  public:
    BufferedReader(std::ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size, std::mutex* _mtx);
    ~BufferedReader();
    void seek(int64_t offset, int origin);
    void read(uint8_t* dst, size_t size);
    uint8_t readByte();
    void skipBytes(size_t size);
  private:
    void updateBuffer();

    std::ifstream* file_stream;
    size_t start;
    size_t length;
    size_t pos;
    uint8_t* buffer;
    uint32_t buffer_size;
    uint32_t buffer_pos = 0;
    size_t buffer_start;
    std::mutex* mtx;
};

class MidiTrack
{
  public:
    MidiTrack(std::ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size, uint32_t _track_num, uint16_t _ppq, std::mutex* _mtx);
    ~MidiTrack();
    void parseDelta();
    void parseDeltaTime();
    void parseEvent(moodycamel::ReaderWriterQueue<NoteEvent>** global_note_events, moodycamel::ReaderWriterQueue<MidiEvent>* global_misc);

    bool ended = false;
    bool delta_parsed = false;
    bool notes_ended = false;
    uint64_t tick_time = 0;
    double time = 0;
    uint32_t notes_parsed = 0;
    std::vector<Tempo> tempo_events;
    Tempo* global_tempo_events = 0;
    double tempo_multiplier = 0;
    uint32_t global_tempo_event_count = 0;
    uint32_t global_tempo_event_index = 0;
    uint16_t ppq = 0;
    uint32_t track_num = 0;
  private:
    uint8_t prev_command = 0;
    BufferedReader* reader = NULL;

    double multiplierFromTempo(uint32_t tempo, uint16_t ppq);
};

class Midi
{
  public:
    Midi(wchar_t* file_name);
    ~Midi();
    void SpawnLoaderThread();
    void SpawnPlaybackThread(std::chrono::steady_clock::time_point start_time);

    moodycamel::ReaderWriterQueue<NoteEvent>** note_event_buffer;
    moodycamel::ReaderWriterQueue<MidiEvent> misc_events;
    Tempo* tempo_array;
    uint32_t tempo_count;
    std::atomic<float> renderer_time;
    uint32_t track_count;
  private:
    void loadMidi();
    void assertText(const char* text);
    void LoaderThread();
    void PlaybackThread();
    uint8_t readByte();
    uint16_t parseShort();
    uint32_t parseInt();

    std::vector<MidiChunk> tracks;
    MidiTrack** readers;
    std::ifstream file_stream;
    size_t file_end;
    uint16_t format;
    uint16_t ppq;
    std::mutex mtx;
    std::thread loader_thread;
    std::thread playback_thread;
    std::chrono::steady_clock::time_point start_time;
};