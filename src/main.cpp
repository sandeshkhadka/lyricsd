#include <iostream>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <unistd.h>
#include <vector>

static const char *MPRIS_PREFIX = "org.mpris.MediaPlayer2";
static const char *MPRIS_OBJECT_PATH = "/org/mpris/MediaPlayer2";
static const char *MPRIS_PLAYER_IFACE = "org.mpris.MediaPlayer2.Player";

int main() {
  auto session = sdbus::createSessionBusConnection();
  auto dbus_proxy =
      sdbus::createProxy(*session, sdbus::ServiceName{"org.freedesktop.DBus"},
                         sdbus::ObjectPath{"/org/freedesktop/DBus"});

  std::vector<std::string> list;
  dbus_proxy->callMethod("ListNames")
      .onInterface("org.freedesktop.DBus")
      .storeResultsTo(list);

  std::vector<std::string> active_clients;
  for (auto &s : list) {
    if (s.find(MPRIS_PREFIX) != std::string::npos) {
      active_clients.push_back(s);
    }
  }

  for (auto &s : active_clients) {
    auto player_proxy =
        sdbus::createProxy(*session, sdbus::ServiceName{s},
                           sdbus::ObjectPath{"/org/mpris/MediaPlayer2"});

    sdbus::Variant meta;
    player_proxy->callMethod("Get")
        .onInterface("org.freedesktop.DBus.Properties")
        .withArguments("org.mpris.MediaPlayer2.Player", "Metadata")
        .storeResultsTo(meta);

    auto metadata = meta.get<std::map<std::string, sdbus::Variant>>();

    std::cout << metadata["xesam:title"].get<std::string>() << "\n";
  }

  return 0;
}
