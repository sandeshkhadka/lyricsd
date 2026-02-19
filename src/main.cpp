#include "mpris.hpp"
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/sdbus-c++.h>
#include <unistd.h>

int main() {
  MprisClient client;
  client.EnterMainLoop();
  return 0;
}
