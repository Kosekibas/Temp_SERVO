// Todo add static ip
// todo add time server
// todo add button
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ServoSmooth.h>

ServoSmooth servo;
uint32_t tmr;

const char* ssid = "TP-Link_B5D2";  // SSID
const char* password = "75979409"; // пароль
const uint16_t min_us=500;
const uint16_t max_us=2500;
const int open=0;
const int close=90;
const int speed=120;
const int acel=0.2;

ESP8266WebServer server(80);

uint8_t food_pin = D4;
bool food_status = HIGH;
bool food_pre_status=LOW;

enum State {
  SERVER,
  MOTOR,
};
State work_state;

String SendHTML(uint8_t food_stat)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Food for Cat </h1>\n";
  
  if(food_stat)
    ptr +="<p>Food Status: Open</p><a class=\"button button-off\" href=\"/food_off\">Close</a>\n";
  else
    ptr +="<p>Food Status: Close</p><a class=\"button button-on\" href=\"/food_on\">Open</a>\n";

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void handle_OnConnect() 
{
  Serial.print("Food status: ");
  Serial.print(food_status);
  Serial.println("");
  server.send(200, "text/html", SendHTML(food_status)); 
}

void handle_food_on() 
{
  food_status = HIGH;
  Serial.println("Food Status: Open");
  server.send(200, "text/html", SendHTML(true)); 
  work_state=MOTOR;
  tmr = millis();
}

void handle_food_off() 
{
  food_status = LOW;
  Serial.println("Food Status: Close");
  server.send(200, "text/html", SendHTML(false)); 
  work_state=MOTOR;
  tmr = millis();
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}



void setup() 
{
  analogWriteResolution(16);  //увеличиваем разрядность ШИМ до 16 бит
  Serial.begin(115200);
  delay(100);
  Serial.println("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.on("/food_on", handle_food_on);
  server.on("/food_off", handle_food_off);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");

  servo.attach(food_pin,min_us,max_us);        // подключить
  // servo.smoothStart();
  servo.setSpeed(speed);    // ограничить скорость
  servo.setAccel(acel);   	  // установить ускорение (разгон и торможение)
   work_state=MOTOR;
  tmr = millis();
}

void loop() 
{
  switch(work_state){
    case SERVER:
      server.handleClient();
      break;
    case MOTOR:
      servo.tick();
      if (food_status!= food_pre_status){
          food_pre_status= food_status;
          servo.setTargetDeg(food_status ? open: close);  
          Serial.println("gOO");
      }
      // TODO zamena na return tick
      if (millis() - tmr >= 5000) {   //  3 сек
        tmr = millis();
        work_state=SERVER;
        Serial.println("to Server");
      }     
      break;
  }
  
    

 }

