#ifndef LYRICS_EMITTER_H
#define LYRICS_EMITTER_H

#include "mpris.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

struct LyricLine {
  int64_t timestamp_ms; // Milliseconds from start
  std::string text;
};

using OnLineChangedCallback = std::function<void(const std::string&)>;

class LyricsEmmiter {
private:
  std::string m_lyrics;
  TrackInfo m_trackinfo;
  std::vector<LyricLine> m_parsed_lyrics;

  size_t m_current_index = 0;
  std::atomic<bool> m_running{false};
  bool m_seeked = false;
  int64_t m_seek_position_ms = 0;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::thread m_thread;

  int64_t m_start_position_ms = 0;

  std::vector<OnLineChangedCallback> m_line_callbacks;

  static std::vector<LyricLine> Parse(const std::string& lrcContent);
  static std::optional<int64_t> ParseTimestamp(const std::string& timestamp);
  size_t FindCurrentIndex(int64_t position_ms);

  void EmitLoop();
  void EmitLine(const std::string& text);

public:
  LyricsEmmiter(const std::string& lyrics,
                const TrackInfo& trackinfo,
                int64_t start_position_us = 0);
  ~LyricsEmmiter();

  void Start();
  void Stop();
  void Seek(int64_t position_us);

  void RegisterOnLineChanged(OnLineChangedCallback cb);
};

#endif
