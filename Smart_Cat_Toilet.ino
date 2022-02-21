/*
  smart cat toilet for M5Stick C Plus
  item
  M5Stick C Plus
  WEIGHT UNIT
  Load Cell 32kg https://ja.aliexpress.com/item/32860005302.html

  library
  HX711 library https://github.com/RobTillaart/HX711

  Special Thanks @hack_tnr @kohacraft_blog
*/

#include "HX711.h"
#include <M5StickCPlus.h>

#include <WiFi.h>
#include <stdlib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <WiFiClientSecure.h>
#include "certificate.h" // IFTTT
String makerEvent = "****"; // Maker Webhooks
String makerKey = "************"; // Maker Webhooks

WiFiClient client;

#define FRONT 1

#define X_LOCAL 40
#define Y_LOCAL 40
#define X_F 30
#define Y_F 30

// 32kg 100.5 | 5kg 420.0983 | 20kg 127.15
#define cal 100.8

// Wi-FiのSSID
const char *ssid = "*********";
// Wi-Fiのパスワード
const char *password = "************";

// GoogleスプレッドシートのデプロイされたURLを設定
const char* published_url = "https://script.google.com/macros/s/**************/exec";

const int capacity = JSON_OBJECT_SIZE(2);
StaticJsonDocument<capacity> json_request;
char buffer[255];

HX711 scale;

uint8_t dataPin = 33;
uint8_t clockPin = 32;

uint32_t start, stop;
volatile float f;

int notZero = 0;
float oldWeight = 0.00;
boolean flg = false;

void setup() {
  M5.begin();
  scale.begin(dataPin, clockPin);

  WiFi.begin(ssid, password); //  Wi-Fi APに接続
  while (WiFi.status() != WL_CONNECTED)
  { //  Wi-Fi AP接続待ち
    delay(500);
    Serial.print(".");
  }
  Serial.print("WiFi connected\r\nIP address: ");
  Serial.println(WiFi.localIP());

  // TODO find a nice solution for this calibration..
  // load cell factor 32 KG
  scale.set_scale(cal);

  M5.Lcd.setRotation(1);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(2, 2, FRONT);
  M5.Lcd.print("Connect Unit to GROVE B");

  M5.Lcd.setCursor(2, 21, FRONT);
  M5.Lcd.print("Btn_A to reset");
  M5.Lcd.setTextSize(5);
  Serial.begin(115200);
  delay(1000);
  // reset the scale to zero = 0
  scale.tare();

  oldWeight = 1;

}

//GASにデータ送信
void sendGoogle(float weight, float wDiff) {
  StaticJsonDocument<500> doc;
  char pubMessage[256];

  JsonArray dataValues = doc.createNestedArray("value");
  dataValues.add(weight);

  JsonArray datawDiff = doc.createNestedArray("wdiff");
  datawDiff.add(wDiff);

  serializeJson(doc, pubMessage);

  // HTTP通信開始
  HTTPClient http;

  Serial.print(" HTTP通信開始　\n");
  http.begin(published_url);

  Serial.print(" HTTP通信POST　\n");
  int httpCode = http.POST(pubMessage);

  if (httpCode > 0) {
    Serial.print(" HTTP Response:");
    Serial.println(httpCode);

    if (httpCode == HTTP_CODE_OK) {
      Serial.println(" HTTP Success!!");
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    //      Serial.println(" FAILED");
    Serial.printf("　HTTP　failed,error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

}

//IFTTTにパラメータを送信
String sendToIFTTT(float value)
{
  const char* server = "maker.ifttt.com";  // Server URL
  String url = "/trigger/" + makerEvent + "/with/key/" + makerKey;
  url += "?value1=";
  url += value;

  HTTPClient httpClient;
  bool ok = httpClient.begin( server, 443, url, IFTTT_ca_cert);
  Serial.printf("httpClient.begin %d\n",ok);
  
  int httpCode = httpClient.GET();
  String httpResponse = httpClient.getString();
  httpClient.end();

  Serial.printf("httpCode:%d\n",httpCode);
  Serial.println(httpResponse);

  return httpResponse; 
}

void loop() {
  M5.update();
  if (M5.BtnA.wasReleased()) {
    scale.tare();
  }

  // continuous scale 4x per second
  f = scale.get_units(5);
  float weight = f / 1000;
  Serial.print(f);
  Serial.print(" ");
  Serial.print(weight);
  Serial.print(" ");
  Serial.println(oldWeight);

  //Serial.println(scale.averageValue());
  M5.Lcd.setCursor(2, 41, FRONT);
  M5.Lcd.printf("                             ");
  M5.Lcd.setCursor(2, 61, FRONT);
  M5.Lcd.printf("%0.2fkg     ", weight);

  // 10g以上20回で値を送信する
  if (weight >= oldWeight + 0.01 || weight <= oldWeight - 0.01) {
    notZero ++;
    Serial.println(notZero);
    if (notZero > 20) {
      float wDiff;
      wDiff = weight - oldWeight;
      if (wDiff >= 0.2) {
        sendGoogle(weight, wDiff);
        String response = sendToIFTTT(wDiff);
      }
      oldWeight = weight;
      notZero = 0;
    }
  }
  else notZero = 0;

}
