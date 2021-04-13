#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
//Custom
#include "State.h"
#include "setupMatrix.h"
#include "borders.h"
#include "modes.h"
#include "updateTime.h"
/*
* DEBUG
*/
#define DEBUG false
/*
* SETUP WIFI/SERVER
*/
const char* ssid = "HERSHEL";
const char* password = "meowmeow";
WebServer server(80);
/*
* SETUP TIME CONSTANTS
*/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800;
const int   daylightOffset_sec = 3600;

State st;

/*
* SETUP SERVER FUNCTIONS
*/
void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(404, "text/plain", message);
}
void handleCmd(){
  String cmd = server.argName(0);
  String argv = server.arg(0);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", cmd +" : "+ argv);
  Serial.println(cmd +" : "+ argv);
  if(cmd == "s"){
    if(argv == "clock"){
      Serial.println("MODE: CLOCK");
      st.mode = 0;
      // Reset lastMin so clock updates instantly
      st.lastMin = -1;
    }
    if(argv == "demo"){
      Serial.println("MODE: DEMO");
      st.mode = 1;
    }
    if(argv == "dht"){
      Serial.println("MODE: DHT");
      st.mode = 3;
    }
  }
  if(cmd == "intensity"){
    matrix.setIntensity(argv.toInt());
  }
  if(cmd == "say"){
    Serial.println("MODE: SAY");
    st.storedText = argv;
    Serial.print("Set saved text to:");
    Serial.println(st.storedText);
    st.mode = 2;
  }
  if(cmd == "border"){
    if(argv == "none"){
      Serial.println("BORDER: NONE");
      st.modeB = 0;
    }
    if(argv == "1"){
      Serial.println("BORDER: 1");
      st.modeB = 1;
    }
    if(argv == "2"){
      Serial.println("BORDER: 2");
      st.modeB = 2;
    }
    if(argv == "3"){
      Serial.println("BORDER: 3");
      st.modeB = 3;
    }
  }
  Serial.println(st.mode);
}

/*
* CHANGE MODE ON BUTTON
*/
void checkBtn(){
  st.buttonState = digitalRead(15);
  if(st.buttonState){
    Serial.println("BTN CHANGE MODE");
    st.mode++;
    if(st.mode>3){
      st.mode = 0;
    }
    runMode(matrix, st);
    delay(500);
  }
}

void setup() {
  st.lenX = matrix.lenX()-1;
  st.lenY =  matrix.lenY()-1;

  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  // Routes
  server.on("/", handleRoot);

  server.on("/cmd", handleCmd);

  server.onNotFound(handleNotFound);

  // Finish Server Setup
  server.begin();
  Serial.println("HTTP server started");
  
  // Setup Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //Setup DHT
  dht.begin();

  //Setup btn
  pinMode(15, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  checkBtn();
  updateTime(st);
  runMode(matrix, st);
  runBorder(matrix, st);
  matrix.delayF();
}
