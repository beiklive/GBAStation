#include "RetroAchievements.h"
#include <fstream>
#include <string>
#include <switch.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include "stb_image/stb_image.h"
#include "PlatformConfig.h"
#include "ROMMetaDatabase.h"
#include "NotificationSystem.h"
#include "TriggerNotification.h"
#include "NDS.h"
#include "RATracker.h"
#include "version.h"

#define STB_IMAGE_IMPLEMENTATION

extern "C" {
    #include <rc_api_user.h>
    #include <rc_api_runtime.h>
    #include <rc_client.h>
    #include <rc_consoles.h>
}

static std::string g_response;
rc_client_t* g_client = NULL;
static bool g_login_successful = false;
bool g_loadAchievements = true;
std::vector<Achievement> g_achievements;
const char* client = "Gheovgos-melonDS/" PROJECT_VERSION " (Switch NX)";

/* -------------- SERVER COMUNICATION --------------*/

bool IsNetworkAvailable() {
    Result rc;

    NifmInternetConnectionType connType;
    u32 wifiStrength = 0;
    NifmInternetConnectionStatus connStatus;

    rc = nifmInitialize(NifmServiceType_User);
    if (R_FAILED(rc)) {
        return false;
    }

    rc = nifmGetInternetConnectionStatus(&connType, &wifiStrength, &connStatus);
    nifmExit();

    if (R_SUCCEEDED(rc)) {
        return (connStatus == NifmInternetConnectionStatus_Connected);
    }

    return false;
}

static size_t switch_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    g_response.append((char*)contents, realsize);
    return realsize;
}

static size_t switch_curl_write_image_callback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t realsize = size * nmemb;
  std::vector<unsigned char>* image_data = static_cast<std::vector<unsigned char>*>(userp);
  image_data->insert(image_data->end(), (unsigned char*)contents, (unsigned char*)contents + realsize);
  return realsize;
}

static void server_error(const rc_client_server_error_t* error)
{
  char buffer[256];
  printf("%s: %s", error->api, error->error_message);
  g_notification.Show(error->error_message);
}

// This for download icons from rcheevos APIs
int DownloadAndPackAvatar(const char* url,  int* outWidth, int* outHeight) {
  CURL* curl = curl_easy_init();
  if (!curl)
      return -1;
  
  std::vector<unsigned char> image_data;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, switch_curl_write_image_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &image_data);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, client);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK || image_data.empty())
    return -1;

  int width, height, channels;
  unsigned char* pixels = stbi_load_from_memory(image_data.data(), image_data.size(), &width, &height, &channels, 4);
  if (!pixels)
      return -1;

  *outWidth = width;
  *outHeight = height;

  int textureId = Gfx::TextureCreate(width, height, DkImageFormat_RGBA8_Unorm);
  Gfx::TextureUpload(textureId, 0, 0, width, height, pixels, width * 4);
  stbi_image_free(pixels);

  return textureId;
}

const char* send_http_request(const char* url, const char* post_data, int* status_code) {
    g_response.clear();

    CURL* curl = curl_easy_init();
    if (!curl) {
        *status_code = -1;
        return nullptr;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, switch_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, client);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        *status_code = -1;
        curl_easy_cleanup(curl);
        return nullptr;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    *status_code = static_cast<int>(http_code);

    curl_easy_cleanup(curl);

    char* response_cstr = static_cast<char*>(malloc(g_response.size() + 1));
    if (response_cstr) {
        std::memcpy(response_cstr, g_response.c_str(), g_response.size() + 1);
    }

    return response_cstr;
}

void store_retroachievements_credentials(const char* username, const char* token) {
  strncpy(Config::RetroAchievementsUsername, username, sizeof(Config::RetroAchievementsUsername) - 1);
  Config::RetroAchievementsUsername[sizeof(Config::RetroAchievementsUsername) - 1] = '\0';

  strncpy(Config::RetroAchievementsToken, token, sizeof(Config::RetroAchievementsToken) - 1);
  Config::RetroAchievementsToken[sizeof(Config::RetroAchievementsToken) - 1] = '\0';
}

