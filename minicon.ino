#include <iostream>
#include <string>
using namespace std;
#include <ESP32Servo.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//モニタのサイズの設定
#define SCREEN_WIDTH    (128)
#define SCREEN_HEIGHT   (64)
#define SCREEN_ADDRESS  (0x3C)
// ディスプレイの宣言
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

const char ssid[] = "ESP_minicon_bag-WiFi";
const char pass[] = "esp32apwifi";
const IPAddress ip(192, 168, 30, 3);
const IPAddress subnet(255, 255, 255, 0);
WiFiServer server(80);

Servo servo_l;
Servo servo_r;
int vel_l = 0;
int vel_r = 0;
int vel_off = 90;
int vel_scale = 6;
int loopcount = 0;
int blink_start=2000;
int face_update=1;
unsigned long time_old;
float ratio_gaze=0;
String now_status = "init";

String html =
  "<!DOCTYPE html>\
<html lang='ja'>\
\
<head>\
  <meta charset='UTF-8'>\
    <style>\
      input {margin:8px;width:25%;font-size: 100%; padding:10px 10px;}\
      div {font-size:60pt;color:orange;text-align:center;width:90%;border:double 40px orange;}\
    </style>\
  <title>minicon_bag Controller</title>\
</head>\
\
<body>\
  <div>\
  <p>minicon_bag Controller</p>\
  <p>move</p>\
  <form method='get'>\
    <input type='submit' name='fr' value='△' /><br>\
    <input type='submit' name='le' value='◁' />\
    <input type='submit' name='st' value='◯' />\
    <input type='submit' name='ri' value='▷' /><br>\
    <input type='submit' name='ba' value='▽' /><br><br>\
  </form>\
  <p>speed= %SPEED% </p>\
  <form method='get'>\
    <input type='submit' name='ac' value='△' /><br>\
    <input type='submit' name='dc' value='▽' /><br><br>\
  </form>\
  </div>\
</body>\
</html>";


void setup_wifi();
void setup_display();
void setup_display();
void html_listen();
void servo_move();
void draw_face();

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setup_display();
  servo_l.attach(26);
  servo_r.attach(5);
}

void loop() {
  html_listen();
  servo_move();
  draw_face();
  loopcount = (loopcount + 1) % 10000;
}

void setup_wifi() {
  WiFi.softAP(ssid, pass);
  delay(100);
  WiFi.softAPConfig(ip, ip, subnet);
  IPAddress myIP = WiFi.softAPIP();
  delay(10);
  server.begin();
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println("Server start!");
}

void setup_display() {
  //Setup SSD1306
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 can not allocate memory!"));
    return;
  }
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.println("Hello\n minibag");
  display.display();
  delay(1000);
}

void html_listen() {
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    Serial.println("New Client.");

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            String buf = html;
            String buf_vel_scale = String(vel_scale);
            buf.replace("%SPEED%", buf_vel_scale);
            client.print(buf);
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        if (currentLine.endsWith("GET /?fr")) {
          now_status = "fr";
          vel_l = vel_scale+2;
          vel_r = -vel_scale;
        }
        if (currentLine.endsWith("GET /?le")) {
          now_status = "le";
          vel_l = -vel_scale+2;
          vel_r = -vel_scale+2;
        }
        if (currentLine.endsWith("GET /?ri")) {
          now_status = "ri";
          vel_l = vel_scale-2;
          vel_r = vel_scale-2;
        }
        if (currentLine.endsWith("GET /?ba")) {
          now_status = "ba";
          vel_l = -vel_scale;
          vel_r = vel_scale+2;
        }
        if (currentLine.endsWith("GET /?st")) {
          now_status = "st";
          vel_l = 0;
          vel_r = 0;
        }
        if (currentLine.endsWith("GET /?ac")) {
          now_status = "ac";
          vel_scale += 1;
        }
        if (currentLine.endsWith("GET /?dc")) {
          now_status = "dc";
          vel_scale -= 1;
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void servo_move() {
  servo_l.write(vel_off + vel_l);
  servo_r.write(vel_off + vel_r);
}

void draw_eyes() {

  int x=40;
  int w=16;
  int h1=20;
  int h2=16;
  int cy=40;
  int cx=64;
  //int blink_start=2000;
  int blink_turn=blink_start+200;
  int blink_end=blink_start+400;
  float ratio=0;
  int gaze=300;
  int dx=0;
  float eps=0.1;

  if(now_status=="ri"){
    ratio_gaze+=eps;
  }
  else if(now_status=="le"){
    ratio_gaze-=eps;
  }
  else if(ratio_gaze>0){
    ratio_gaze-=eps;
  }
  else if(ratio_gaze<0){
    ratio_gaze+=eps;
  }
  if(ratio_gaze>1){
    ratio_gaze=1;
  }
  if(ratio_gaze<-1){
    ratio_gaze=-1;
  }
  Serial.print("ratio_gaze=");
  Serial.println(ratio_gaze);
  dx=(int)(-8*ratio_gaze);
  
  int now_time=(int)(millis()-time_old);
  if(now_time > blink_start && now_time < blink_turn ){
     ratio=(float)(now_time-blink_start)/(blink_turn-blink_start);
     h1=(int)((float)h1*(1.0-ratio));
     h2=(int)((float)h2*(1.0-ratio));
  }
  else if(now_time > blink_turn){
    ratio=(float)(now_time-blink_turn)/(blink_end-blink_turn);
    h1=(int)((float)h1*ratio);
    h2=(int)((float)h2*ratio);
  }
  if(now_time > blink_end){
    time_old=millis();
    blink_start=random(20, 40)*100;
  }

  //Serial.print("h1=");Serial.print(h1);Serial.print(" h2=");Serial.println(h2);
  //Serial.print("ratio=");Serial.print(ratio);Serial.print(" now_time=");Serial.println(now_time);
  //目2
  display.fillRoundRect(cx-x-w/2+dx, cy-h1,  w, h1+h2,  4,  1);  //1
  display.fillRoundRect(cx+x-w/2+dx, cy-h1,  w, h1+h2,  4,  1);  //1
  //まつげ
  display.fillRoundRect( 8, cy-h1-8,  32, 4,  1,  WHITE);  //1
  display.fillRoundRect(88, cy-h1-8,  32, 4,  1,  WHITE);
  //ハイライト2
  //display.fillTriangle( cx-x+w/2,  max(cy-2,cy-h1),   cx-x+w/2,  min(cy+3,cy+h2),  cx-x,  cy, 0);
  //display.fillTriangle( cx+x+w/2,  max(cy-2,cy-h1),   cx+x+w/2,  min(cy+3,cy+h2),  cx+x,  cy, 0);
}

void draw_face() {
  //display_size=128,64
  face_update=1;
  if (face_update) {
    
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(58, 50);
    
    display.clearDisplay();
    draw_eyes();
    //まつげ
    //display.fillRoundRect( 8, 8,  32, 4,  1,  WHITE);  //1
    //display.fillRoundRect(88, 8,  32, 4,  1,  WHITE);  //1
    //口
    //display.fillTriangle( 64 - 6,  50,  64 + 6,  50,  64,  60, 2);
    //display.println("U");
    display.display();
    face_update=0;
  }
}
