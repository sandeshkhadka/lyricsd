#include "lyrics-dbus-service.hpp"
#include "lyrics-emitter.hpp"
#include "lyrics.hpp"
#include "mpris.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/sdbus-c++.h>
#include <unistd.h>

int main() {
  std::mutex mtx;
  std::condition_variable cv;
  bool trackChanged = false;
  bool running = true;

  MprisClient client;
  Lyrics lyrics_client;
  LyricsDbusService dbus_service;

  std::unique_ptr<LyricsEmmiter> emitter;

  // Helper: stop old emitter, fetch lyrics, start new emitter
  auto startEmitter = [&] {
    if (emitter) {
      emitter->Stop();
      emitter.reset();
    }

    TrackInfo info = client.GetTrackInfo();
    std::string lyrics = lyrics_client.GetLyrics(info);
    if (lyrics.empty()) {
      return;
    }

    int64_t position_us = client.GetPosition();
    emitter = std::make_unique<LyricsEmmiter>(lyrics, info, position_us);
    emitter->RegisterOnLineChanged([&](const std::string& line) {
      dbus_service.SetCurrentLine(line);
    });
    emitter->Start();
  };

  client.RegisterOnTrackChanged([&] {
    {
      std::lock_guard<std::mutex> g(mtx);
      trackChanged = true;
    }
    cv.notify_one();
  });

  client.RegisterOnSeeked([&](int64_t position_us) {
    if (emitter) {
      emitter->Seek(position_us);
    }
  });

  client.RegisterOnPlaybackStatus([&](const std::string& status) {
    if (!emitter)
      return;
    if (status == "Paused") {
      emitter->Pause();
    } else if (status == "Playing") {
      emitter->Resume();
    }
  });

  startEmitter();

  // All callback registration should be done before entering main loop
  // the client's callback vector is not thread safe
  client.EnterMainLoop();

  std::unique_lock<std::mutex> lock(mtx);
  while (running) {
    cv.wait(lock, [&] {
      return trackChanged;
    });
    trackChanged = false;
    lock.unlock();
    startEmitter();
    lock.lock();
  }

  return 0;
}