static void server_call(const rc_api_request_t* request, rc_client_server_callback_t callback, void* callback_data, rc_client_t* client) {
  int status_code;
  const char* response = send_http_request(request->url, request->post_data, &status_code);

  rc_api_server_response_t server_response;
  memset(&server_response, 0, sizeof(server_response));
  server_response.http_status_code = status_code;
  server_response.body = response;
  server_response.body_length = response ? strlen(response) : 0;

  callback(&server_response, callback_data);

  if (response) {
  free((void*)response);
  }
}

std::vector<Achievement> achievements_list() {
  g_loadAchievements = false;
  g_notification.Show("Fetching achievements list...");

  std::vector<Achievement> achievements;
  char url[128];

  rc_client_achievement_list_t* list = rc_client_create_achievement_list(
      g_client,
      RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE_AND_UNOFFICIAL,
      RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_PROGRESS
  );

  if (!list) {
      return achievements;
  }

  for (int i = 0; i < list->num_buckets; i++) {
      for (int j = 0; j < list->buckets[i].num_achievements; j++) {
          const rc_client_achievement_t* achievement = list->buckets[i].achievements[j];
          Achievement ach;

          ach.title = achievement->title ? achievement->title : "Unknown Title";
          ach.description = achievement->description ? achievement->description : "No Description";

          if (list->buckets[i].bucket_type == RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED) {
              ach.progress = "Unsupported";
          } else if (achievement->unlocked) {
              ach.progress = "Unlocked";
          } else if (achievement->measured_percent) {
              ach.progress = achievement->measured_progress;
          } else {
              ach.progress = "Locked";
          }

          if (rc_client_achievement_get_image_url(achievement, achievement->state, url, sizeof(url)) == RC_OK) {
            ach.textureId = DownloadAndPackAvatar(url, &ach.width, &ach.height);
          } else {
              ach.textureId = -1;
          }

          achievements.push_back(ach);
      }
  }

  rc_client_destroy_achievement_list(list);
  return achievements;
}

/* -------------- LEADERBOARD --------------*/

static void leaderboard_started(const rc_client_leaderboard_t* leaderboard)
{
  printf("Leaderboard attempt started: %s - %s", leaderboard->title, leaderboard->description);
  fflush(stdout);
}

static void leaderboard_failed(const rc_client_leaderboard_t* leaderboard)
{
  printf("Leaderboard attempt failed: %s", leaderboard->title);
  fflush(stdout);
}

static void leaderboard_submitted(const rc_client_leaderboard_t* leaderboard)
{
  printf("Submitted %s for %s", leaderboard->tracker_value, leaderboard->title);
  fflush(stdout);
}

static void leaderboard_tracker_update(const rc_client_leaderboard_tracker_t* tracker)
{
  printf("Tracker %d updated: %s", tracker->id, tracker->display);
  fflush(stdout);
  std::string trackerId = std::to_string(tracker->id);

  LeaderboardTracker* data = find_tracker(trackerId);
  if (data) {
    data->Update(tracker->display);
  }
}

static void leaderboard_tracker_show(const rc_client_leaderboard_tracker_t* tracker)
{
  printf("Tracker %d show: %s", tracker->id, tracker->display);
  fflush(stdout);
  std::string trackerId = std::to_string(tracker->id);

  create_tracker(trackerId, tracker->display);
}

static void leaderboard_tracker_hide(const rc_client_leaderboard_tracker_t* tracker)
{
  std::string trackerId = std::to_string(tracker->id);

  destroy_tracker(trackerId);
}

