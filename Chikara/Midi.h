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

struct MidiChunk
{
  size_t start;
  size_t length;
};

struct NoteColor 
{
  float r;
  float g;
  float b;
};

struct Note
{
  float start;
  float end;
  int key;
  NoteColor color;
  char channel;
  char velocity;
  bool noteon_played = false;
};

struct Tempo
{
  uint64_t pos;
  uint32_t tempo;
};

// TODO (Khang): constexpr meta-programming is too hard, using macros for now...
//                         C      C#    D      D#    E      F      F#    G      G#    A      A#    B
#define SHARP_TABLE_OCTAVE false, true, false, true, false, false, true, false, true, false, true, false,
#define SEVEN_SHARP_TABLE_OCTAVES SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE SHARP_TABLE_OCTAVE

// 21 full octaves, 4 notes left over
constexpr std::array<bool, 256> g_sharp_table = {
  SEVEN_SHARP_TABLE_OCTAVES
  SEVEN_SHARP_TABLE_OCTAVES
  SEVEN_SHARP_TABLE_OCTAVES
//C      C#    D      D#
  false, true, false, true,
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
    void parseEvent(std::list<Note*>** global_notes);

    bool ended = false;
    bool delta_parsed = false;
    uint64_t tick_time = 0;
    double time = 0;
    uint32_t notes_parsed = 0;
    std::list<Note*>** note_stacks = NULL;
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

    void initNoteStacks();
    void deleteNoteStacks();
    double multiplierFromTempo(uint32_t tempo, uint16_t ppq);
};

class Midi
{
  public:
    Midi(const char* file_name);
    ~Midi();

    std::list<Note*>** note_buffer;
    Tempo* tempo_array;
    uint32_t tempo_count;
  private:
    void loadMidi();
    void assertText(const char* text);
    uint8_t readByte();
    uint16_t parseShort();
    uint32_t parseInt();

    std::vector<MidiChunk> tracks;
    MidiTrack** readers;
    std::ifstream file_stream;
    size_t file_end;
    uint16_t format;
    uint16_t ppq;
    uint32_t track_count;
    std::mutex mtx;
};