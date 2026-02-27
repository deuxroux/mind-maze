#include "WebManager.h"
#include "config.h"
#include "secrets.h"

WiFiServer server(80);
awot::Application app;
static StaticJsonDocument<512> inbox; //message for this module-- a one message inbox. 
static bool inboxHasMessage = false;
bool inboxUnread = false;
bool mazeAwaiting = false;
static uint32_t mazeSeedCounter = 0; //every new maze request increments this counter. 

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;



const char DASHBOARD_HTML[] PROGMEM=
R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title> Monitoring Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; }
  </style>
</head>
<body>
  <h1>Monitoring Dashboard</h1>
<div>
  <h2>Actions</h2>
  <button id="requestMazeBtn">Send New Maze</button>
  <button id="ackBtn">Acknowledge Result</button>
  <div id="requestMazeStatus"></div>
</div>

<div>
  <h2>Status</h2>
  <div id="mazeStatus">No Maze Issued</div>
  <div id ="instruction">Assign Maze in Actions section above. </div>
</div>

<div>
  <h2>Debug</h2>
  <a href="/api/send">Send test JSON to patient device</a><br>
  <a href="/api/pull">Read this module's inbox</a><br>
  <a href="/api/ack"> Clear this module's inbox</a><br>
  <a href="/api/request_maze"> Issue new maze</a><br>
  <a href="/api/push"> Receive maze result</a>
</div>

</body>
<script>
  const btn = document.getElementById("requestMazeBtn");
  const ackBtn = document.getElementById("ackBtn");
  const statusEl = document.getElementById("requestMazeStatus");
  const state = document.getElementById("mazeStatus");
  const instruction = document.getElementById("instruction")

  btn.addEventListener("click", async () => {
    statusEl.textContent = "Sending request...";
    try {
      const res = await fetch("/api/request_maze", { method: "POST" });
      const data = await res.json();
      if(data.ok){
        statusEl.textContent = "Request Sent. Maze Active";
        instruction.textContent = "Maze Assignment Sent. Wait for Results";
      }else{
        statusEl.textContent = "Request failed";
      }
      
    } catch (e) {
      statusEl.textContent = "Request failed";
    }
  });

  ackBtn.addEventListener("click", async () => {
    try {
      const res = await fetch("/api/ack");
      const data = await res.json();
      if (data.ok) {
        instruction.textContent = "Inbox cleared.";
        statusEl.innerText="";
        if (!data.unread) {
          state.innerText = "No Result Yet";
        }
      }
    } catch (e) {
      instruction.textContent = "Ack failed";
    }
  });

  async function refresh() {
    const res = await fetch("/api/pull");
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const json = await res.json();

    if(json.message){
      state.innerText = JSON.stringify(json.message, null, 2);
      statusEl.innerText = "result received!";
      instruction.innerText = "Acknowledge with pushbutton or in actions to clear and send new Maze.";
    } else{
      state.innerText = "No Result Yet"
    }
  }
  setInterval(refresh, 5000);
</script>
</html>
)rawliteral";

//HELPERS
void json_reply(awot::Response &res, int code, const JsonDocument &doc) {
  res.status(code);                              // HTTP status
  res.set("Content-Type", "application/json");   // heder
  serializeJson(doc, res);                       //serialize and send
}

//web and route management

void init_webapp(){
  //app routes
  app.get("/", get_dashboard);
  
  // P2P routes
  app.post("/api/push", push_message);
  app.get("/api/pull", pull_message);
  app.get("/api/send", send_test); //test route-- can comment out later. 
  app.get("/api/ack", ack_message);
  app.post("/api/request_maze", request_new_maze); //button route to request new maze.


  server.begin();
  Serial.println("[HTTP] Listening on :80");
}

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
  Serial.print("[WiFi] Monitoring Module Connected! IP Address: ");
  Serial.println(WiFi.localIP());
//END CODE FOR WIFI INIT
}


void serve_http(){
  //Serve HTTP LOGIC REPEATEDLY
  WiFiClient client = server.available();
  if (client) {
    client.setTimeout(1500);
    app.process(&client); 
    client.stop();
  }
//END Http logic
}

//p2p route functions
void push_message(awot::Request &req, awot::Response &res){
  Serial.println("[PUSH] handler hit for monitoring mod");

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
  inboxUnread = true;

  reply["ok"] = true;
  reply["storedBytes"] = measureJson(inbox); //check length of message for safety checks

  //send json
  Serial.println(inboxHasMessage ? "[PUSH] stored" : "[PUSH] not stored");
  json_reply(res, 200, reply);
}

void pull_message(awot::Request &req, awot::Response &res){
  (void) req;
  // Return inbox and clear it

  StaticJsonDocument<1024> reply;
  reply["ok"] = true;
  reply["empty"] = !inboxHasMessage;
  reply["unread"] = inboxUnread;

  if(!inboxHasMessage){
    reply["message"] = nullptr;
  }else {
    JsonVariant m = reply.createNestedObject("message");
    m.set(inbox.as<JsonVariant>());
  }
  Serial.println(!inboxHasMessage ? "[PULL] empty inbox" : "[PULL] message in inbox");

  json_reply(res, 200, reply);
}

void ack_message(awot::Request &req, awot::Response &res) {
  (void)req;

  clear_inbox_state();

  StaticJsonDocument<256> reply;
  reply["ok"] = true;
  reply["empty"] = true;
  reply["unread"] = inboxUnread;
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
  c.stop();

  //return true if there is a connection
  bool ok200 = statusLine.startsWith("HTTP/1.1 200");

  return ok200;

}

//route to just test functionality
void send_test(awot::Request &req, awot::Response &res){
  (void)req; //may need to silence compiler...

  StaticJsonDocument<256> msg;

  //meta data for message test
  msg["type"] = "test";
  msg["from"] = WiFi.localIP().toString();
  msg["millis"] = (unsigned long)millis(); // track time
  msg["note"] = "hello diag maze module this is physician monitoring mod";

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

void request_new_maze(awot::Request &req, awot::Response &res){
  (void)req;

  StaticJsonDocument<256> msg;
  msg["type"] = "request_maze";
  msg["from"] = WiFi.localIP().toString();
  msg["millis"] = (unsigned long)millis();

  uint32_t seed = ++mazeSeedCounter; //increment seed counter-- use temporary variable for json object. 
  if (seed == 0) seed = 1; //wrap around to 1 if overflow-- saw in Adafruit but i don't think this will ever happen. 
  msg["maze_seed"] = seed;

  String body;
  serializeJson(msg, body);

  IPAddress peer(PEER_IP);
  bool ok = post_json_to_peer(peer, 80, "/api/push", body);
  mazeAwaiting = ok;

  StaticJsonDocument<256> reply;
  reply["ok"] = ok;
  reply["sentBytes"] = body.length();
  reply["peer"] = peer.toString();

  json_reply(res, ok ? 200:500, reply);
}

bool pull_local_message(JsonDocument &outMsg, bool &outEmpty) {
  outMsg.clear();

  if (!inboxHasMessage) {
    outEmpty = true;
    inboxUnread = true;
    return true;
  }
  outMsg.set(inbox);

  outEmpty = false;
  return true;
}

void clear_inbox_state() {
  inbox.clear();
  inboxHasMessage = false;
  inboxUnread = false; //mark read
}
