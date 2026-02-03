#include "WebManager.h"
#include "config.h"
#include "secrets.h"

WiFiServer server(80);
awot::Application app;
static StaticJsonDocument<512> inbox; //message for this module
static bool inboxHasMessage = false;


char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

const char DASHBOARD_HTML[] PROGMEM=
R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title> Patient's Diagnostic Maze Device Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; }
  </style>
</head>
<body>
  <h1>Patient's Diagnostic Maze Device Dashboard</h1>
  <div id="status">LOADING</div>

  <div>
  <h2>API endpoints</h2>
  <a href="/api/send">Send JSON (result) to physician monitoring module</a>
  <br>
  <a href="/api/pull">Receive JSON (maze) from physician monitoring module</a>
  </div>
</body>
</html>
)rawliteral";

//HELPERS
void json_reply(awot::Response &res, int code, const JsonDocument &doc) {
  res.status(code);                              // HTTP status
  res.set("Content-Type", "application/json");   // heder
  serializeJson(doc, res);                       //serialize and send
}

//web init and routes

void init_wifi(){
  //CODE FOR WIFI INIT
  WiFi.disconnect();

  IPAddress localIP(NODE_IP);
  IPAddress dns(DNS_IP);
  IPAddress gateway(GATEWAY_IP);
  IPAddress subnet(SUBNET_IP);

  WiFi.config(localIP, dns, gateway, subnet);

  WiFi.begin(ssid, pass);
  delay(1000);
  //check if module exists
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("[ERROR] WiFiS3 module not detected.");
  }
  Serial.print("[WiFi] Attempting connection to SSID: ");
  Serial.println(ssid);
  //Wait for connection...
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println("Waiting to connect...");
  }

  Serial.print("[WiFi] Diagnostic MAZE Module Connected! IP Address: ");
  Serial.println(WiFi.localIP());
//END CODE FOR WIFI INIT
}

void init_webapp(){
  //app routes
  app.get("/", get_dashboard);

  // P2P routes
  app.post("/api/push", push_message);
  app.get("/api/pull", pull_message);
  app.get("/api/send", send_test); //test route-- can comment out later. 

  server.begin();
  Serial.println("[HTTP] Listening on :80");
}

void serve_http(){
  //Serve HTTP LOGIC REPEATEDLY
  WiFiClient client = server.available();

  if(!client) return;
 
  client.setTimeout(1500);
  app.process(&client); 
  client.stop();

//END Http logic
}

//p2p route functions
void push_message(awot::Request &req, awot::Response &res){
  Serial.println("[PUSH] handler hit for maze mod");

  //clear inbox just in case not cleared already
  inbox.clear();

  // Receive JSON and store to notifs inbox
  StaticJsonDocument<1024> reply;

  //error checking w built in deserializer-- log only if exsits. 
  //this also saves to inbox
  DeserializationError err = deserializeJson(inbox, req);

  if (err) {
    inboxHasMessage= false; 
    reply["ok"] = false;
    reply["error"] = "BAD_JSON";
    reply["detail"] = err.c_str();
    json_reply(res, 400, reply);
    return; //break and do not continue
  }

  inboxHasMessage = true;
  reply["ok"] = true;
  reply["storedBytes"] = measureJson(inbox); //check length of message for safety checks

  //send json
  json_reply(res, 200, reply);
}

void pull_message(awot::Request &req, awot::Response &res){
  // Return inbox and clear it
  Serial.println("[PULL] handler hit for maze mod");

  StaticJsonDocument<1024> reply;
  reply["ok"] = true;

  //if empty, return. otherwise read the message. 
  if (!inboxHasMessage) {
    reply["empty"] = true;
    reply["message"] = nullptr;
    json_reply(res,200, reply);
    return;
  }
  reply["empty"] = false;

  JsonObject msg = reply.createNestedObject("message"); //create message object in the reply object to read
  msg.set(inbox.as<JsonObjectConst>()); //set the message here as the inbox contents. 

  inbox.clear(); //clear inbox
  inboxHasMessage = false;
  Serial.println(inboxHasMessage ? "[PULL] had message" : "[PULL] empty");
  json_reply(res, 200, reply);
}


void get_dashboard(awot::Request &req, awot::Response &res){
  res.set("Content-Type", "text/html");
  res.printP(DASHBOARD_HTML);
}

bool post_json_to_peer(const IPAddress &peer, uint16_t port, const char *path, const String &body){
  WiFiClient c; //new client to connect to peer device
  if (!c.connect(peer, port)) return false;

  //create outbount message with the details
  c.print("POST ");
  c.print(path);
  c.print(" HTTP/1.1\r\nHost: ");
  c.print(peer);
  c.print("\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: ");
  c.print(body.length());
  c.print("\r\n\r\n"); //escape chars for readability
  c.print(body);

  //get time and wait until timeout. afterwards fail gracefully if no client connection
  uint32_t start = millis();
  while (!c.available() && (millis() - start) < 3000) { delay(1); }

  if (!c.available()) {
    c.stop();
    return false;
  }

  String statusLine = c.readStringUntil('\n');  //go until escape char to get status for tcp buffer
  statusLine.trim(); //need to trim r char to keep http compliant
  Serial.print("Peer status: ");
  Serial.println(statusLine);
  c.stop();

  //return true if there is a connection
  return statusLine.startsWith("HTTP/1.1 200");
}


void send_test(awot::Request &req, awot::Response &res){
  (void)req; //may need to silence compiler...

  StaticJsonDocument<256> msg;

  //meta data for message test
  msg["type"] = "test";
  msg["from"] = WiFi.localIP().toString();
  msg["millis"] = (unsigned long)millis(); // track time
  msg["note"] = "hello physician monitoring module this is diag maze mod";

  String body;
  serializeJson(msg, body);

  IPAddress peer(PEER_IP); //constructor for the ip address of peer node
  bool ok = post_json_to_peer(peer, 80, "/api/push", body); //confirm send

  StaticJsonDocument<256> reply; //reply acknowledges send to the requestor
  reply["ok"] = ok; 
  reply["sentBytes"] = body.length();
  reply["peer"] = peer.toString();

  json_reply(res, ok ? 200:500, reply); //if ok was bad, send a bad reply via 500 status code
}
