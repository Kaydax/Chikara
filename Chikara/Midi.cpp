#include "Midi.h"

#pragma region Midi Class

Midi::Midi(const char* file_name)
{
  //Open the file, seek to the end of the file and save that to a var, then return to the start of the file
  file_stream.open(file_name, ios::binary | ios::ate);
  file_stream.seekg(0, ios::end);
  file_end = file_stream.tellg();
  file_stream.seekg(0, ios::beg);

  loadMidi(); //Load the file

  try
  {
    double seconds = 0;
    uint64_t time = 0;
    for(int i = 0; i < track_count; i++)
    {
      readers[i]->parseDelta();
    }
    while(true)
    {
      bool all_ended = true;
      for(int i = 0; i < track_count; i++)
      {
        MidiTrack* track = readers[i];
        if(track->ended)
        {
          if(track->time > seconds) seconds = track->time;
          continue;
        }
        all_ended = false;
        if(!track->delta_parsed) track->parseDeltaTime();
        if(time >= track->tick_time)
        {
          track->parseEvent2ElectricBoogaloo();
        }
      }
      time++;
      if(all_ended) break;
    }

    cout << "Finished parsing\n";
    cout << "Seconds length: " << seconds << "\n";
    uint64_t notes = 0;
    for(int i = 0; i < track_count; i++)
    {
      notes += readers[i]->notes_parsed;
    }
    cout << "Notes parsed: " << notes;

  } catch(const char* e)
  {
    cout << e;
  }
}

Midi::~Midi()
{
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

      file_stream.seekg(pos + length, ios::beg);

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

    MidiTrack** parse_tracks = new MidiTrack * [track_count];

    for(int i = 0; i < track_count; i++)
    {
      MidiTrack* track = new MidiTrack(&file_stream, tracks[i].start, tracks[i].length, 100000, ppq);

      parse_tracks[i] = track;

      while(!track->ended)
      {
        track->parseDelta();
        track->parseEvent1();
      }

      tc += track->tempo_events.size();

      cout << "\nParsed track " << i << " note count " << track->notes_parsed;
      nc += (uint64_t)track->notes_parsed;
    }

    cout << "\nTotal tempo events: " << tc;
    cout << "\nTotal notes: " << nc << endl;

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
        cout << "Broke\n";
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
      readers[i] = new MidiTrack(&file_stream, tracks[i].start, tracks[i].length, 100000, ppq);
      readers[i]->global_tempo_events = tempo_array;
      readers[i]->global_tempo_event_count = tempo_count;
    }

  } catch(const char* e)
  {
    cout << "\n" << e;
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

BufferedReader::BufferedReader(ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size)
{
  file_stream = _file_stream;
  start = _start;
  length = _length;
  buffer_size = _buffer_size;
  pos = start;

  if(buffer_size > length) buffer_size = (uint32_t)length;

  buffer = new uint8_t[buffer_size];

  updateBuffer();
}

BufferedReader::~BufferedReader()
{
  delete[] buffer;
}

uint8_t BufferedReader::readByte()
{
  if(push_back != -1)
  {
    uint8_t b = (uint8_t)push_back;
    push_back = -1;
    return b;
  }
  return readByteFast();
}

uint8_t BufferedReader::readByteFast()
{
  if(buffer_pos >= buffer_size)
  {
    updateBuffer();
    buffer_pos = 0;
  }

  return buffer[buffer_pos++];
}

void BufferedReader::updateBuffer()
{
  uint32_t read = buffer_size;

  if((pos + read) > (start + length)) read = start + length - pos;

  if(read == 0) throw "\nOutside the buffer";

  file_stream->seekg(pos, ios::beg);
  file_stream->read((char*)buffer, read);
  pos += read;
}

#pragma endregion

#pragma region Midi Track

MidiTrack::MidiTrack(ifstream* _file_stream, size_t _start, size_t _length, uint32_t _buffer_size, uint16_t _ppq)
{
  reader = new BufferedReader(_file_stream, _start, _length, _buffer_size);
  ppq = _ppq;
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
    while((c = reader->readByteFast()) > 0x7F)
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

void MidiTrack::parseDeltaTime()
{
  if(ended) return;
  try
  {
    uint8_t c;
    uint32_t val = 0;
    while((c = reader->readByteFast()) > 0x7F)
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
    uint8_t command = reader->readByteFast();
    if(command < 0x80)
    {
      push_back = command;
      command = prev_command;
    }

    prev_command = command;

    uint8_t cmd = (uint8_t)(command & 0xF0);
    uint8_t channel = (uint8_t)(command & 0x0F);

    switch(cmd)
    {
      case 0x80:
        reader->readByte();
        reader->readByteFast();
        break;
      case 0x90:
        reader->readByte();
        if(reader->readByteFast() > 0) notes_parsed++;
        break;
      case 0xA0:
        reader->readByte();
        reader->readByteFast();
        break;
      case 0xB0:
        reader->readByte();
        reader->readByteFast();
        break;
      case 0xC0:
        reader->readByte();
        break;
      case 0xD0:
        reader->readByte();
        break;
      case 0xE0:
        reader->readByte();
        reader->readByteFast();
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
            while((c = reader->readByteFast()) > 0x7F)
            {
              val = (val << 7) | (c & 0x7F);
            }
            val = val << 7 | c;

            switch(command2)
            {
              case 0x2F:
                ended = true;
                break;
              case 0x51:
              {
                uint32_t tempo = 0;
                for(int i = 0; i != 3; i++)
                  tempo = (uint32_t)((tempo << 8) | reader->readByteFast());
                //Tempo

                Tempo t;
                t.pos = tick_time;
                t.tempo = tempo;
                tempo_events.push_back(t);
                break;
              }
              default:
              {
                for(int i = 0; i < val; i++)
                {
                  reader->readByteFast();
                }
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
            reader->readByteFast();
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
    cout << e;
    ended = true;
  }
}

void MidiTrack::parseEvent2ElectricBoogaloo()
{
  if(ended)
  {
    return;
  }
  try
  {
    delta_parsed = false;
    uint8_t command = reader->readByteFast();
    if(command < 0x80)
    {
      push_back = command;
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
        uint8_t vel = reader->readByteFast();
        // Note Off
        break;
      }
      case 0x90:
      {
        uint8_t key = reader->readByte();
        uint8_t vel = reader->readByteFast();
        // Note On

        if(vel > 0) notes_parsed++;

        break;
      }
      case 0xA0:
      {
        uint8_t key = reader->readByte();
        uint8_t vel = reader->readByteFast();
        // Polyphonic Pressure
        break;
      }
      case 0xB0:
      {
        uint8_t controller = reader->readByte();
        uint8_t value = reader->readByteFast();
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
        uint8_t val2 = reader->readByteFast();
        // Pitch Wheel
        break;
      }
      default:
      {
        switch(command)
        {
          case 0xFF:
          {
            uint8_t command2 = reader->readByte();

            uint8_t c;
            uint32_t val = 0;
            while((c = reader->readByteFast()) > 0x7F)
            {
              val = (val << 7) | (c & 0x7F);
            }
            val = val << 7 | c;

            switch(command2)
            {
              case 0x2F:
                ended = true;
                break;
              case 0x51:
              {
                uint32_t tempo = 0;
                for(int i = 0; i != 3; i++)
                  tempo = (uint32_t)((tempo << 8) | reader->readByteFast());
                //Tempo
                break;
              }
              default:
              {
                for(int i = 0; i < val; i++)
                {
                  reader->readByteFast();
                }
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
            reader->readByteFast();
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
    cout << e;
    ended = true;
  }
}

#pragma endregion
