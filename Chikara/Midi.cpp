#include "Midi.h"

NoteColor colors[16] = {
  {51 / 255.0f, 102 / 255.0f, 255 / 255.0f},
  {255 / 255.0f, 126 / 255.0f, 51 / 255.0f},
  {51 / 255.0f, 255 / 255.0f, 102 / 255.0f},
  {255 / 255.0f, 51 / 255.0f, 129 / 255.0f},
  {51 / 255.0f, 255 / 255.0f, 255 / 255.0f},
  {228 / 255.0f, 51 / 255.0f, 255 / 255.0f},
  {153 / 255.0f, 255 / 255.0f, 51 / 255.0f},
  {75 / 255.0f, 51 / 255.0f, 255 / 255.0f},
  {255 / 255.0f, 204 / 255.0f, 51 / 255.0f},
  {51 / 255.0f, 180 / 255.0f, 255 / 255.0f},
  {255 / 255.0f, 51 / 255.0f, 51 / 255.0f},
  {51 / 255.0f, 255 / 255.0f, 177 / 255.0f},
  {255 / 255.0f, 51 / 255.0f, 204 / 255.0f},
  {78 / 255.0f, 255 / 255.0f, 51 / 255.0f},
  {153 / 255.0f, 51 / 255.0f, 255 / 255.0f},
  {231 / 255.0f, 255 / 255.0f, 51 / 255.0f}
};

#pragma region Midi Class

Midi::Midi(const char* file_name)
{
  //Open the file, seek to the end of the file and save that to a var, then return to the start of the file
  file_stream.open(file_name, std::ios::binary | std::ios::ate);
  file_stream.seekg(0, std::ios::end);
  file_end = file_stream.tellg();
  file_stream.seekg(0, std::ios::beg);

  //Load the file
  loadMidi();

  try
  {
    double seconds = 0;
    uint64_t time = 0;
    while (true) {
      bool all_ended = true;
      for (int i = 0; i < track_count; i++)
      {
        MidiTrack* track = readers[i];
        if (track->ended)
        {
          if (track->time > seconds) seconds = track->time;
          continue;
        }
        all_ended = false;
        if (!track->delta_parsed)
          track->parseDeltaTime();
        if (time < track->tick_time)
          continue;
        while (time >= track->tick_time) {
          track->parseEvent2ElectricBoogaloo(note_buffer);
          track->parseDeltaTime();
          if (track->ended)
            break;
        }
      }
      time++;
      if (all_ended) break;
    }

    std::cout << "Finished parsing\n";
    std::cout << "Seconds length: " << seconds << "\n";
    uint64_t notes = 0;
    for(int i = 0; i < track_count; i++)
    {
      notes += readers[i]->notes_parsed;
    }
    std::cout << "Notes parsed: " << notes;

  } catch(const char* e)
  {
    std::cout << e;
  }
}

Midi::~Midi()
{
  for(int i = 0; i < 256; i++)
  {
    std::list<Note*>* notes = note_buffer[i];
    while(!notes->empty())
    {
      delete notes->front();
      notes->pop_front();
    }
    delete notes;
  }
  delete note_buffer;
}

