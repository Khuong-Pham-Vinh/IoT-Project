/*......................................................INITIAL.............................................................*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include "string.h"
#include <WiFiManager.h>
#include "ticker.h"
#define Numberphone "+84339720400"
Ticker CheckNetwork;

volatile bool needReconnect = false;
volatile bool isUsing4G = false;
volatile bool isUsingWiFi = false;
String response;
int itv = 1000;

SoftwareSerial mySerial(5,4);
SoftwareSerial mySerial1(13,12);

const char* ssid = "khuong";     
const char* password = "15151515";  

const char* mqttServer = "broker.hivemq.com";
const int mqttport = 1883;
const char* mqtt_username = "Khuong123";
const char* mqtt_password = "Khuong@123";

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);
String rsp1,rsp2,rsp3;

/*.........................................................SETUP_SYSTEM..............................................................*/

void sim_at_wait(String &response) {  
  response = ""; 
  while (mySerial1.available()) {
    char c = mySerial1.read();
    response += c; 
    Serial.write(c); 
  }
}

String sim_at_cmd(String cmd) {
  mySerial1.println(cmd);
  delay(1000);
  sim_at_wait(response);
  return response; 
}

/*.........................................................4G_MODE...................................................................*/

void mqttPublish_4G_mode(String topic, String message) {
  Serial.println("Publishing a message..");
  sim_at_cmd("AT+CMQTTTOPIC=0," + String(topic.length()));
  sim_at_cmd(topic);
  sim_at_cmd("AT+CMQTTPAYLOAD=0," + String(message.length()));
  sim_at_cmd(message);
  sim_at_cmd("AT+CMQTTPUB=0,1,60");
  Serial.println("A message is: " + message);
}

void setup_MQTT_4G() {
  delay(500);
  sim_at_cmd("AT");
  sim_at_cmd("ATE0");

  sim_at_cmd("AT+CMQTTDISC=0,60");
  sim_at_cmd("AT+CMQTTREL=0");
  sim_at_cmd("AT+CMQTTSTOP");
  
  sim_at_cmd("AT+CMQTTSTART");
  sim_at_cmd("AT+CMQTTACCQ=0,\"arduino_client3578902017888778761h03\"");
  sim_at_cmd("AT+CMQTTCONNECT=0,\"tcp://broker.hivemq.com:1883\",60,1");
  delay(2000);
  Serial.println("MQTT connection successful!");
  isUsing4G = true;
}  

void sent_sms(String message)
{
    sim_at_cmd("AT+CMGF=1");  
    String cmd = "AT+CMGS=\"";
    cmd += Numberphone;      
    cmd += "\"";
    sim_at_cmd(cmd);          
    delay(100);            
    sim_at_cmd(message + (char)26);
}

/*.............................................................WIFI_MODE...........................................................*/

void setup_wifi() {
  WiFi.begin(ssid,password);
  Serial.print("Connecting to...");
  int attempt = 0;
  while ((WiFi.status() != WL_CONNECTED) && attempt < 6) {
    delay(1000);
    Serial.print(".");
    attempt++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed");
  }
}

