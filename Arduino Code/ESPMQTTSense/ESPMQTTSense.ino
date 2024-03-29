#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFiManager.h>                  //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#define MQTT_PORT 1883							      // default MQTT broker port

char  fmversion[7] = "v2.6";					    // firmware version of this sensor
char  mqtt_server[] = "192.168.0.xxxx";		// MQTT broker IP address
char  mqtt_username[] = "xxxxxxxxx";			// username for MQTT broker (USE ONE)
char  mqtt_password[] = "xxxxxxxxxxx";		// password for MQTT broker
char  mqtt_clientid[] = "remotesense4";	  // client id for connections to MQTT broker

ADC_MODE(ADC_VCC);

unsigned long previousPublishMillis = 0;  // store the last time we published data  
const long publishInterval = 300000;      // interval (in MS) to update data (5 min)
unsigned long currentMillis;              // store LOT mcu has been running
unsigned int runCount = 0;

const String baseTopic = "remotesense4";
const String tempTopic = baseTopic + "/" + "temperature";
const String humiTopic = baseTopic + "/" + "humidity";
const String presTopic = baseTopic + "/" + "pressure";
const String vccTopic  = baseTopic + "/" + "vcc";
const String fwTopic   = baseTopic + "/" + "firmwarever";

char temperature[6];
char humidity[6];
char pressure[7];
char vcc[10];

WiFiManager wifiManager;
WiFiClient wifiClient;
PubSubClient mqttclient(wifiClient);
Adafruit_BME280 bme; // I2C init

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Wire");
  WiFi.mode(WIFI_STA);
    
  Wire.begin(2, 0); // GPIO2 and GPIO0 on the ESP
  Wire.setClock(100000);
  Serial.println("Searching for sensors");
  
   if (!bme.begin(0x76)) {
     Serial.println("Could not find a valid BME280 sensor, check wiring!");
     delay(3000);
     ESP.reset();
     delay(5000);
   }

  Serial.println();
  Serial.print("Connecting to WiFi...");

  // configure the wifi manager to timeout and
  wifiManager.setConfigPortalTimeout(180);
  if(!wifiManager.autoConnect()) {
      Serial.println("Failed to connect, restarting ESP...");
      delay(3000);
      ESP.reset();
      delay(5000);
  }

  mqttclient.setServer(mqtt_server, MQTT_PORT);
}

void loop() {
  
  yield(); // yield for wifi 
  
  // check to see if it's time to publish based on millis
  currentMillis = millis();
  if ( (currentMillis - previousPublishMillis >= publishInterval) || (runCount == 0) ){
    runCount = 1;
    bmeRead();
    vccRead();
    
    MQTT_connect(); // connect to wifi/mqtt as needed
    mqttclient.publish(tempTopic.c_str(), temperature);
    mqttclient.publish(humiTopic.c_str(), humidity);
    mqttclient.publish(presTopic.c_str(), pressure);
    mqttclient.publish(vccTopic.c_str(), vcc);
	  mqttclient.publish(fwTopic.c_str(), fmversion);
    
    previousPublishMillis = currentMillis;
  }
}

void bmeRead() {
  float t = (bme.readTemperature()*9/5+32-13);  // converted to F from C
  float h = bme.readHumidity();
  float p = bme.readPressure()/3389.39; // get pressure in inHg

  dtostrf(t, 5, 2, temperature); // 5 chars total, 2 decimals (BME's are really accurate)
  dtostrf(h, 5, 2, humidity);	 // 5 chars total, 2 decimals (BME's are really accurate)
  dtostrf(p, 5, 2, pressure);	 // 5 chars total, 2 decimals (BME's are really accurate)
}

void vccRead() {
  float v  = ESP.getVcc() / 1000.0;
  dtostrf(v, 5, 2, vcc); // 5 chars total, 2 decimals
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
    if (mqttclient.connect(mqtt_clientid, mqtt_username, mqtt_password)) {
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
