#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


#define WIFI_SSID "xxxxxxxxxxx"					// your WiFi SSID
#define WIFI_PASS "xxxxxxxxxxxxx"				// your WiFi password 
#define MQTT_PORT 1883							// default MQTT broker port

char  fmversion[7] = "v1.9a";					// firmware version of this sensor
char  mqtt_server[] = "192.168.0.0";			// MQTT broker IP address
char  mqtt_username[] = "bmesensors";			// username for MQTT broker (USE ONE)
char  mqtt_password[] = "!bmesensors!";			// password for MQTT broker
char  mqtt_clientid[] = "basementsensor";		// client id for connections to MQTT broker

ADC_MODE(ADC_VCC);

unsigned long previousPublishMillis = 0;        // store the last time we published data  
const long publishInterval = 300000;            // interval (in MS) to update data (5 min)
unsigned long currentMillis;                    // store LOT mcu has been running

const String baseTopic = "basementsensor";
const String tempTopic = baseTopic + "/" + "temperature";
const String humiTopic = baseTopic + "/" + "humidity";
const String presTopic = baseTopic + "/" + "pressure";
const String vccTopic  = baseTopic + "/" + "vcc";
const String fwTopic   = baseTopic + "/" + "firmwarever";

char temperature[6];
char humidity[6];
char pressure[7];
char vcc[10];

IPAddress ip;

WiFiClient WiFiClient;
PubSubClient mqttclient(WiFiClient);
Adafruit_BME280 bme; // I2C init

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println("Starting Wire");
  
  Wire.begin(2, 0); // GPIO2 and GPIO0 on the ESP
  Wire.setClock(100000);
  Serial.println("Searching for sensors");
  
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
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
  
  yield(); // yield for wifi 
  
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
