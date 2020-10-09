#include "Utils.h"

std::wstring Utils::GetFileName(std::filesystem::path file_path)
{
  return std::filesystem::path(file_path).filename().wstring();
}

static void handleDiscordReady(const DiscordUser* connectedUser)
{
  printf("\nDiscord: connected to user %s#%s - %s\n",
         connectedUser->username,
         connectedUser->discriminator,
         connectedUser->userId);
}

static void handleDiscordDisconnected(int errcode, const char* message)
{
  printf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
}

static void handleDiscordError(int errcode, const char* message)
{
  printf("\nDiscord: error (%d: %s)\n", errcode, message);
}

void Utils::InitDiscord()
{
  DiscordEventHandlers handlers;
  memset(&handlers, 0, sizeof(handlers));
  handlers.ready = handleDiscordReady;
  handlers.errored = handleDiscordError;
  handlers.disconnected = handleDiscordDisconnected;

  // Discord_Initialize(const char* applicationId, DiscordEventHandlers* handlers, int autoRegister, const char* optionalSteamId)
  Discord_Initialize("732538738122424321", &handlers, 1, NULL);
}

void Utils::UpdatePresence(const char* state, const char* details, std::string file_name, uint64_t start_time, uint64_t end_time)
{
  const char* playing_text = std::string(details + file_name).c_str();

  DiscordRichPresence discordPresence;
  memset(&discordPresence, 0, sizeof(discordPresence));
  discordPresence.state = state;
  discordPresence.details = playing_text;
  discordPresence.startTimestamp = start_time;
  discordPresence.endTimestamp = end_time;
  Discord_UpdatePresence(&discordPresence);
}

void Utils::DestroyDiscord()
{
  Discord_Shutdown();
}

rgb Utils::HSVtoRGB(float H, float S, float V)
{
  float s = S;
  float v = V;
  float C = s * v;
  float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
  float m = v - C;
  float r, g, b;
  rgb out;

  if(H >= 0 && H < 60)
  {
    r = C, g = X, b = 0;
  }
  else if(H >= 60 && H < 120)
  {
    r = X, g = C, b = 0;
  }
  else if(H >= 120 && H < 180)
  {
    r = 0, g = C, b = X;
  }
  else if(H >= 180 && H < 240)
  {
    r = 0, g = X, b = C;
  }
  else if(H >= 240 && H < 300)
  {
    r = X, g = 0, b = C;
  }
  else
  {
    r = C, g = 0, b = X;
  }

  int R = (r + m) * 255;
  int G = (g + m) * 255;
  int B = (b + m) * 255;

  out.r = R;
  out.g = B;
  out.b = G;

  return out;
}

std::string Utils::wstringToUtf8(std::wstring str)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  return conv.to_bytes(str);
}