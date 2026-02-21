#include "mpris.hpp"
#include <cstdint>
#include <iostream>
#include <sdbus-c++/Error.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <string>
#include <utility>
#include <vector>

std::string MprisClient::DBUS_SERVICE_NAME = "org.freedesktop.DBus";
std::string MprisClient::DBUS_OBJECT_PATH = "/org/freedesktop/DBus";
std::string MprisClient::DBUS_PROPS_IFACE = "org.freedesktop.DBus.Properties";
std::string MprisClient::MPRIS_SERVICE_PREFIX = "org.mpris.MediaPlayer2";
std::string MprisClient::MPRIS_OBJECT_PATH = "/org/mpris/MediaPlayer2";
std::string MprisClient::MPRIS_PLAYER_IFACE = "org.mpris.MediaPlayer2.Player";

TrackInfo MprisClient::GetTrackInfo() {
  return m_trackInfo;
}

void MprisClient::PrintCurrentTrackInfo() {
  std::string message;
  if (m_trackInfo.title.size() <= 0)
    return;
  message += m_trackInfo.title;
  if (m_trackInfo.artist.size() > 0) {
    message += " by '" + m_trackInfo.artist + "'";
  }
  if (m_trackInfo.artist.size() > 0) {
    message += " from Album: " + m_trackInfo.album;
  }
  message += "[ " + std::to_string(m_trackInfo.duration_us) + " ]";
  std::cout << message << std::endl;
}

TrackInfo MprisClient::ParseMetadata(
  const std::map<std::string, sdbus::Variant>& metadata) {
  TrackInfo info;
  auto getStr = [&](const char* key) -> std::string {
    auto it = metadata.find(key);
    if (it != metadata.end()) {
      try {
        return it->second.get<std::string>();
      } catch (...) {
        std::cerr << "Cannot extract " << it->first << "\n";
        std::cerr << "Correct Type is: " << it->second.peekValueType() << "\n";
      }
    }
    return "";
  };

  auto getStrArray = [&](const char* key) -> std::string {
    auto it = metadata.find(key);
    if (it != metadata.end()) {
      try {
        auto arr = it->second.get<std::vector<std::string>>();
        if (!arr.empty())
          return arr[0];
      } catch (...) {
        std::cerr << "Cannot extract " << it->first << "\n";
        std::cerr << "Correct Type is: " << it->second.peekValueType() << "\n";
      }
    }
    return "";
  };

  auto getInt = [&](const char* key) -> int64_t {
    auto it = metadata.find(key);
    if (it != metadata.end()) {
      try {
        return it->second.get<int64_t>();
      } catch (...) {
        std::cerr << "Cannot extract " << it->first << "\n";
        std::cerr << "Correct Type is: " << it->second.peekValueType() << "\n";
      }
    }
    return 0;
  };

  info.title = getStr("xesam:title");
  info.artist = getStrArray("xesam:artist");
  info.album = getStr("xesam:album");
  info.duration_us = getInt("mpris:length");

  return info;
}

MprisClient::MprisClient() {
  try {
    m_session = sdbus::createSessionBusConnection();
    m_dbus_proxy =
      sdbus::createProxy(*m_session,
                         sdbus::ServiceName{MprisClient::DBUS_SERVICE_NAME},
                         sdbus::ObjectPath{MprisClient::DBUS_OBJECT_PATH});

    std::cout << "Connected to session bus.\n";
    m_dbus_proxy->uponSignal("NameOwnerChanged")
      .onInterface(MprisClient::DBUS_SERVICE_NAME)
      .call([this](const std::string& name,
                   const std::string& oldOwner,
                   const std::string& newOwner) {
        OnNameOwnerChange(name, oldOwner, newOwner);
      });
  } catch (sdbus::Error e) {
    std::cerr << "Failed to create a session bus: " << e.getMessage()
              << std::endl;
    exit(1);
  }
  MprisClient::FindAndWatchPlayer();
}

void MprisClient::InvalidateCurrentPlayer() {
  m_currentPlayerName.clear();
  m_trackInfo.Clear();
  m_currentPlayerProxy = nullptr;
}

void MprisClient::OnNameOwnerChange(const std::string& name,
                                    const std::string& oldOwner,
                                    const std::string& newOwner) {
  if (name.rfind(MprisClient::MPRIS_SERVICE_PREFIX) != std::string::npos) {
    if (!newOwner.empty() && m_currentPlayerName.empty()) {
      WatchPlayer(name);
    } else if (!oldOwner.empty() && name == m_currentPlayerName) {
      InvalidateCurrentPlayer();
      MprisClient::FindAndWatchPlayer();
      RunTrackChangedCallbacks();
    }
  }
}

