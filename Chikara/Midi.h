#pragma once

#include <stdexcept>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <ppl.h>
#include <mutex>
#include <shared_mutex>
using namespace std;

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
  double start;
  double end;
  int key;
  NoteColor color;
};

struct Tempo
{
  uint64_t pos;
  uint32_t tempo;
};

class BufferedReader
{
  public:
    BufferedReader(ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size, std::mutex* _mtx);
    ~BufferedReader();
    void seek(int64_t offset, int origin);
    void read(uint8_t* dst, size_t size);
    uint8_t readByte();
  private:
    void updateBuffer();

    ifstream* file_stream;
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
    MidiTrack(ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size, uint32_t _track_num, uint16_t _ppq, std::mutex* _mtx);
    ~MidiTrack();
    void parseDelta();
    void parseDeltaTime();
    void parseEvent2ElectricBoogaloo(list<Note*>** global_notes);
    void parseEvent1();

    bool ended = false;
    bool delta_parsed = false;
    uint64_t tick_time = 0;
    double time = 0;
    uint32_t notes_parsed = 0;
    list<Note*>** note_stacks = NULL;
    vector<Tempo> tempo_events;
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

    list<Note*>** note_buffer;
    Tempo* tempo_array;
    uint32_t tempo_count;
  private:
    void loadMidi();
    void assertText(const char* text);
    uint8_t readByte();
    uint16_t parseShort();
    uint32_t parseInt();

    vector<MidiChunk> tracks;
    MidiTrack** readers;
    ifstream file_stream;
    size_t file_end;
    uint16_t format;
    uint16_t ppq;
    uint32_t track_count;
    std::mutex mtx;
};