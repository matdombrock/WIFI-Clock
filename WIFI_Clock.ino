#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <MatrixGL.h>
#include "DHT.h"
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
/*
* SETUP LED MATRIX
*/
#define CLK_PIN   18  // or SCK//13
#define DATA_PIN  23  // or MOSI//11
#define CS_PIN 5 // or SS//10
#define MAX_DEVICES 4
#define LENGTHX 32
#define LENGTHY 8
MatrixGL matrix(CLK_PIN,DATA_PIN,CS_PIN,MAX_DEVICES);

/*
* SETUP DHT (TEMP/HUMIDITY SENSOR)
*/
#define DHTPIN 17     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
/*
* SETUP GLOBALS
*/
// float lastHum = 0;
// float lastTemp = 0;
// int mode = 0; // mode
// int modeB = 0; // border mode
// int si = 0;
// int sb = 0; // global border iterator
// int localTime[4];
// int lastMin = -1;
// String storedText = "ABC123";
// int lenX = matrix.lenX()-1;
// int lenY = matrix.lenY()-1;
// bool buttonState = 0;

class State {
  public:
    float lastHum = 0;
    float lastTemp = 0;
    int mode = 0; // mode
    int modeB = 0; // border mode
    int si = 0;
    int sb = 0; // global border iterator
    int localTime[4];
    int lastMin;
    String storedText = "ABC123";
    int lenX = matrix.lenX()-1;
    int lenY = matrix.lenY()-1;
    bool buttonState = 0;
}st;
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
* TIME FUNCTIONS
*/
void updateTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  int thour = timeinfo.tm_hour;
  int tmin  = timeinfo.tm_min;
  int h1 = 0;
  int h2 = 0;
  int m1 = 0;
  int m2 = 0;
  // 24hr time
  if(thour>9){
    h1 = 1;
    h2 = thour - 10;
  }else{
    h2 = thour;
  }
  if(thour>19){
    h1 = 2;
    h2 = thour - 20;
  }
  if(tmin > 9){
    m1 = 1;
    m2 = tmin - 10;
  }else{
    m2 = tmin;
  }
  if(tmin > 19){
    m1 = 2;
    m2 = tmin - 20;
  }
  if(tmin > 29){
    m1 = 3;
    m2 = tmin - 30;
  }
  if(tmin > 39){
    m1 = 4;
    m2 = tmin - 40;
  }
  if(tmin > 49){
    m1 = 5;
    m2 = tmin - 50;
  }
  st.localTime[0] = h1;
  st.localTime[1] = h2;
  st.localTime[2] = m1;
  st.localTime[3] = m2;
  #if DEBUG == true
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println(thour);
  Serial.println(tmin);
  Serial.println(h1);
  Serial.println(h2);
  Serial.println(m1);
  Serial.println(m2);
  #endif
}
/*
* DHT FUNCTIONS
*/
void updateDHT(){
  delay(1000);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  #if DEBUG == true
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
  #endif
  if(st.lastHum != h && st.lastTemp != t){
    st.lastHum = h;
    st.lastTemp = t;
  }
}
/*
* MATRIX DRAW FUNCTIONS
*/
void demoRandomNoise(){
  // Generate random noise
  int X = matrix.lenX();
  int Y = matrix.lenY();
  matrix.lock();
  matrix.clear();
  for(int i = 0; i<(X*Y); i++){
    matrix.drawPoint(random(X), random(Y)); 
  }
  matrix.unlock();
  matrix.delayF();
}
void drawClock(){
  // Skip if time has not changed
  if(st.localTime[3] != st.lastMin || true){
    matrix.lock();
    matrix.clear();
    matrix.drawNum(st.localTime[0],5,1);
    matrix.drawNum(st.localTime[1],10,1);
    matrix.drawChar(':',14,1);
    matrix.drawNum(st.localTime[2],18,1);
    matrix.drawNum(st.localTime[3],23,1);  
    matrix.unlock(); 
  }
  st.lastMin = st.localTime[3];
}
void println(String str){
  char buf[6];
  str.toCharArray(buf, 7);
  matrix.lock();
  matrix.clear();
  for(int i = 0; i<6; i++){
    if(buf[i]==NULL){
      buf[i] = ' ';
    }
    matrix.drawChar(buf[i],1+(i*5),1);  
  }
  matrix.unlock();
}
void say(){
  println(st.storedText);
}
void drawDHT(){
  updateDHT();
  String DHTString = String(st.lastTemp).substring(0,2) + "C" + String(st.lastHum).substring(0,2) + "H";
  println(DHTString);
}
/*
* MATRIX BORDER FUNCTIONS
*/
void border(){
  if(st.sb > 300){
    matrix.drawLine(0,0, 0,st.lenY);
  }else if(st.sb > 200){
    matrix.drawLine(st.lenX,0, 0,0);
  }else if(st.sb > 100){
    matrix.drawLine(st.lenX,st.lenY, st.lenX,0);
  }else{
    matrix.drawLine(0,st.lenY, st.lenX,st.lenY);
  }
  st.sb+=10;
  if(st.sb>400){
    st.sb=0;
  }
}
void border2()
{
  if(st.sb>31){
    //reverse
    int sx = st.sb - 31;
    matrix.drawPoint(0,sx/4);
    matrix.drawPoint(st.lenX,st.lenY-sx/4);
    matrix.drawPoint(sx,0);
    matrix.drawPoint(st.lenX-sx,st.lenY);
  }else{
    matrix.drawPoint(0,st.lenY-st.sb/4);
    matrix.drawPoint(st.lenX,st.sb/4);
    matrix.drawPoint(st.lenX-st.sb,0);
    matrix.drawPoint(st.sb,st.lenY);
  }
  st.sb++;
  if(st.sb>63){
    st.sb=0;
  }
}
void border3(){
  matrix.drawPoint(random(st.lenX),0);
  matrix.drawPoint(random(st.lenX),st.lenY);
  matrix.drawPoint(0,random(st.lenY));
  matrix.drawPoint(st.lenX,random(st.lenY));
  matrix.drawPoint(random(st.lenX),0);
  matrix.drawPoint(random(st.lenX),st.lenY);
  matrix.drawPoint(0,random(st.lenY));
  matrix.drawPoint(st.lenX,random(st.lenY));
  matrix.drawPoint(random(st.lenX),0);
  matrix.drawPoint(random(st.lenX),st.lenY);
  matrix.drawPoint(0,random(st.lenY));
  matrix.drawPoint(st.lenX,random(st.lenY));
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
    runMode();
    delay(500);
  }
}
/*
* RUN CURRENT MODE
*/
void runMode(){
   switch(st.mode){
    case 0:
      drawClock();
      break;
    case 1:
      demoRandomNoise();
      break;
    case 2:
      say();
      break;
    case 3:
      drawDHT();
      break;
    default:
      drawClock();
      break;
   }
}
/*
* RUN CURRENT BORDER
*/
void runBorder(){
  switch (st.modeB){
    case 0:
      break;
    case 1:
      border();
      break;
    case 2:
      border2();
      break;
    case 3:
      border3();
      break;
    default:
      break;
  }
}

void setup() {
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
  updateTime();
  runMode();
  runBorder();
  matrix.delayF();
}
