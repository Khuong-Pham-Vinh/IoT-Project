#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(5,4); 
#define DHT11PIN 14
#define LED 13
#define DHTType DHT11
DHT HT(DHT11PIN, DHTType);

int humi,temp;
const char* id2 = "id2";
const char* on2 = "2on";
const char* off2 = "2off";

char all[200];

void setup()
{  
  HT.begin();
  Serial.begin(115200);
  mySerial.begin(9600);
  pinMode(LED, OUTPUT);
}

void loop()
{
  humi = (int)HT.readHumidity() + 0;
  temp = (int)HT.readTemperature() + 0;

  String rsp = mySerial.readStringUntil('\n'); 
  if (strstr(rsp.c_str(),id2) != NULL) {
    sprintf(all,"{\"id\": \"device2\",\"temperature2\": \"%d\",\"humidity2\": \"%d\",\"mode\":", temp, humi);// tao chuoi json
    mySerial.println(all);
  }
  if (strstr(rsp.c_str(),on2) != NULL) {
    digitalWrite(LED, HIGH);
  }
  if (strstr(rsp.c_str(),off2) != NULL) {
    digitalWrite(LED, LOW);
  }
}
