#ifndef WEBMANAGER_H
#define WEBMANAGER_H
#include<Arduino.h>
#include <aWOT.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>

extern const char DASHBOARD_HTML[] PROGMEM;
extern bool inboxUnread;

void init_webapp();
void init_wifi();
void serve_http();

//declared routes
void get_dashboard(awot::Request &req, awot::Response &res);
void send_test(awot::Request &req, awot::Response &res);

// P2P Cxs --  & memory refs for efficiency
//debug routes
void push_message(awot::Request &req, awot::Response &res);  //  /api/push post debug route
void pull_message(awot::Request &req, awot::Response &res);  //  /api/pull get debug route
void ack_message(awot::Request &req, awot::Response &res);
void request_new_maze(awot::Request &req, awot::Response &res);

//helpers for peer to peer interpretation and json encoding
bool post_json_to_peer(const IPAddress &peer, uint16_t port, const char *path, const String &body);
bool pull_local_message(JsonDocument &outMsg, bool &outEmpty);

#endif