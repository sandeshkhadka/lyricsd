#include "lyrics-dbus-service.hpp"
#include <iostream>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IObject.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/VTableItems.h>
#include <sdbus-c++/sdbus-c++.h>

LyricsDbusService::LyricsDbusService() {
  try {
    m_connection =
      sdbus::createSessionBusConnection(sdbus::ServiceName{SERVICE_NAME});

    m_object =
      sdbus::createObject(*m_connection, sdbus::ObjectPath{OBJECT_PATH});

    m_object
      ->addVTable(
        sdbus::registerSignal("LyricsLineChanged")
          .withParameters<std::string>("line"),
        sdbus::registerProperty("CurrentLine")
          .withGetter([this]() -> std::string {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_current_line;
          })
          .withUpdateBehavior(
            sdbus::Flags::PropertyUpdateBehaviorFlags::EMITS_CHANGE_SIGNAL))
      .forInterface(sdbus::InterfaceName{INTERFACE_NAME});

    m_connection->enterEventLoopAsync();

    std::cout << "D-Bus service registered: " << SERVICE_NAME << "\n";
  } catch (sdbus::Error& e) {
    std::cerr << "Failed to create D-Bus service: " << e.getMessage() << "\n";
  }
}

LyricsDbusService::~LyricsDbusService() {
  if (m_object) {
    m_object->unregister();
  }
}

void LyricsDbusService::SetCurrentLine(const std::string& line) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_current_line = line;
  }

  if (m_object) {
    try {
      m_object->emitSignal("LyricsLineChanged")
        .onInterface(INTERFACE_NAME)
        .withArguments(line);

      m_object->emitPropertiesChangedSignal(
        sdbus::InterfaceName{INTERFACE_NAME},
        {sdbus::PropertyName{"CurrentLine"}});
    } catch (sdbus::Error& e) {
      std::cerr << "Failed to emit signal: " << e.getMessage() << "\n";
    }
  }
}

std::string LyricsDbusService::GetCurrentLine() {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_current_line;
}
