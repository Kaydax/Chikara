#pragma once
#include <string>
#include <glm/glm.hpp>
#include <ini.h>
#include <cpp/INIReader.h>

class Config {
public:
  static Config& GetConfig() {
    static Config config;
    return config;
  };
  static std::string GetConfigPath();
  void Load(std::string& path);
  bool Save();
  std::string config_path;

  bool vsync = false;
  bool note_hide = false;
  bool rainbow_bar = false;
  glm::vec3 bar_color = glm::vec3(0.0f / 255, 196.0f / 255, 177.0f / 255);
private:
  glm::vec3 GetVec3(INIReader& reader, const std::string& section, const std::string& name, glm::vec3& default_value);

  void WriteBool(FILE* file, const std::string& name, bool value);
  void WriteVec3(FILE* file, const std::string& name, glm::vec3& value);
};