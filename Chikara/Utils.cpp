#include "Utils.h"

std::string Utils::GetFileName(std::filesystem::path file_path)
{
  return std::filesystem::path(file_path).filename().string();
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

void Utils::destroyDiscord()
{
  Discord_Shutdown();
}