// Multiple challenge indicators may be shown, but only one per achievement, so key the list on the achievement ID
static void challenge_indicator_show(const rc_client_achievement_t* achievement)
{
  char url[128];
  int width, height;

  if (rc_client_achievement_get_image_url(achievement, RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED, url, sizeof(url)) == RC_OK)
  {
    int textureId = DownloadAndPackAvatar(url, &width, &height);
    if (textureId >= 0) {
      if (achievement->state == RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE) {
        g_triggerNotifications.UpdateState(achievement->id, textureId, width, height, true);
      }
    } else {
      g_notification.Show("%d", achievement->id);
    }
  }
}

static void challenge_indicator_hide(const rc_client_achievement_t* achievement)
{
  g_triggerNotifications.RemoveNotificationById(achievement->id);
}

// The UPDATE event assumes the indicator is already visible, and just asks us to update the image/text.
static void progress_indicator_update(const rc_client_achievement_t* achievement)
{
  char url[128];
  int width, height;

  if (rc_client_achievement_get_image_url(achievement, RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE, url, sizeof(url)) == RC_OK)
  {
    int textureId = DownloadAndPackAvatar(url, &width, &height);
    if (textureId >= 0) {
      g_notification.ShowWithIcon(textureId, width, height, "%s\n%s", achievement->title, achievement->measured_progress);
    } else {
      g_notification.Show("%s", achievement->measured_progress);
    }
  }
}

static void progress_indicator_show(const rc_client_achievement_t* achievement)
{
  // The SHOW event tells us the indicator was not visible, but should be now.
  // To reduce duplicate code, we just update the non-visible indicator, then show it.
  progress_indicator_update(achievement);
}

static void progress_indicator_hide(void)
{
  // The HIDE event indicates the indicator should no longer be visible.
  // g_notification.DestroyNotifiation();
  
}

static void game_mastered(void)
{
  char message[128], submessage[128];
  char url[128];
  int textureId = -1, width, height;
  
  const rc_client_game_t* game = rc_client_get_game_info(g_client);

  if (rc_client_game_get_image_url(game, url, sizeof(url)) == RC_OK)
  {
    int textureId = DownloadAndPackAvatar(url, &width, &height);
    if (Config::hardcoreMode == 1)
      g_notification.ShowWithIcon(textureId, width, height, "Mastered %s\n%s", game->title, rc_client_get_user_info(g_client)->display_name);
    else
      g_notification.ShowWithIcon(textureId, width, height, "Completed %s\n%s", game->title, rc_client_get_user_info(g_client)->display_name);

  }

  // play_sound("mastery.wav");
}
/* -------------- LOGIC PROCESSING AND MEMORY --------------*/
static void achievement_triggered(const rc_client_achievement_t* achievement)
{
  char url[128];
  const char* message = "Achievement Unlocked";

  // the runtime already took care of dispatching the server request to notify the
  // server, we just have to tell the player.

  if (rc_client_achievement_get_image_url(achievement, RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED, url, sizeof(url)) == RC_OK) {
    int width, height;
    int textureId = DownloadAndPackAvatar(url, &width, &height);
    
    if (textureId >= 0) {
      g_notification.ShowWithIcon(textureId, width, height, "%s\n%s", message, achievement->title);
    } else {
      g_notification.Show("%s\n%s", message, achievement->title);
    }
  } else {
    g_notification.Show("%s\n%s", message, achievement->title);
  }

  // it's nice to also give an audio cue when an achievement is unlocked
  // TODO LATER play_sound("unlock.wav");
}

