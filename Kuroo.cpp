#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <DHTesp.h>
#include <BH1750.h>
#define LED_COUNT 3
#define LED_GREEN 5
#define LED_YELLOW 18
#define LED_RED 19
#define DHT_PIN 15

const uint8_t arLed[LED_COUNT] = {LED_RED, LED_YELLOW, LED_GREEN};

#define WIFI_SSID "los"
#define WIFI_PASSWORD "12345678"
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_TOPIC_PUBLISH "esp32_carlos1/data"
#define MQTT_TOPIC_SUBSCRIBE "esp32_carlos1/cmd"

float globalHumidity = 0, globalTemp = 0, globalLux = 0;
int g_nCount=0;
Ticker timer1Sec;
Ticker timerLedBuiltinOff;
Ticker timerReadSensor;
Ticker timerPublish, sendLux, sendTemp, sendHumid ,ledOff;
Ticker timerMqtt;
DHTesp dht;
BH1750 lightMeter;

char g_szDeviceId[30];
WiFiClient espClient;
PubSubClient mqtt(espClient);
boolean mqttConnect();
void WifiConnect();
void onPublishMessage();
bool turnOn = false;
float fHumidity = 0;
float fTemperature = 0;
float lux = 0;
char szTopic[50];
char szData[10];


void onPublishTemperature() {
fTemperature = dht.getTemperature();
Serial.printf("\nTemperature: %.2f C\n", fTemperature);
sprintf(szTopic, "%s/temp", MQTT_TOPIC_PUBLISH);
sprintf(szData, "%.2f", fTemperature);
mqtt.publish(szTopic, szData);
}

void onPublishHumidity() {
fHumidity = dht.getHumidity();
Serial.printf("\nHumidity: %.2f\n", fHumidity);
sprintf(szTopic, "%s/humidity", MQTT_TOPIC_PUBLISH);
sprintf(szData, "%.2f", fHumidity);
mqtt.publish(szTopic, szData);
}

void onPublishLight() {
lux = lightMeter.readLightLevel();
Serial.printf("\nLight: %.2f lx\n", lux);
sprintf(szTopic, "%s/light", MQTT_TOPIC_PUBLISH);
sprintf(szData, "%.2f", lux);
mqtt.publish(szTopic, szData);
ledOff.once_ms(100, [](){
digitalWrite(LED_BUILTIN, LOW);
});

digitalWrite(LED_RED, (lux > 400)?HIGH:LOW);
digitalWrite(LED_GREEN, (lux < 400)?HIGH:LOW);
if (lux > 400)
{
Serial.printf("\npintu terbuka!");
}else
{
Serial.printf("\npintu tertutup!");
}
}

void setup() {
Serial.begin(115200);
delay(100);
pinMode(LED_BUILTIN, OUTPUT);
for (uint8_t i=0; i<LED_COUNT; i++)
pinMode(arLed[i], OUTPUT);
Wire.begin();
lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
Serial.printf("Free Memory: %d\n", ESP.getFreeHeap());
WifiConnect();
dht.setup(DHT_PIN, DHTesp::DHT11);
mqttConnect();

timerPublish.attach_ms(2000, onPublishMessage);
sendTemp.attach_ms(7000, onPublishTemperature);
sendHumid.attach_ms(5000, [](){ onPublishHumidity(); });
sendLux.attach_ms(4000, [](){ onPublishLight(); });
}

void loop() {
mqtt.loop();
}


void mqttCallback(char* topic, byte* payload, unsigned int len) {
String strTopic = topic;
int8_t idx = strTopic.lastIndexOf('/') + 1;
String strDev = strTopic.substring(idx);
Serial.printf("==> Recv [%s]: ", topic);
Serial.write(payload, len);
Serial.println();

if (strcmp(topic, MQTT_TOPIC_SUBSCRIBE) == 0) {
payload[len] = '\0';
Serial.printf("==> Recv [%s]: ", payload);
if (strcmp((char*)payload, "led-on") == 0) {
turnOn = true;
digitalWrite(LED_YELLOW, HIGH);
}
else if (strcmp((char*)payload, "led-off") == 0) {
turnOn = false;
digitalWrite(LED_YELLOW, LOW);
}
}
}
void onPublishMessage() {
szTopic[50];
szData[10];
digitalWrite(LED_BUILTIN, HIGH);
}


boolean mqttConnect() {
sprintf(g_szDeviceId, "esp32_%08X",(uint32_t)ESP.getEfuseMac());
mqtt.setServer(MQTT_BROKER, 1883);
mqtt.setCallback(mqttCallback);
Serial.printf("Connecting to %s clientId: %s\n", MQTT_BROKER, g_szDeviceId);

boolean fMqttConnected = false;
for (int i=0; i<3 && !fMqttConnected; i++) {
Serial.print("Connecting to mqtt broker...");
fMqttConnected = mqtt.connect(g_szDeviceId);
if (fMqttConnected == false) {
Serial.print(" fail, rc=");
Serial.println(mqtt.state());
delay(1000);
}
}

if (fMqttConnected)
{
Serial.println(" success");
mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE);
Serial.printf("Subcribe topic: %s\n", MQTT_TOPIC_SUBSCRIBE);
}
return mqtt.connected();
}

void WifiConnect()
{
WiFi.mode(WIFI_STA);
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
while (WiFi.waitForConnectResult() != WL_CONNECTED) {
Serial.println("Connection Failed! Rebooting...");
delay(5000);
ESP.restart();
}
Serial.print("System connected with IP address: ");
Serial.println(WiFi.localIP());
Serial.printf("RSSI: %d\n", WiFi.RSSI());
}