std::vector<std::string> MprisClient::SearchAvailablePlayers() {
  std::vector<std::string> all;
  std::vector<std::string> players;
  m_dbus_proxy->callMethod("ListNames")
    .onInterface(MprisClient::DBUS_SERVICE_NAME)
    .storeResultsTo(all);

  for (const auto& c : all) {
    if (c.find(MprisClient::MPRIS_SERVICE_PREFIX) != std::string::npos) {
      players.push_back(c);
    }
  }
  return players;
}

void MprisClient::WatchPlayer(const std::string& name) {
  if (name == m_currentPlayerName) {
    return;
  }
  auto proxy =
    sdbus::createProxy(*m_session,
                       sdbus::ServiceName{name},
                       sdbus::ObjectPath{MprisClient::MPRIS_OBJECT_PATH});

  proxy->uponSignal("PropertiesChanged")
    .onInterface(MprisClient::DBUS_PROPS_IFACE)
    .call([this](const std::string& iface,
                 const std::map<std::string, sdbus::Variant>& changed,
                 const std::vector<std::string>& invalidated) {
      OnPropertiesChanged(iface, changed, invalidated);
    });

  proxy->uponSignal("Seeked")
    .onInterface("org.mpris.MediaPlayer2.Player")
    .call([this](int64_t newPosition) {
      this->m_trackInfo.m_seeked_pos = newPosition;
      RunSeekedCallbacks(newPosition);
    });
  try {
    sdbus::Variant metadataVar;
    proxy->callMethod("Get")
      .onInterface(DBUS_PROPS_IFACE)
      .withArguments(MPRIS_PLAYER_IFACE, "Metadata")
      .storeResultsTo(metadataVar);

    auto metadata = metadataVar.get<std::map<std::string, sdbus::Variant>>();
    m_trackInfo = ParseMetadata(metadata);
  } catch (sdbus::Error& e) {
    std::cerr << "Falied to set intial trackinfo: " << e.getMessage() << "\n";
  }

  std::cout << "Watching Player: " << name << std::endl;
  if (m_trackInfo.IsValid()) {
    std::cout << "Playing Right Now: ";
    PrintCurrentTrackInfo();
  }

  m_currentPlayerProxy = std::move(proxy);
  m_currentPlayerName = name;
}

void MprisClient::FindAndWatchPlayer() {
  std::vector<std::string> playerNames = MprisClient::SearchAvailablePlayers();
  for (const auto& p : playerNames) {
    WatchPlayer(p);
    break;
  }
}

void MprisClient::OnPropertiesChanged(
  const std::string& iface,
  const std::map<std::string, sdbus::Variant>& changed,
  const std::vector<std::string>& invalidated) {
  for (auto& [k, v] : changed) {
    if (k == "Metadata") {
      m_trackInfo =
        ParseMetadata(v.get<std::map<std::string, sdbus::Variant>>());
      std::cout << "Changed Track. \n Now Playing: ";
      PrintCurrentTrackInfo();
      RunTrackChangedCallbacks();
    }
  }
}

void MprisClient::RunTrackChangedCallbacks() {
  for (auto& cb : m_callbacks) {
    cb();
  }
}

void MprisClient::RegisterOnTrackChanged(OnTrackChangedCallback cb) {
  m_callbacks.push_back(cb);
}

void MprisClient::RegisterOnSeeked(OnSeekedCallback cb) {
  m_seek_callbacks.push_back(cb);
}

void MprisClient::RunSeekedCallbacks(int64_t position_us) {
  for (auto& cb : m_seek_callbacks) {
    cb(position_us);
  }
}

int64_t MprisClient::GetPosition() {
  if (!m_currentPlayerProxy)
    return 0;
  try {
    sdbus::Variant posVar;
    m_currentPlayerProxy->callMethod("Get")
      .onInterface(DBUS_PROPS_IFACE)
      .withArguments(MPRIS_PLAYER_IFACE, "Position")
      .storeResultsTo(posVar);
    return posVar.get<int64_t>();
  } catch (sdbus::Error& e) {
    std::cerr << "Failed to get position: " << e.getMessage() << "\n";
    return 0;
  }
}

void MprisClient::EnterMainLoop() {
  m_session->enterEventLoopAsync();
}
