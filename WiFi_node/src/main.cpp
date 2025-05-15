#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
SoftwareSerial mySerial(5,4); 
#define DHT11PIN 14
#define LED 13
#define DHTType DHT11
DHT HT(DHT11PIN, DHTType);

int humi,temp;
const char* id3 = "id3";
const char* on3 = "3on";
const char* off3 = "3off";
const char* ssid = "khuong";     
const char* password = "15151515";  
const char* mqttServer = "broker.hivemq.com";
const int mqttport = 1883;
char all[200];

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);

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
    String clientId = "ESP8266Clientttt-";
    clientId += String(random(0xffff), HEX); 
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT server by WiFi mode");
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
      if ((char)payload[0] == '3' && (char)payload[2] == 'n') digitalWrite(LED, HIGH);
      if ((char)payload[0] == '3' && (char)payload[2] == 'f') digitalWrite(LED, LOW);
      if ((char)payload[0] == 'i' && (char)payload[2] == '3') {
         sprintf(all,"{\"id\": \"device3\",\"temperature3\": \"%d\",\"humidity3\": \"%d\",\"mode\":\"WiFi\"}", temp, humi);// tao chuoi json
         client.publish("wifinode/sensor3",all);
      }
  }
}

void setup()
{  
  HT.begin();
  Serial.begin(115200);
  mySerial.begin(9600);
  pinMode(LED, OUTPUT);
  setup_wifi();

  client.setServer(mqttServer, mqttport);
  client.setCallback(callback);
  MQTT_WiFi_mode();
}

void loop()
{
  humi = (int)HT.readHumidity() + 0;
  temp = (int)HT.readTemperature() + 0;
  client.loop();
}