void MQTT_WiFi_mode() {
  int retrycount = 0;
  while (!client.connected()&&retrycount<5) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX); 
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT server by WiFi mode");
      client.subscribe("wifinode/sensor3");
      //client.subscribe("khuong/sub/gateway");
      client.subscribe("khuong/alarm");
      client.subscribe("wifinode/data");
      return;
    }  else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" Try again in 1 second");
      delay(1000);
      retrycount++;
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for(int i=0; i<length;i++) incommingMessage += (char)payload[i];
    Serial.println("Massage arived ["+String(topic)+"] "+incommingMessage);
  if (strcmp(topic, "wifinode/data") == 0) {
      if ((char)payload[0] == '1' && (char)payload[2] == 'n') mySerial.println("1on");
      if ((char)payload[0] == '1' && (char)payload[2] == 'f') mySerial.println("1off");
      if ((char)payload[0] == '2' && (char)payload[2] == 'n') mySerial.println("2on");
      if ((char)payload[0] == '2' && (char)payload[2] == 'f') mySerial.println("2off");
      if ((char)payload[0] == '3' && (char)payload[2] == 'n') client.publish("wifinode/data","3on");
      if ((char)payload[0] == '3' && (char)payload[2] == 'f') client.publish("wifinode/data","3off");
  }
  if (strcmp(topic, "wifinode/sensor3") == 0)
    client.publish("wifinode/sensor3/server", incommingMessage.c_str()); // WiFi_node
  if (strcmp(topic, "khuong/alarm") == 0) {
    if (incommingMessage == "1tempover" || incommingMessage == "1tempunder" )
      sent_sms("Temperature of LoRa node 1 is out of safe range!");
    if (incommingMessage == "2tempover" || incommingMessage == "2tempunder" )
      sent_sms("Temperature of LoRa node 2 is out of safe range!");
    if (incommingMessage == "3tempover" || incommingMessage == "3tempunder" )
      sent_sms("Temperature of Wi-Fi node 3 is out of safe range!");
    if (incommingMessage == "1humiover" || incommingMessage == "1humiunder" )
      sent_sms("Humidity of LoRa node 1 is out of safe range!");
    if (incommingMessage == "2humiover" || incommingMessage == "2humiunder" )
      sent_sms("Humidity of LoRa node 2 is out of safe range!");
    if (incommingMessage == "3humiover" || incommingMessage == "3humiunder" )
      sent_sms("Humidity of Wi-Fi node 3 is out of safe range!");
  }
}

/*..........................................................CHECK_NETWORK....................................................................*/

void checkNetwork() {
  if (WiFi.status() != WL_CONNECTED && isUsing4G == false) {
    needReconnect = true;
  }
  if (WiFi.status() == WL_CONNECTED && isUsing4G == true) {
    isUsing4G = false;
    needReconnect = false;
    isUsingWiFi = true;
  }
}

/*...............................................................SETUP.......................................................................*/

void setup()
{  
  Serial.begin(115200);
  mySerial.begin(9600);
  mySerial1.begin(115200);

  client.setServer(mqttServer,mqttport);
  client.setCallback(callback);

  setup_wifi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not available! Switching 4G...");
    setup_MQTT_4G();
  } else MQTT_WiFi_mode();

  CheckNetwork.attach(3, checkNetwork);
}

/*..............................................................LOOP.......................................................................*/

void loop()
{
  client.loop();
  mySerial.println("id1");
  delay(itv);
  rsp1 = mySerial.readStringUntil('\n');

  mySerial.println("id2");
  delay(itv);
  rsp2 = mySerial.readStringUntil('\n');

  if (needReconnect) {
    Serial.println("Reconnecting to WiFi...");
    setup_wifi();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi!");
      MQTT_WiFi_mode();
      needReconnect = false;
    } else {
      Serial.println("Reconnecting to 4G...");
      setup_MQTT_4G();
      needReconnect = false;
    }
  }
  if (isUsingWiFi){ 
    MQTT_WiFi_mode();
    isUsingWiFi = false;
  }

    if (WiFi.status() != WL_CONNECTED && isUsing4G == true) {
      mqttPublish_4G_mode("khuong/sensor1", rsp1 + "\"4G\"}");
      delay(1000);
      mqttPublish_4G_mode("khuong/sensor2", rsp2 + "\"4G\"}"); 
      delay(1000);
    }
    if (WiFi.status() == WL_CONNECTED) {
      client.publish("khuong/sensor1", (rsp1 + "\"WiFi\"}").c_str()); 
      client.publish("khuong/sensor2", (rsp2 + "\"WiFi\"}").c_str());  
      delay(itv);
      client.publish("wifinode/data", "id3");
    }
}
/*................................................................THE_END......................................................................*/ 