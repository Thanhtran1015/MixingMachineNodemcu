#include <LiquidCrystal_I2C.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureAxTLS.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureAxTLS.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>

//#include <LiquidCrystal_I2C.h>
//#define ESP32
#ifdef ESP8266 || ESP32
#define ISR_PREFIX ICACHE_RAM_ATTR
#else
#define ISR_PREFIX
#endif
#include <SocketIOClient.h>
#include <ArduinoJson.h>
//#include <WiFi.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//#define PIN_DO 14
LiquidCrystal_I2C lcd(0x27, 20, 4);
SocketIOClient client;
const char* ssid     = "SHC-INT";
const char* password = "20182019";

const byte proximitySensor = 12;
const byte dc5vRelay = 13;


char host[] = "10.4.4.92";
int port = 3484;

extern String RID;
extern String Rname;
extern String Rcontent;

long durationMinute;
long durationSecond;
long d;

long rssi = WiFi.RSSI();
unsigned long previousMillis = 0;
long interval = 5000;
unsigned long startMilli;


volatile unsigned int pulses; //Biến lưu giá trị thời điểm trước đó bắt đầu tính vat can.
// volatile có nghĩa là ra lệnh cho trình biên dịch biết rằng giá trị của biến có thể bị thay đổi bởi một lời gọi hàm ngầm định nào đó
long rpm;//Biến lưu giá trị tốc độ tính được: số vòng trên phút.
unsigned long timeOld;

void ISR_PREFIX counter()
{
  pulses++;
}

void setup() {
  Serial.begin(9600);
  pinMode(proximitySensor, INPUT_PULLUP);
  pinMode(dc5vRelay, OUTPUT);
  pulses = 0;
  timeOld = 0;

  lcd.begin();       //Khởi động màn hình. Bắt đầu cho phép Arduino sử dụng màn hình
  lcd.backlight();   //Bật đèn nền
  lcd.clear();
  attachInterrupt(digitalPinToInterrupt(proximitySensor), counter, FALLING); //Ra lệnh khởi động interrupt. Hàm attachInterrupt
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    long rssi = WiFi.RSSI();
    Serial.print("RSSI:");
    Serial.println(rssi);
    delay(1000);
    lcd.setCursor(0, 0);
    lcd.print("Connecting Wifi ");
    lcd.setCursor(0, 1);
    lcd.print("RSSI: ");
    lcd.print(rssi);

  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wifi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  // ArduinoOTA
  // .onStart([]() {
  //   String type;
  //   if (ArduinoOTA.getCommand() == U_FLASH)
  //     type = "sketch";
  //   else // U_SPIFFS
  //     type = "filesystem";

  //   // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  //   Serial.println("Start updating " + type);
  // })
  // .onEnd([]() {
  //   Serial.println("\nEnd");
  // })
  // .onProgress([](unsigned int progress, unsigned int total) {
  //   Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  // })
  // .onError([](ota_error_t error) {
  //   Serial.printf("Error[%u]: ", error);
  //   if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
  //   else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
  //   else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
  //   else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  //   else if (error == OTA_END_ERROR) Serial.println("End Failed");
  // });

  // ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    delay(500);
    ESP.restart();
  }
  if (client.connected())
  {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Server Connected");
    lcd.setCursor(0, 1);
    lcd.print("               ");
    client.send("connection", "message", "Connected !");
    delay(2000);
    lcd.clear();
  }

}

void loop() {

 


  
  ArduinoOTA.handle();
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis > interval)
    {
      previousMillis = currentMillis;
      //client.heartbeat(0);
      client.send("atime", "message", "Time please?");
    }
    if (millis() - timeOld >= 1000)//Mỗi một giây, tốc độ được thực hiện tính toán.
    {
      detachInterrupt(digitalPinToInterrupt(proximitySensor));// tat ngat

      rpm = pulses * 60; //Tính toán giá trị tốc độ dựa vào thời gian và số lan đếm được.
      //send server
      if (rpm != 0) {
        long newMilli = millis();
        durationMinute = (newMilli - startMilli) / 1000 / 60;
        durationSecond = (newMilli - startMilli) / 1000 % 60;
        d = (newMilli - startMilli) / 1000 ;
        if (durationMinute < 10) {
          Serial.print("0");
        }
        Serial.print(durationMinute);
        Serial.print(":");
        if (durationSecond < 10) {
          Serial.print("0");
        }
        Serial.print(durationSecond);

        String JSON;
        StaticJsonDocument<200> doc;
        doc["I"] = "EM001";
        doc["R"] = rpm;
        doc["D"] = d;

        serializeJson(doc, Serial);
        Serial.println();
        serializeJson(doc, JSON);

        client.sendJSON("JSON", JSON);
        JSON = "";

      }
      else if (rpm == 0) {
        startMilli = millis();
        durationMinute = 0;
        durationSecond = 0;
      }
      lcd.setCursor(0, 0);
      lcd.print("Mixing Machine");
      lcd.setCursor(0, 1);
      lcd.print("Operation Time:");
      lcd.setCursor(0, 2);
      if (durationMinute < 10) {
        lcd.print("0");
      }
      lcd.print(durationMinute);

      lcd.print(":");

      if (durationSecond < 10) {
        lcd.print("0");
      }
      lcd.print(durationSecond);
      lcd.setCursor(0, 3);
      lcd.print("RPM : ");
      lcd.print(rpm);
      lcd.print("           ");

      timeOld = millis();
      pulses = 0;

      attachInterrupt(digitalPinToInterrupt(proximitySensor), counter, FALLING);// Khởi động lại interrupt.

    }

    if (client.monitor())
    {
      if (RID == "atime" && Rname == "time")
      {
        Serial.print("Il est ");
        Serial.println(Rcontent);
        client.send("revatime", "time", Rcontent);
      }


      if (RID == "alarm" && Rname == "status" )
      {
        if(Rcontent == "ON")
        {
         digitalWrite(dc5vRelay, HIGH);   // bật alarm
         Serial.print(Rcontent);
 
  
        }
        else if (Rcontent == "OFF")
        {
          digitalWrite(dc5vRelay, LOW);    // tắt alarm
          Serial.print(Rcontent);
        }
        
      }
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Mat ket noi wifi");
      delay(500);
      ESP.restart();
    }
    //Kết nối lại!
    if (!client.connected()) {
      Serial.print("Mat ket noi server");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Disconnected Server!");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting... ");
      delay(500);
      client.reconnect(host, port);
      lcd.clear();
      //ESP.restart();
    }
  
}