void Midi::loadMidi()
{
  try
  {
    assertText("MThd");
    uint32_t length = parseInt();
    if(length != 6) throw "\nCorrupt Header";

    format = parseShort();
    parseShort();
    ppq = parseShort();

    if(format == 2) throw "\nStop using fucking format 2";

    uint32_t count = 0;

    while(file_stream.tellg() != file_end)
    {
      assertText("MTrk");
      length = parseInt();
      size_t pos = file_stream.tellg();

      if(length + pos > file_end) length = file_end - pos;

      file_stream.seekg(pos + length, std::ios::beg);

      MidiChunk chunk;
      chunk.start = pos;
      chunk.length = length;
      tracks.push_back(chunk);
      count++;
    }

    track_count = count;

    //Parse all the tracks and the note cout for each track
    uint64_t nc = 0;
    uint32_t tc = 0;
    uint32_t tn = 0;

    MidiTrack** parse_tracks = new MidiTrack * [track_count];

    concurrency::parallel_for(uint32_t(0), track_count, [&](uint32_t i)
      {
        MidiTrack* track = new MidiTrack(&file_stream, tracks[i].start, tracks[i].length, 100000, i, ppq, &mtx);

        parse_tracks[i] = track;

        while (!track->ended)
        {
          track->parseDelta();
          track->parseEvent1();
        }

        mtx.lock();
        tc += track->tempo_events.size();
        tn++;

        std::cout << "\nParsed track " << tn << " note count " << track->notes_parsed;
        nc += (uint64_t)track->notes_parsed;
        mtx.unlock();
      });

    /*for(int i = 0; i < track_count; i++)
    {
      MidiTrack* track = new MidiTrack(&file_stream, tracks[i].start, tracks[i].length, 100000, i, ppq);

      parse_tracks[i] = track;

      while(!track->ended)
      {
        track->parseDelta();
        track->parseEvent1();
      }

      tc += track->tempo_events.size();

      std::cout << "\nParsed track " << i << " note count " << track->notes_parsed;
      nc += (uint64_t)track->notes_parsed;
    }*/

    std::cout << "\nTotal tempo events: " << tc;
    std::cout << "\nTotal notes: " << nc << std::endl;

    tempo_array = new Tempo[tc];
    tempo_count = tc;

    uint32_t t = 0;
    uint32_t* track_tempo_index = new uint32_t[track_count];
    for(int i = 0; i < track_count; i++) track_tempo_index[i] = 0;

    while(t < tempo_count)
    {
      int min = -1;
      uint64_t min_time = 0;
      for(int i = 0; i < track_count; i++)
      {
        if(track_tempo_index[i] < parse_tracks[i]->tempo_events.size())
        {
          uint64_t pos = parse_tracks[i]->tempo_events[track_tempo_index[i]].pos;
          if(min == -1 || min_time > pos)
          {
            min_time = pos;
            min = i;
          }
        }
      }
      if(min == -1)
      {
        std::cout << "Broke\n";
        break;
      }
      tempo_array[t++] = parse_tracks[min]->tempo_events[track_tempo_index[min]];
      track_tempo_index[min]++;
    }

    uint64_t tick_time = 0;
    for(int i = 0; i < track_count; i++) delete parse_tracks[i];
    delete[] parse_tracks;
    delete[] track_tempo_index;

    readers = new MidiTrack * [count];
    for(int i = 0; i < count; i++)
    {
      readers[i] = new MidiTrack(&file_stream, tracks[i].start, tracks[i].length, 100000, i, ppq, &mtx);
      readers[i]->global_tempo_events = tempo_array;
      readers[i]->global_tempo_event_count = tempo_count;
    }

    note_buffer = new std::list<Note*> * [256];
    for(int i = 0; i < 256; i++)
    {
      note_buffer[i] = new std::list<Note*>();
    }

  } catch(const char* e)
  {
    std::cout << "\n" << e;
  }
}

void Midi::assertText(const char* text)
{
  for(int i = 0; text[i] != 0; i++)
  {
    if(text[i] != readByte())
    {
      throw "\nCorrupt Chunk Header";
    }
  }
}

uint8_t Midi::readByte()
{
  uint8_t b;
  file_stream.read((char*)&b, 1);

  return b;
}

uint16_t Midi::parseShort()
{
  uint16_t length = 0;
  for(int i = 0; i != 2; i++)
  {
    length = (uint16_t)(length << 8) | (uint8_t)readByte();
  }

  return length;
}

uint32_t Midi::parseInt()
{
  uint32_t length = 0;
  for(int i = 0; i != 4; i++)
  {
    length = (uint32_t)(length << 8) | (uint8_t)readByte();
  }

  return length;
}

#pragma endregion

#pragma region BufferedReader class
BufferedReader::BufferedReader(std::ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size, std::mutex* _mtx)
{
  file_stream = _file_stream;
  start = _start;
  length = _length;
  buffer_size = _buffer_size;
  pos = start;
  mtx = _mtx;
  buffer_start = start;

  if (buffer_size > length) buffer_size = (uint32_t)length;
  buffer = new uint8_t[buffer_size];

  updateBuffer();
}