static void event_handler(const rc_client_event_t* event, rc_client_t* client)
{
  switch (event->type){
    case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
      printf("RA: Achievement triggered: %s\n", event->achievement->title);
      achievement_triggered(event->achievement);
      break;
    case RC_CLIENT_EVENT_LEADERBOARD_STARTED:
      leaderboard_started(event->leaderboard);
      break;
    case RC_CLIENT_EVENT_LEADERBOARD_FAILED:
      leaderboard_failed(event->leaderboard);
      break;
    case RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED:
      leaderboard_submitted(event->leaderboard);
      break;
    case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE:
      leaderboard_tracker_update(event->leaderboard_tracker);
      break;
    case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW:
      leaderboard_tracker_show(event->leaderboard_tracker);
      break;
    case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE:
      leaderboard_tracker_hide(event->leaderboard_tracker);
      break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_SHOW:
      challenge_indicator_show(event->achievement);
      break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE:
      challenge_indicator_hide(event->achievement);
      break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW:
      progress_indicator_show(event->achievement);
      break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
      progress_indicator_update(event->achievement);
      break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE:
      progress_indicator_hide();
      break;
    case RC_CLIENT_EVENT_GAME_COMPLETED:
      game_mastered();
      break;
    case RC_CLIENT_EVENT_SERVER_ERROR:
      server_error(event->server_error);
      break;
    default:
      printf("RA: Unhandled event (%d)\n", event->type);
      break;
  }
  fflush(stdout);
}


uint32_t read_memory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client)
{
  for (uint32_t i = 0; i < num_bytes; ++i) {
    uint32_t addr = address + i;
    if (addr >= 0x00000000 && addr < 0x00400000) {
      // Main RAM (0x02000000 - 0x02400000)
      buffer[i] = NDS::MainRAM[addr];
    } else if (addr >= 0x03000000 && addr < 0x03008000) {
      // Shared WRAM (0x03000000 - 0x03008000)
      buffer[i] = NDS::SharedWRAM[addr - 0x03000000];
    } else if (addr >= 0x03800000 && addr < 0x03810000) {
      // ARM7 WRAM (0x03800000 - 0x03810000)
      buffer[i] = NDS::ARM7WRAM[addr - 0x03800000];
    } else {
      buffer[i] = 0;
      }
  }
  return num_bytes;
}

void capture_retroachievements_state(FILE* file)
{
  const uint32_t buffer_size = (uint32_t)rc_client_progress_size(g_client);
  uint8_t* buffer = (uint8_t*)malloc(buffer_size);
  if (rc_client_serialize_progress(g_client, buffer) == RC_OK) {
    // write a marker, the buffer size, then the buffer contents
    fwrite("RCHV", 1, 4, file);
    fwrite(&buffer_size, 1, sizeof(buffer_size), file); 
    fwrite(buffer, 1, buffer_size, file);
  }
  free(buffer);
}

int restore_retroachievements_state(FILE* file)
{
  char marker[4];
  uint32_t buffer_size;
  uint8_t* buffer = NULL;
  int result = 0;
   
  if (fread(marker, 1, 4, file) == 4 && memcmp(marker, "RCHV", 4) == 0) {
    // found achievement data marker
    if (fread(&buffer_size, 1, sizeof(buffer_size), file) == sizeof(buffer_size)) {
      // allocate buffer
      buffer = (uint8_t*)malloc(buffer_size);
      if (buffer) {
        if (fread(buffer, 1, buffer_size, file) != buffer_size) {
          // failed to fill buffer, discard buffer
          free(buffer);
          buffer = NULL;
        }
      }
    }
  }      

  result = (rc_client_deserialize_progress(g_client, buffer) == RC_OK) ? 1 : 0;

  if (buffer)
    free(buffer);

  return result;
}


void rc_client_process_ra() {
  rc_client_do_frame(g_client);
}

void initialize_retroachievements_client(void)
{
  g_client = rc_client_create(read_memory, server_call);

  rc_client_set_event_handler(g_client, event_handler);

  if (Config::hardcoreMode == 1)
    rc_client_set_hardcore_enabled(g_client, 1);
  else 
    rc_client_set_hardcore_enabled(g_client, 0);
}

void shutdown_retroachievements_client(void)
{
  if (g_client)
  {
    // Release resources associated to the client instance
    rc_client_destroy(g_client);
    g_client = NULL;
  }  
}

