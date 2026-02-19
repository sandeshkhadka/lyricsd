#ifndef MPRISCLIENT_H
#define MPRISCLIENT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <string>
#include <vector>

using OnTrackChangedCallback = std::function<void()>;

struct TrackInfo {
  std::string title;
  std::string artist;
  std::string album;
  int64_t duration_us; // Microseconds
  int64_t m_seeked_pos;

  bool IsValid() const {
    return !title.empty();
  }

  void Clear() {
    title.clear();
    artist.clear();
    album.clear();
    duration_us = 0;
    m_seeked_pos = 0;
  }

  bool operator==(const TrackInfo& other) const {
    return title == other.title && artist == other.artist &&
           album == other.album && duration_us == other.duration_us;
  }

  bool operator!=(const TrackInfo& other) const {
    return !(*this == other);
  }
};

class MprisClient {
private:
  static std::string DBUS_SERVICE_NAME;
  static std::string DBUS_OBJECT_PATH;
  static std::string DBUS_PROPS_IFACE;
  static std::string MPRIS_SERVICE_PREFIX;
  static std::string MPRIS_OBJECT_PATH;
  static std::string MPRIS_PLAYER_IFACE;

  std::unique_ptr<sdbus::IProxy> m_currentPlayerProxy;
  std::string m_currentPlayerName;
  std::unique_ptr<sdbus::IConnection> m_session;
  std::unique_ptr<sdbus::IProxy> m_dbus_proxy;
  std::vector<OnTrackChangedCallback> m_callbacks;

  void RunTrackChangedCallbacks();

  TrackInfo m_trackInfo;

  std::vector<std::string> SearchAvailablePlayers();
  void FindAndWatchPlayer();
  void OnPropertiesChanged(const std::string& iface,
                           const std::map<std::string, sdbus::Variant>& changed,
                           const std::vector<std::string>& invalidated);

  static void PrintMetadata(const sdbus::Variant&);

  void WatchPlayer(const std::string& name);
  void InvalidateCurrentPlayer();

  TrackInfo
  ParseMetadata(const std::map<std::string, sdbus::Variant>& metadata);

  void OnNameOwnerChange(const std::string& name,
                         const std::string& oldOwner,
                         const std::string& newOwner);

  void PrintCurrentTrackInfo();

public:
  MprisClient();
  void RegisterOnTrackChanged(OnTrackChangedCallback);
  void EnterMainLoop();
  TrackInfo GetTrackInfo();
};

#endif
