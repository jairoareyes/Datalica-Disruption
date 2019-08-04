#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7

#define ORG "auqt74"
#define DEVICE_TYPE "SabanaHack"
#define DEVICE_ID "IOTSiemens"
#define TOKEN "!66cd6aCIrfaY4*o0-"
#define pinTempSensor A0
#define pinMotor 12
const char* ssid = "SabanaHack";
const char* password = "Sabana.2019";

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;
int tempValue=0;
const char eventTopic[] = "iot-2/evt/status/fmt/json";
const char cmdTopic[] = "iot-2/cmd/led/fmt/json";

WiFiClient wifiClient;
void callback(char* topic, byte* payload, unsigned int payloadLength) {
  int data = *payload;
  int pwm;
  data = (data-48)*10;//convierte en int el ascii
  pwm = map(data, 1, 100, 50, 100);
  analogWrite(pinMotor,pwm);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");
  Serial.print("Info: ");
  Serial.print(pwm,DEC);	
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if (payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW); 
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}
PubSubClient client(server, 1883, callback, wifiClient);

int publishInterval = 2000; // 5 seconds//Send adc every 5sc
long lastPublishMillis;

void setup() {
  Serial.begin(9600); Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pinMotor, OUTPUT);
  wifiConnect();
  mqttConnect();
}

void loop() {
  if (millis() - lastPublishMillis > publishInterval) {
	tempValue = calculateTemp(analogRead(pinTempSensor));
    publishData(tempValue);
    lastPublishMillis = millis();
  }

  if (!client.loop()) {
    mqttConnect();
  }
}

void wifiConnect() {
  Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("nWiFi connected, IP address: "); Serial.println(WiFi.localIP());

}

void mqttConnect() {
  if (!!!client.connected()) {
    Serial.print("Reconnecting MQTT client to "); Serial.println(server);
    while (!!!client.connect(clientId, authMethod, token)) {
      Serial.print(".");
      delay(500);
    }
    if (client.subscribe(cmdTopic)) {
      Serial.println("subscribe to responses OK");
    } else {
      Serial.println("subscribe to responses FAILED");
    }
    Serial.println();
  }
}


void publishData(float temp) {
  String payload = "{\"d\":{\"luz\":";
  payload += String("30");
  payload += String(",\"temp\":");
  payload += String((int)round(temp), DEC);
  payload += String(",\"hum\":");
  payload += String("50");
  payload += String(",\"power\":");
  payload += String("70");
  payload += "}}";


  Serial.print("Sending payload: "); Serial.println(payload);

  if (client.publish(eventTopic, (char*) payload.c_str())) {
    Serial.println("Publish OK");
  } else {
    Serial.println("Publish FAILED");
  }

}

int calculateTemp(float adcValue){
	adcValue = (adcValue/1024) * 1000; //3300 is the voltage provided by NodeMCU
	adcValue = adcValue/10;
	// adcValue = adcValue ;
	adcValue=(int)round(adcValue);
	return adcValue;
}