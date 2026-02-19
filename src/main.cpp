#include "lyrics.hpp"
#include "mpris.hpp"
#include <condition_variable>
#include <iostream>
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

  client.RegisterOnTrackChanged([&] {
    {
      std::lock_guard<std::mutex> g(mtx);
      trackChanged = true;
    }
    cv.notify_one();
  });

  std::cout << lyrics_client.GetLyrics(client.GetTrackInfo());

  // all callback registration should be done before entering main loop
  // the client's callback vector is not thread safe
  client.EnterMainLoop();

  std::unique_lock<std::mutex> lock(mtx);
  while (running) {
    cv.wait(lock, [&] {
      return trackChanged;
    });
    trackChanged = false;
    lock.unlock();
    // do some processing for changed state
    std::cout << lyrics_client.GetLyrics(client.GetTrackInfo());
    lock.lock();
  }

  return 0;
}
