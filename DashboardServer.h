#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <functional>

class DashboardServer {
 public:
  DashboardServer();

  void begin();
  void loop();

  void broadcastJson(const String& payload);
  void setMessageHandler(std::function<void(const String&)> handler);

 private:
  void handleRoot();
  void onWsEvent(uint8_t clientNum, WStype_t type, uint8_t* payload, size_t length);

  WebServer http_;
  WebSocketsServer ws_;
  std::function<void(const String&)> handler_;
};