static void login_callback(int result, const char* error_message, rc_client_t* client, void* userdata)
{
  // No Response, retry
  if (result == -32) {
    if (Config::RetroAchievementsUsername[0] != '\0' || Config::RetroAchievementsUsername[0] != '/') {
      g_notification.Show("Login failed: no response. Retrying..");
      rc_client_begin_login_with_token(g_client, Config::RetroAchievementsUsername, Config::RetroAchievementsToken, login_callback, NULL);
      return;
    }
    return;
  }
  if (result != RC_OK) {
    g_notification.Show("Login failed: %s", error_message);
    g_login_successful = false;
    return;
  }

  const rc_client_user_t* user = rc_client_get_user_info(client);
  int avatarTexture = -1;
  int texWidth = 0, texHeight = 0;

  store_retroachievements_credentials(user->username, user->token);

  if (user->avatar_url) {
    avatarTexture = DownloadAndPackAvatar(user->avatar_url,  &texWidth, &texHeight);

    if(Config::hardcoreMode == 1)
      g_notification.ShowWithIcon(avatarTexture, texWidth, texHeight, "Welcome %s (%u points)\nHardcore mode enabled", user->display_name, user->score);
    else
      g_notification.ShowWithIcon(avatarTexture, texWidth, texHeight, "Welcome %s (%u points)\nHardcore NOT enabled", user->display_name, user->score);

    return;
  }
  g_notification.Show("Welcome %s (%u points)", user->display_name, user->score);
}

/* -------------- STARTING GAME SESSION --------------*/

static void show_game_placard(void)
{
  char url[128];
  // async_image_data* image_data = NULL;
  const rc_client_game_t* game = rc_client_get_game_info(g_client);
  rc_client_user_game_summary_t summary;
  rc_client_get_user_game_summary(g_client, &summary);

  // Construct a message indicating the number of achievements unlocked by the user.
  if (summary.num_core_achievements == 0)
  {
    g_notification.Show("%s\nNo achievements available for this game.", game->title);
  }
  else if (summary.num_unsupported_achievements)
  {
    g_notification.Show("%s\nYou have %u of %u achievements unlocked.\nSome achievements are not supported.", game->title, summary.num_unlocked_achievements, summary.num_core_achievements);
  }

  // The emulator is responsible for managing images. This uses rc_client to build
  // the URL where the image should be downloaded from.

  if (rc_client_game_get_image_url(game, url, sizeof(url)) == RC_OK)
  {
    int width, height;
    int textureId = DownloadAndPackAvatar(url, &width, &height);
    
    if (textureId >= 0) {
      g_notification.ShowWithIcon(textureId, width, height, "%s\nYou have %u of %u achievements unlocked.\n", game->title, summary.num_unlocked_achievements, summary.num_core_achievements);
    } else {
      g_notification.Show("%s\nYou have %u of %u achievements unlocked.", game->title, summary.num_unlocked_achievements, summary.num_core_achievements);
    }
  }
  else {
    g_notification.Show("%s\nYou have %u of %u achievements unlocked.", game->title, summary.num_unlocked_achievements, summary.num_core_achievements);
  }

}

static void load_game_callback(int result, const char* error_message, rc_client_t* client, void* userdata)
{
  if (result != RC_OK)
  {
    g_notification.Show("RetroAchievements game load failed: %s", error_message);
    return;
  }

  show_game_placard();

  //g_achievements = achievements_list();
}

void load_game_from_file(const char* path)
{
  rc_client_begin_identify_and_load_game(g_client, RC_CONSOLE_NINTENDO_DS, path, NULL, 0, load_game_callback, NULL);
}


/* -------------- INITIALIZATION --------------*/

void InitRetroAchievements(const char* username, const char* password, bool isToken) {
  if (!IsNetworkAvailable())
    return;
    
  initialize_retroachievements_client();

  if(isToken) rc_client_begin_login_with_token(g_client, username, password, login_callback, NULL);
  else rc_client_begin_login_with_password(g_client, username, password, login_callback, NULL);

}

bool isConnected() {
  if(g_client) {
    return true;
  }
  return false;
}

void resetRCClient() {
  rc_client_reset(g_client);
}

void rc_client_paused() {
  rc_client_idle(g_client);
}