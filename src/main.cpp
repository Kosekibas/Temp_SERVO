// Todo 1 add static ip
// Todo set sp time in html https://arduino-tex.ru/news/72/esp826-uroki-http-zapros-post.html
  // https://kotyara12.ru/iot/esp-http/
// Todo add setpoint time variable to flash
// +todo add time server
// +todo 2 add button
//? todo 3 create func for open and close. Need info
//! todo soft move to start position
// todo after food go sleep 
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ServoSmooth.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

ServoSmooth servo_2;
uint32_t tmr;

const char* ssid = "TP-Link_B5D2";  // SSID
const char* password = "75979409"; // пароль
const uint16_t min_us=500;
const uint16_t max_us=2500;
const int open=0;
const int close=90;
const int speed=120;
const int acel=0.2;

const long utcOffsetInSeconds = 10800;
// bool wait_open;
int hour_open = 5;
int min_open=20;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

ESP8266WebServer server(80);

struct feeder{
  uint8_t but_pin;
  uint8_t food_pin;
  bool open = false;
  bool pre_status = true;
  bool flag = false;
  ServoSmooth servo;
};

feeder feeder1={D0, D1};
feeder feeder2={D3, D4};
feeder feeder3={D5, D6};

bool work_to_timer=true;

enum Status{
  OPEN,
  CLOSE
};

Status food_status = CLOSE;

enum State {
  SERVER,
  MOTOR,
};

State work_state;

void get_food_status (){
  if (!feeder1.open || !feeder2.open || !feeder3.open){
    food_status=CLOSE;
  }
  else food_status=OPEN;
}

void open_all_feeder(){
  feeder1.open=true;
  feeder2.open=true;
  feeder3.open=true;
  get_food_status();
  work_state=MOTOR;
}

void close_all_feeder(){
  feeder1.open=false;
  feeder2.open=false;
  feeder3.open=false;
  get_food_status();
  work_state=MOTOR;
}

String SendHTML(Status food_stat)
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>FOOD</title>\n";
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
  
  if(food_stat == OPEN)
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
  open_all_feeder();
  Serial.println("Food Status: Open");
  server.send(200, "text/html", SendHTML(OPEN)); 

}

void handle_food_off() 
{
  close_all_feeder();
  Serial.println("Food Status: Close");
  server.send(200, "text/html", SendHTML(CLOSE)); 
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
  timeClient.begin();
  server.on("/", handle_OnConnect);
  server.on("/food_on", handle_food_on);
  server.on("/food_off", handle_food_off);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");

  feeder1.servo.attach(feeder1.food_pin,min_us,max_us,1000);
  delay(1000);
  feeder1.servo.setSpeed(speed);    
  feeder1.servo.setAccel(acel);   	  
  pinMode(feeder1.but_pin, INPUT_PULLUP);

  feeder2.servo.attach(feeder2.food_pin,min_us,max_us,1000);
  feeder2.servo.setSpeed(speed);    
  feeder2.servo.setAccel(acel);   	
  pinMode(feeder2.but_pin, INPUT_PULLUP);

  feeder3.servo.attach(feeder3.food_pin,min_us,max_us,1000);
  feeder3.servo.setSpeed(speed);    
  feeder3.servo.setAccel(acel);   	
  pinMode(feeder3.but_pin, INPUT_PULLUP);

  tmr = millis();
  work_state=MOTOR;
  // wait_open=true;
}


bool button_press(feeder& f){
  bool btnState = !digitalRead(f.but_pin);
  if (btnState && !f.flag) {  // обработчик нажатия
    f.flag = true;
    Serial.println("press");
    f.open =! f.open ;
    return true;
  }
  if (!btnState && f.flag) {  // обработчик отпускания
    f.flag = false;  
  }
  return false;
}

bool need_open(feeder& f){
  if(timeClient.getHours()==hour_open && timeClient.getMinutes()==min_open){
    f.open=true;
    return true;
  }
  return false;
}

bool work_servo(feeder& f){
  if (f.open!= f.pre_status){
      Serial.println(f.open);
      Serial.println(f.pre_status);
      f.pre_status = f.open;
      f.servo.setTargetDeg(f.open ? open: close);  
      Serial.println("Work servo");

  }
  if (f.servo.tick()) return true;
  return false;
}

void loop() 
{
  get_food_status();
  // bool servo_finish=false;
  switch(work_state){
    case SERVER:
      if (button_press(feeder1) || button_press(feeder2) || button_press(feeder3)){
        work_state=MOTOR;
        break;
      }
      server.handleClient();
      if (millis() - tmr >= 15000) {   // каждые 15 сек
        tmr = millis();
        timeClient.update();
        Serial.print(timeClient.getHours());
        Serial.print(":");
        Serial.println(timeClient.getMinutes());
      }
      if (work_to_timer){
        if(timeClient.getHours()==hour_open && timeClient.getMinutes()==min_open){
          open_all_feeder();
          work_to_timer=false;
        }
      }
      break;
    case MOTOR:
        if (work_servo(feeder1) && work_servo(feeder2) && work_servo(feeder3)){
          work_state=SERVER;
          Serial.println("end Work Servo");
        }

      break;
  }
 }

