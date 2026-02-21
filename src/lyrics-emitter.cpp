#include "lyrics-emitter.hpp"
#include "mpris.hpp"
#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>

LyricsEmmiter::LyricsEmmiter(const std::string& lyrics,
                             const TrackInfo& trackinfo,
                             int64_t start_position_us) {
  m_lyrics = lyrics;
  m_trackinfo = trackinfo;
  m_parsed_lyrics = Parse(lyrics);
  m_start_position_ms = start_position_us / 1000;
}

LyricsEmmiter::~LyricsEmmiter() {
  Stop();
}

void LyricsEmmiter::RegisterOnLineChanged(OnLineChangedCallback cb) {
  m_line_callbacks.push_back(cb);
}

void LyricsEmmiter::EmitLine(const std::string& text) {

  std::cout << text << std::endl;

  for (auto& cb : m_line_callbacks) {
    cb(text);
  }
}

void LyricsEmmiter::Start() {
  if (m_parsed_lyrics.empty()) {
    std::cerr << "No parsed lyrics to emit.\n";
    return;
  }

  m_running = true;
  m_thread = std::thread(&LyricsEmmiter::EmitLoop, this);
}

void LyricsEmmiter::Stop() {
  if (!m_running)
    return;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = false;
  }
  m_cv.notify_one();

  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void LyricsEmmiter::Seek(int64_t position_us) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_seeked = true;
    m_seek_position_ms = position_us / 1000;
  }
  m_cv.notify_one();
}

void LyricsEmmiter::Pause() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_paused = true;
  }
  m_cv.notify_one();
}

void LyricsEmmiter::Resume() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_paused = false;
  }
  m_cv.notify_one();
}

void LyricsEmmiter::EmitLoop() {
  // Anchor: wall-clock time corresponding to playback_position_ms
  auto wall_anchor = std::chrono::steady_clock::now();
  int64_t playback_anchor_ms = m_start_position_ms;

  m_current_index = FindCurrentIndex(playback_anchor_ms);

  while (m_running && m_current_index < m_parsed_lyrics.size()) {
    int64_t line_ts = m_parsed_lyrics[m_current_index].timestamp_ms;
    auto target_time =
      wall_anchor + std::chrono::milliseconds(line_ts - playback_anchor_ms);

    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait_until(lock, target_time, [this] {
        return !m_running || m_seeked || m_paused;
      });

      if (!m_running) {
        break;
      }

      if (m_paused) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          now - wall_anchor);
        m_paused_playback_ms = playback_anchor_ms + elapsed.count();

        m_cv.wait(lock, [this] {
          return !m_running || !m_paused || m_seeked;
        });

        if (!m_running) {
          break;
        }

        // Re-anchor wall clock to the paused position
        wall_anchor = std::chrono::steady_clock::now();
        playback_anchor_ms = m_paused_playback_ms;

        if (m_seeked) {
          m_seeked = false;
          wall_anchor = std::chrono::steady_clock::now();
          playback_anchor_ms = m_seek_position_ms;
          m_current_index = FindCurrentIndex(playback_anchor_ms);
          std::cout << "\n--- Seeked ---\n";
        }
        continue;
      }

      if (m_seeked) {
        m_seeked = false;
        wall_anchor = std::chrono::steady_clock::now();
        playback_anchor_ms = m_seek_position_ms;
        m_current_index = FindCurrentIndex(playback_anchor_ms);
        std::cout << "\n--- Seeked ---\n";
        continue;
      }
    }

    EmitLine(m_parsed_lyrics[m_current_index].text);
    m_current_index++;
  }
}

std::vector<LyricLine> LyricsEmmiter::Parse(const std::string& lrcContent) {
  std::vector<LyricLine> lyrics;

  // Match lines like [00:12.34]Some lyric text
  // Also handles multiple timestamps: [00:12.34][00:45.67]Text
  // shamelessly asked ChatGPT for this regex
  std::regex lineRegex(R"(\[(\d{1,2}):(\d{2})\.(\d{2,3})\])");

  std::istringstream stream(lrcContent);
  std::string line;

  while (std::getline(stream, line)) {
    if (line.empty())
      continue;

    std::vector<int64_t> timestamps;
    std::string text;

    std::sregex_iterator iter(line.begin(), line.end(), lineRegex);
    std::sregex_iterator end;

    size_t lastMatchEnd = 0;

    for (; iter != end; ++iter) {
      const std::smatch& match = *iter;

      int minutes = std::stoi(match[1].str());
      int seconds = std::stoi(match[2].str());
      std::string msStr = match[3].str();

      // Handle both .xx and .xxx formats
      int milliseconds = std::stoi(msStr);
      if (msStr.length() == 2) {
        milliseconds *= 10;
      }

      int64_t timestamp_ms = (minutes * 60 + seconds) * 1000 + milliseconds;
      timestamps.push_back(timestamp_ms);

      lastMatchEnd = match.position() + match.length();
    }

    // Extract text after last timestamp
    if (lastMatchEnd > 0 && lastMatchEnd < line.length()) {
      text = line.substr(lastMatchEnd);

      // Trim whitespace
      size_t start = text.find_first_not_of(" \t\r\n");
      size_t end = text.find_last_not_of(" \t\r\n");
      if (start != std::string::npos) {
        text = text.substr(start, end - start + 1);
      } else {
        text.clear();
      }
    }

    if (timestamps.empty())
      continue;

    for (int64_t ts : timestamps) {
      lyrics.push_back({ts, text});
    }
  }
  std::sort(
    lyrics.begin(), lyrics.end(), [](const LyricLine& a, const LyricLine& b) {
      return a.timestamp_ms < b.timestamp_ms;
    });

  return lyrics;
}

size_t LyricsEmmiter::FindCurrentIndex(int64_t position_ms) {
  if (m_parsed_lyrics.empty())
    return 0;

  size_t left = 0;
  size_t right = m_parsed_lyrics.size();

  while (left < right) {
    size_t mid = left + (right - left) / 2;
    if (m_parsed_lyrics[mid].timestamp_ms <= position_ms) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  return left > 0 ? left - 1 : 0;
}