BufferedReader::~BufferedReader()
{
  delete[] buffer;
}

void BufferedReader::updateBuffer()
{
  uint32_t read = buffer_size;

  if ((pos + read) > (start + length))
    read = start + length - pos;

  if (read == 0)
    throw "\nOutside the buffer";

  mtx->lock();
  file_stream->seekg(pos, std::ios::beg);
  file_stream->read((char*)buffer, read);
  mtx->unlock();
  buffer_start = pos;
  buffer_pos = 0;
}

// origin = fseek origin (SEEK_SET or SEEK_CUR, no need for SEEK_END)
// storage as int64 is fine, nobody will have a midi over 9223 pb
void BufferedReader::seek(int64_t offset, int origin)
{
  if (origin != SEEK_SET && origin != SEEK_CUR)
    throw "Invalid seek origin!";

  int64_t real_offset = offset;
  if (origin == SEEK_SET)
    real_offset += start;
  else
    real_offset += pos;

  if (real_offset < (int64_t)start)
    throw "Attempted to seek before start!";
  if (real_offset > start + length)
    throw "Attempted to seek past end!";

  pos = real_offset;

  // buffer doesn't have to be remade if seeking between it already
  if (buffer_start <= real_offset && real_offset < buffer_start + buffer_size) {
    buffer_pos = pos - buffer_start;
    return;
  }

  updateBuffer();
}

void BufferedReader::read(uint8_t* dst, size_t size)
{
  if (pos + size > start + length)
    throw "Attempted to read past end!";
  if (size > buffer_size)
    throw "(UMIMPLEMENTED) Requested read size is larger than the buffer size!";

  if (buffer_start + buffer_pos + size > buffer_start + buffer_size)
    updateBuffer();

  memcpy(dst, buffer + buffer_pos, size);
  pos += size;
  buffer_pos += size;
}

uint8_t BufferedReader::readByte()
{
  uint8_t ret;
  read(&ret, 1);
  return ret;
}

void BufferedReader::skipBytes(size_t size) {
  seek(size, SEEK_CUR);
}

#pragma endregion

#pragma region Midi Track

MidiTrack::MidiTrack(std::ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size, uint32_t _track_num, uint16_t _ppq, std::mutex* _mtx)
{
  reader = new BufferedReader(_file_stream, _start, _length, _buffer_size, _mtx);
  ppq = _ppq;
  track_num = _track_num;
}

MidiTrack::~MidiTrack()
{
  delete reader;
}

void MidiTrack::parseDelta()
{
  if(ended) return;
  try
  {
    uint8_t c;
    uint32_t val = 0;
    while((c = reader->readByte()) > 0x7F)
    {
      val = (val << 7) | (c & 0x7F);
    }
    val = val << 7 | c;

    tick_time += val;
    delta_parsed = true;
  } catch(const char* e)
  {
    ended = true;
  }
}

void MidiTrack::initNoteStacks()
{
  note_stacks = new std::list<Note*> * [16 * 256];
  for(int i = 0; i < 16 * 256; i++)
  {
    note_stacks[i] = new std::list<Note*>();
  }
}

void MidiTrack::deleteNoteStacks()
{
  if(note_stacks != NULL)
  {
    for(int i = 0; i < 256 * 16; i++)
    {
      std::list<Note*>* stack = note_stacks[i];
      while(!stack->empty())
      {
        Note* n = stack->front();
        delete n;
        stack->pop_front();
      }
      delete note_stacks[i];
    }
    delete[] note_stacks;
    note_stacks = NULL;
  }
}

void MidiTrack::parseDeltaTime()
{
  if(ended) return;
  try
  {
    uint8_t c;
    uint32_t val = 0;
    while((c = reader->readByte()) > 0x7F)
    {
      val = (val << 7) | (c & 0x7F);
    }
    val = val << 7 | c;

    while(global_tempo_event_index < global_tempo_event_count && val + tick_time > global_tempo_events[global_tempo_event_index].pos)
    {
      Tempo t = global_tempo_events[global_tempo_event_index];

      uint64_t offset = t.pos - tick_time;
      val -= offset;
      time += offset * tempo_multiplier;
      tempo_multiplier = multiplierFromTempo(t.tempo, ppq);
      tick_time += offset;

      global_tempo_event_index++;
    }

    time += val * tempo_multiplier;
    tick_time += val;
    delta_parsed = true;
  } catch(const char* e)
  {
    ended = true;
  }
}

