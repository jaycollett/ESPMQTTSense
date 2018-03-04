#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


#define WIFI_SSID "xxxxx"
#define WIFI_PASS "xxx"
#define MQTT_PORT 1883
char  mqtt_server[] = "192.168.0.0";
char  mqtt_username[] = "xxxxx";
char  mqtt_password[] = "xxxxxxx";

ADC_MODE(ADC_VCC);

unsigned long previousPublishMillis = 0;        // store the last time we published data  
const long publishInterval = 60000;             // interval (in MS) to update Adafruit IO data (1 min)
unsigned long currentMillis;                    // store LOT mcu has been running

const String baseTopic = "basementsensor";
const String tempTopic = baseTopic + "/" + "temperature";
const String humiTopic = baseTopic + "/" + "humidity";
const String presTopic = baseTopic + "/" + "pressure";
const String vccTopic  = baseTopic + "/" + "vcc";

char temperature[6];
char humidity[6];
char pressure[7];
char vcc[10];

IPAddress ip;

WiFiClient WiFiClient;
PubSubClient mqttclient(WiFiClient);
Adafruit_BME280 bme; // I2C

void setup() {
  Serial.begin(115200);
  delay(150);

  Wire.begin(0, 2);
  Wire.setClock(100000);
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    //while (1);
  }

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  mqttclient.setServer(mqtt_server, MQTT_PORT);
}


void loop() {
  
  // yield for wifi 
  yield();
  
  // check to see if it's time to publish based on millis
  currentMillis = millis();
  if (currentMillis - previousPublishMillis >= publishInterval) {
    bmeRead();
    vccRead();
    
    MQTT_connect(); // connect to wifi/mqtt as needed
    mqttclient.publish(tempTopic.c_str(), temperature);
    mqttclient.publish(humiTopic.c_str(), humidity);
    mqttclient.publish(presTopic.c_str(), pressure);
    mqttclient.publish(vccTopic.c_str(), vcc);
    
    previousPublishMillis = currentMillis;
  }
}

void bmeRead() {
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure()/100.0F;

  dtostrf(t, 5, 1, temperature);
  dtostrf(h, 5, 1, humidity);
  dtostrf(p, 5, 1, pressure);
}

void vccRead() {
  float v  = ESP.getVcc() / 1000.0;
  dtostrf(v, 5, 1, vcc);
}

// #############################################################################
//  mqtt Connect function
//  This function connects and reconnects as necessary to the MQTT server and
//  WiFi.
//  Should be called in the loop to ensure connectivity
// #############################################################################
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqttclient.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect(mqtt_server, mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
  Serial.println("MQTT Connected!");
}