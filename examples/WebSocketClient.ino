#include <ESP8266WiFi.h>
#include <ArduinoJson.h>  // Json库
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <WebSocketClient.h>


// 设备初始化的时候 AP信息
const IPAddress apIP(192, 168, 4, 1);
const char* apSSID = "ESP8266_SETUP";
const char* apPWD = "123456789";
boolean settingMode;
String ssidList;
String client_id; // ws客户端ID

char path[] = "/";
char host[] = "192.168.31.100";

DNSServer dnsServer;
ESP8266WebServer webServer(80);
WebSocketClient webSocketClient;
// Use WiFiClient class to create TCP connections
WiFiClient client;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);  // 初始化EERPOM缓存区大小
  delay(10);
  
  if (restoreConfig()) {
    Serial.println("Existing Wifi...");
    if (checkConnection()) {
      Serial.println("Existing Wifi ok!!!!");
      settingMode = false;
      startWebServer();
      return;
    } else {
      Serial.println("Existing Wifi No!!!!");
      settingMode = true;
      setupMode();
    }
  }
  delay(10);
}


void loop() {
  if (settingMode) {
    dnsServer.processNextRequest();
  }
  webServer.handleClient();
  
  String data;
  if (client.connected()) {
    webSocketClient.getData(data);
    if (data.length() > 0) {
      // 解析json
      const size_t bufferSize = JSON_ARRAY_SIZE(199) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + 1000;  // 设置数组缓冲区大小
      DynamicJsonDocument doc(bufferSize);  // 缓冲区大小
      deserializeJson(doc, data);
      JsonObject root = doc.as<JsonObject>();
      String type = root["type"];  // 类型
      if(type == "binding"){
        String aclient = root["client_id"];  // 获取设备ID
        client_id = aclient;  // 赋值
      }else{
        //获取内容
        String content = root["content"];  // 获取内容
        Serial.println(content);

        // 需要点亮LED或者其他操作 皆写在此处...
        
      }
    }else{
       // 没消息时候的心跳检测输出
      data = String(analogRead(1));
      Serial.println(data);
    }
   
    // webSocketClient.sendData(data);  // 发送消息，暂时用不到
    
  } else {
    Serial.println("Client disconnected.");
    iniwebsocket(); // 掉线重连
  }
  
  // wait to fully let the client disconnect
  delay(3000);
}


boolean restoreConfig() {
  Serial.println("Reading EEPROM...");
  String ssid = "";
  String pass = "";
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (int i = 32; i < 96; ++i) {
      pass += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(pass);
    WiFi.softAPdisconnect(true); // 关闭软AP模式
    delay(100);
    WiFi.begin(ssid.c_str(), pass.c_str());
    // WiFi.hostname("ESP_test");  // 修改设备名称
    return true;
  }
  else {
    Serial.println("Config not found.");
    return false;
  }
}

// 连接wifi
boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

//初始化websocket
boolean iniwebsocket(){
    // Connect to the websocket server
    if (client.connect(host, 9393)) {
      Serial.println("Connected");
    } else {
      Serial.println("Connection failed.");
    }
    // Handshake with the server
    webSocketClient.path = path;
    webSocketClient.host = host;
    if (webSocketClient.handshake(client)) {
      Serial.println("Handshake successful");
    } else {
      Serial.println("Handshake failed.");
    }
}

// 开启WebServer
void startWebServer() {
  if (settingMode) {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.softAPIP());
    webServer.on("/settings", []() {
      String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    webServer.on("/setap", []() {
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      String ssid = urlDecode(webServer.arg("ssid"));
      Serial.print("SSID: ");
      Serial.println(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      Serial.print("Password: ");
      Serial.println(pass);
      Serial.println("Writing SSID to EEPROM...");
      for (int i = 0; i < ssid.length(); ++i) {
        EEPROM.write(i, ssid[i]);
      }
      Serial.println("Writing Password to EEPROM...");
      for (int i = 0; i < pass.length(); ++i) {
        EEPROM.write(32 + i, pass[i]);
      }
      EEPROM.commit();
      Serial.println("Write EEPROM done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("AP mode", s));
    });
  }
  else {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    webServer.on("/", []() {
      String s = "<h1>Thank you for using!</h1>";
      webServer.send(200, "text/html", makePage("Index", s));
    });
    webServer.on("/bindingcode", []() {
      String s = "<h2>Your client number: ";
      s += client_id;
      s += "</h2>";
      webServer.send(200, "text/html", makePage("Index", s));
    });
  }
  webServer.begin();
}

// 初始化开启自身AP
void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID, apPWD);
  dnsServer.start(53, "*", apIP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
}

// webserver页面信息
String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

// 附近wifi 特殊名字编码转换
String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

