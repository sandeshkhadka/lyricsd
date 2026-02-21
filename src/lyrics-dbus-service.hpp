#ifndef LYRICS_DBUS_SERVICE_H
#define LYRICS_DBUS_SERVICE_H

#include <memory>
#include <mutex>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IObject.h>
#include <string>

class LyricsDbusService {
private:
  static constexpr const char* SERVICE_NAME = "org.lyricsd.Lyrics";
  static constexpr const char* OBJECT_PATH = "/org/lyricsd/Lyrics";
  static constexpr const char* INTERFACE_NAME = "org.lyricsd.Lyrics";

  std::unique_ptr<sdbus::IConnection> m_connection;
  std::unique_ptr<sdbus::IObject> m_object;

  std::string m_current_line;
  std::mutex m_mutex;

public:
  LyricsDbusService();
  ~LyricsDbusService();

  void SetCurrentLine(const std::string& line);
  std::string GetCurrentLine();
};

#endif
