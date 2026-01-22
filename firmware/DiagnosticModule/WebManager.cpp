#include "WebManager.h"
#include "config.h"
#include "secrets.h"

WiFiServer server(80);
awot::Application app;

String statusText = "idle";
bool pending = false;
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

const char DASHBOARD_HTML[] PROGMEM=
R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title> Diagnostic Module Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; }
  </style>
</head>
<body>
  <h1>Diagnostic Module Dashboard</h1>
  <div id="status">LOADING</div>

  <script>
    async function refresh() {
      const response = await fetch("/status");
      const json = await response.json();
      document.getElementById("status").innerText = json.state;
    }
    setInterval(refresh, 1000);
  </script>
</body>
</html>
)rawliteral";

void init_webapp(){
  //app routes here
  app.get("/", get_dashboard);
  app.get("/status", upload_result);

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

  Serial.print("[WiFi] Connected! IP Address: ");
  Serial.println(WiFi.localIP());
//END CODE FOR WIFI INIT
}


String stateJson(){
  //return json string with the status 
  return String("blah");
}

void get_dashboard(awot::Request &req, awot::Response &res){
  res.set("Content-Type", "text/html");
  res.print(DASHBOARD_HTML);
}

void upload_result(awot::Request &req, awot::Response &res){
  res.set("Content-Type", "text/html");
  res.print(DASHBOARD_HTML);
}

void serve_http(){
  //Serve HTTP LOGIC REPEATEDLY
  WiFiClient client = server.available();
  if (client) {
    client.setTimeout(1000);
    app.process(&client); 
    client.stop();
  }
//END Http logic
}
