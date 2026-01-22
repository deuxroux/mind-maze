#ifndef WEBMANAGER_H
#define WEBMANAGER_H
#include<Arduino.h>
#include <aWOT.h>
#include <WiFiS3.h>

extern const char DASHBOARD_HTML[] PROGMEM;

String stateJson();
void init_webapp();
void init_wifi();

void serve_http();

//declare routes
void get_dashboard(awot::Request &req, awot::Response &res);
void upload_result(awot::Request &req, awot::Response &res);

#endif