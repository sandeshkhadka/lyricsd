#ifndef LYRICS_H
#define LYRICS_H

#include "mpris.hpp"
#include <filesystem>
#include <string>
class Lyrics {
private:
  std::string m_filename;

  std::filesystem::path m_cache_dir_path;

  TrackInfo m_trackinfo;
  std::string cached_lyrics;

  void Init();
  bool HasCached(const TrackInfo&);
  std::string UrlEncode(const std::string&);
  std::string BuildUrl(const TrackInfo&);
  std::string MakeRequest(const std::string&);
  std::string ParseSyncedLyrics(const std::string&);

public:
  Lyrics();
  ~Lyrics();

  std::string GetLyrics(const TrackInfo&);
  static std::string REL_CHACHE_DIR;
  static std::string LYRICS_API;
};

#endif
