#pragma once

#include <stdexcept>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
using namespace std;

struct MidiChunk
{
  size_t start;
  size_t length;
};

struct NoteColor 
{
  //idk do some shit here
};

struct Note
{
  double start;
  double end;
  NoteColor* color;
};

struct Tempo
{
  uint64_t pos;
  uint32_t tempo;
};

class BufferedReader
{
  public:
    BufferedReader(ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size);
    ~BufferedReader();
    uint8_t readByte();
    uint8_t readByteFast();

    int push_back = -1;
  private:
    void updateBuffer();

    ifstream* file_stream;
    size_t start;
    size_t length;
    size_t pos;
    uint8_t* buffer;
    uint32_t buffer_size;
    uint32_t buffer_pos = 0;
};

class MidiTrack
{
  public:
    MidiTrack(ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size, uint16_t _ppq);
    ~MidiTrack();
    void parseDelta();
    void parseDeltaTime();
    void parseEvent2ElectricBoogaloo();
    void parseEvent1();

    bool ended = false;
    bool delta_parsed = false;
    uint64_t tick_time = 0;
    double time = 0;
    uint32_t notes_parsed = 0;
    list<Note>* note_stacks;
    vector<Tempo> tempo_events;
    Tempo* global_tempo_events = 0;
    double tempo_multiplier = 0;
    uint32_t global_tempo_event_count = 0;
    uint32_t global_tempo_event_index = 0;
    uint16_t ppq = 0;
  private:
    int push_back = -1;
    uint8_t prev_command = 0;
    BufferedReader* reader = NULL;

    double multiplierFromTempo(uint32_t tempo, uint16_t ppq);
};

class Midi
{
  public:
    Midi(const char* file_name);
    ~Midi();

    list<Note> note_buffer;
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
};