double MidiTrack::multiplierFromTempo(uint32_t tempo, uint16_t ppq)
{
  return tempo / 1000000.0 / ppq;
}

void MidiTrack::parseEvent1()
{
  if(ended)
  {
    return;
  }
  try
  {
    delta_parsed = false;
    uint8_t command = reader->readByte();
    if(command < 0x80)
    {
      reader->seek(-1, SEEK_CUR);
      command = prev_command;
    }

    prev_command = command;

    uint8_t cmd = (uint8_t)(command & 0xF0);
    uint8_t channel = (uint8_t)(command & 0x0F);

    switch(cmd)
    {
      case 0x80:
        reader->readByte();
        reader->readByte();
        break;
      case 0x90:
        reader->readByte();
        if(reader->readByte() > 0) notes_parsed++;
        break;
      case 0xA0:
        reader->readByte();
        reader->readByte();
        break;
      case 0xB0:
        reader->readByte();
        reader->readByte();
        break;
      case 0xC0:
        reader->readByte();
        break;
      case 0xD0:
        reader->readByte();
        break;
      case 0xE0:
        reader->readByte();
        reader->readByte();
        break;
      default:
      {
        switch(command)
        {
          case 0xFF:
          {
            uint8_t command2 = reader->readByte();

            uint8_t c;
            uint32_t val = 0;
            while((c = reader->readByte()) > 0x7F)
            {
              val = (val << 7) | (c & 0x7F);
            }
            val = val << 7 | c;

            switch(command2)
            {
              case 0x51:
              {
                uint32_t tempo = 0;
                for (int i = 0; i != 3; i++)
                  tempo = (uint32_t)((tempo << 8) | reader->readByte());
                //Tempo

                Tempo t;
                t.pos = tick_time;
                t.tempo = tempo;
                tempo_events.push_back(t);
                break;
              }
              // huge block of copy + paste below, until these two functions get merged...
              case 0x00:
                // Sequence number
                reader->skipBytes(2);
                break;
              case 0x01: // Text
              case 0x02: // Copyright info
              case 0x03: // Track name
              case 0x04: // Track instrument name
              case 0x05: // Lyric
              case 0x06: // Marker
              case 0x07: // Cue point
              case 0x7F: // Sequencer-specific information
                reader->skipBytes(val);
                break;
              case 0x20: // MIDI Channel prefix
                reader->skipBytes(1);
                break;
              case 0x21: // MIDI Port
                reader->skipBytes(1);
                break;
              case 0x2F:
                // End of track
                ended = true;
                break;
              case 0x54:
                // SMPTE Offset
                reader->skipBytes(5);
                break;
              case 0x58:
                // Time signature
                reader->skipBytes(4);
                break;
              case 0x59:
                // Key signature
                reader->skipBytes(2);
                break;

              default:
              {
                printf("\nUnknown meta-event 0x%x", command2);
                reader->skipBytes(val);
                break;
              }
            }
            break;
          }
          case 0xF0:
            while(reader->readByte() != 0xF7);
            break;
          case 0xF2:
            reader->readByte();
            reader->readByte();
            break;
          case 0xF3:
            reader->readByte();
            break;
          default:
            break;
        }
        break;
      }
    }
  } catch(const char* e)
  {
    std::cout << e;
    ended = true;
  }
}

