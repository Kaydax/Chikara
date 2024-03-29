#include <shlwapi.h>
#include "Config.h"

Config g_config;

std::string Config::GetConfigPath() {
  char temp[1024] = {};
  if (GetModuleFileNameA(NULL, temp, 1024)) {
    // get the directory of the executable
    char* temp2 = _strdup(temp);
    PathRemoveFileSpecA(temp2);
    std::string path(temp2);
    free(temp2);

    return path + "\\config.ini";
  }
  // if the user is running chikara in a way that GetModuleFileName fails, it's best not to save at all...
  return "";
}

void Config::Load(std::string& path) {
  INIReader reader(path.c_str());

  vsync = reader.GetBoolean("Chikara", "VSync", vsync);
  fullscreen = reader.GetBoolean("Chikara", "Fullscreen", fullscreen);
  transparent = reader.GetBoolean("Chikara", "Transparent", transparent);
  note_hide = reader.GetBoolean("Chikara", "NoteHide", note_hide);
  rainbow_bar = reader.GetBoolean("Chikara", "RainbowBar", rainbow_bar);
  discord_rpc = reader.GetBoolean("Chikara", "RPC", discord_rpc);
  markers = reader.GetBoolean("Chikara", "Markers", markers);
  bar_color = GetVec3(reader, "Chikara", "BarColor", bar_color);
  clear_color = GetVec3(reader, "Chikara", "ClearColor", clear_color);
  note_speed = GetFloat(reader, "Chikara", "NoteSpeed", note_speed);
  rainbow_speed = GetFloat(reader, "Chikara", "RainbowSpeed", rainbow_speed);
  start_delay = GetFloat(reader, "Chikara", "StartDelay", start_delay);
  loader_buffer = GetFloat(reader, "Chikara", "LoaderBuffer", loader_buffer);

  config_path = path;
}

bool Config::Save() {
  if (config_path.empty())
    return false;

  FILE* ini = fopen(config_path.c_str(), "w");
  if (!ini)
    return false;

  fprintf(ini, "[Chikara]\n");

  WriteBool(ini, "VSync", vsync);
  WriteBool(ini, "Fullscreen", fullscreen);
  WriteBool(ini, "Transparent", transparent);
  WriteBool(ini, "NoteHide", note_hide);
  WriteBool(ini, "RainbowBar", rainbow_bar);
  WriteBool(ini, "RPC", discord_rpc);
  WriteBool(ini, "Markers", markers);
  WriteVec3(ini, "BarColor", bar_color);
  WriteVec3(ini, "ClearColor", clear_color);
  WriteFloat(ini, "NoteSpeed", note_speed);
  WriteFloat(ini, "RainbowSpeed", rainbow_speed);
  WriteFloat(ini, "StartDelay", start_delay);
  WriteFloat(ini, "LoaderBuffer", loader_buffer);

  fclose(ini);

  return true;
}

glm::vec3 Config::GetVec3(INIReader& reader, const std::string& section, const std::string& name, glm::vec3& default_value) {
  glm::vec3 result;
  result.r = reader.GetReal(section, name + "R", default_value.r);
  result.g = reader.GetReal(section, name + "G", default_value.g);
  result.b = reader.GetReal(section, name + "B", default_value.b);
  return result;
}

float Config::GetFloat(INIReader& reader, const std::string& section, const std::string& name, float& default_value)
{
  float result = reader.GetReal(section, name, default_value);
  return result;
}

void Config::WriteBool(FILE* file, const std::string& name, bool b) {
  if (b)
    fprintf(file, "%s = true\n", name.c_str());
  else
    fprintf(file, "%s = false\n", name.c_str());
}

void Config::WriteVec3(FILE* file, const std::string& name, glm::vec3& value) {
  fprintf(file, "%sR = %f\n", name.c_str(), value.r);
  fprintf(file, "%sG = %f\n", name.c_str(), value.g);
  fprintf(file, "%sB = %f\n", name.c_str(), value.b);
}

void Config::WriteFloat(FILE* file, const std::string& name, float value)
{
  fprintf(file, "%s = %f\n", name.c_str(), value);
}