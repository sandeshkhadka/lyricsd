#include "lyrics.hpp"
#include "simplejson.hpp"
#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <iostream>
#include <string>

std::string Lyrics::REL_CHACHE_DIR = ".cache/lyricsd";
std::string Lyrics::LYRICS_API = "https://lrclib.net/api/get";

static size_t
WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
  size_t totalSize = size * nmemb;
  userp->append(static_cast<char*>(contents), totalSize);
  return totalSize;
}

std::string Lyrics::MakeRequest(const std::string& url) {
  CURL* curl = curl_easy_init();
  if (!curl)
    return "";

  std::string response;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    std::cerr << "curl error: " << curl_easy_strerror(res) << std::endl;
    response.clear();
  }

  long httpCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

  if (httpCode != 200) {
    response.clear();
  }

  curl_easy_cleanup(curl);

  return response;
}

void Lyrics::Init() {
  char* home = std::getenv("HOME");
  if (home == nullptr) {
    std::cerr << "Failed to get user's home directory \n";
    exit(1);
  }
  m_cache_dir_path =
    std::filesystem::path(std::string(home) + "/" + REL_CHACHE_DIR);
  if (!std::filesystem::exists(m_cache_dir_path)) {
    try {

      std::cout << "Cache directory not found.\n Creating cache dir now.\n";
      std::filesystem::create_directory(m_cache_dir_path);
      std::cout << "Cache directory created: " << m_cache_dir_path << "\n";
    } catch (std::filesystem::filesystem_error e) {
      std::cerr << "Failed to create cache dir: " << e.what() << "\n";
    }
  }
}

Lyrics::Lyrics() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  Init();
}
Lyrics::~Lyrics() {
  curl_global_cleanup();
}

std::string Lyrics::UrlEncode(const std::string& str) {
  CURL* curl = curl_easy_init();
  if (!curl)
    return str;

  char* output =
    curl_easy_escape(curl, str.c_str(), static_cast<int>(str.length()));
  std::string result = output ? output : str;
  curl_free(output);
  curl_easy_cleanup(curl);

  return result;
}

std::string Lyrics::BuildUrl(const TrackInfo& track_info) {
  std::string url = Lyrics::LYRICS_API;
  std::string track_name = UrlEncode(track_info.title);
  std::string artist = UrlEncode(track_info.artist);
  std::string album = UrlEncode(track_info.album);

  url += "?track_name=" + track_name;
  if (artist.size() > 0) {
    url += "&artist_name=" + artist;
  }
  if (album.size() > 0) {
    url += "&album_name=" + album;
  }
  std::chrono::microseconds us(track_info.duration_us);
  std::chrono::duration<double> seconds =
    std::chrono::duration_cast<std::chrono::duration<double>>(us);
  url += "&duration=" + std::to_string(seconds.count());

  return url;
}

std::string Lyrics::GetLyrics(const TrackInfo& track_info) {
  if (track_info == m_trackinfo) {
    return cached_lyrics;
  }
  std::string url = BuildUrl(track_info);
  std::string response = MakeRequest(url);
  std::string syncedLyrics = ParseSyncedLyrics(response);

  m_trackinfo = track_info;
  cached_lyrics = syncedLyrics;
  return syncedLyrics;
}

std::string Lyrics::ParseSyncedLyrics(const std::string& json) {
  return ExtractJsonString(json, "syncedLyrics");
}