void MidiTrack::parseEvent2ElectricBoogaloo(std::list<Note*>** global_notes)
{
  if(ended)
  {
    return;
  }
  try
  {
    delta_parsed = false;
    uint8_t command = reader->readByte();
    if(command < 0x80)
    {
      reader->seek(-1, SEEK_CUR);
      command = prev_command;
    }

    prev_command = command;

    uint8_t cmd = (uint8_t)(command & 0xF0);
    uint8_t channel = (uint8_t)(command & 0x0F);

    switch(cmd)
    {
      case 0x80:
      {
        uint8_t key = reader->readByte();
        uint8_t vel = reader->readByte();
        if(note_stacks == NULL) return;

        std::list<Note*>* stack = note_stacks[channel * 256 + key];

        if(!stack->empty())
        {
          Note* n = stack->front();
          stack->pop_front();
          n->end = time;
        }

        // Note Off
        break;
      }
      case 0x90:
      {
        uint8_t key = reader->readByte();
        uint8_t vel = reader->readByte();
        // Note On

        if(note_stacks == NULL) initNoteStacks();
        std::list<Note*>* stack = note_stacks[channel * 256 + key];

        if(vel > 0)
        {

          Note* n = new Note();
          n->color = colors[channel];
          n->start = time;
          n->end = -1;
          n->key = key;
          n->channel = channel;
          n->velocity = vel;

          stack->push_back(n);
          global_notes[key]->push_back(n);

          notes_parsed++;
        }
        else
        {
          if(!stack->empty())
          {
            Note* n = stack->front();
            stack->pop_front();
            n->end = time;
          }
        }
        break;
      }
      case 0xA0:
      {
        uint8_t key = reader->readByte();
        uint8_t vel = reader->readByte();
        // Polyphonic Pressure
        break;
      }
      case 0xB0:
      {
        uint8_t controller = reader->readByte();
        uint8_t value = reader->readByte();
        // Controller
        break;
      }
      case 0xC0:
      {
        uint8_t program = reader->readByte();
        // Program Change
        break;
      }
      case 0xD0:
      {
        uint8_t pressure = reader->readByte();
        // Channel Pressure
        break;
      }
      case 0xE0:
      {
        uint8_t val1 = reader->readByte();
        uint8_t val2 = reader->readByte();
        // Pitch Wheel
        break;
      }
      case 0xF0:
      {
        switch(command)
        {
          case 0xFF:
          {
            uint8_t command2 = reader->readByte();

            uint8_t c;
            uint32_t val = 0;
            while((c = reader->readByte()) > 0x7F)
            {
              val = (val << 7) | (c & 0x7F);
            }
            val = val << 7 | c;

            switch(command2)
            {
              case 0x51:
              {
                uint32_t tempo = 0;
                for(int i = 0; i != 3; i++)
                  tempo = (uint32_t)((tempo << 8) | reader->readByte());
                //Tempo
                break;
              }
              case 0x00:
                // Sequence number
                reader->skipBytes(2);
                break;
              case 0x01: // Text
              case 0x02: // Copyright info
              case 0x03: // Track name
              case 0x04: // Track instrument name
              case 0x05: // Lyric
              case 0x06: // Marker
              case 0x07: // Cue point
              case 0x7F: // Sequencer-specific information
                reader->skipBytes(val);
                break;
              case 0x20: // MIDI Channel prefix
                reader->skipBytes(1);
                break;
              case 0x21: // MIDI Port
                reader->skipBytes(1);
                break;
              case 0x2F:
                // End of track
                ended = true;
                break;
              case 0x54:
                // SMPTE Offset
                reader->skipBytes(5);
                break;
              case 0x58:
                // Time signature
                reader->skipBytes(4);
                break;
              case 0x59:
                // Key signature
                reader->skipBytes(2);
                break;
              default:
                printf("%x\n", command2);
                throw "yell at kaydax for making these two different functions\n";
                break;
              }
            }
            break;
          case 0xF0:
            while(reader->readByte() != 0xF7);
            break;
          case 0xF2:
            reader->readByte();
            reader->readByte();
            break;
          case 0xF3:
            reader->readByte();
            break;
          default:
            throw std::runtime_error("Unknown system event!");
            break;
        }
        break;
      }
      default:
        throw std::runtime_error("Unknown event!");
    }
  } catch(const char* e)
  {
    std::cout << e;
    ended = true;
  }
  if(ended)
  {
    deleteNoteStacks();
  }
}

#pragma endregion
