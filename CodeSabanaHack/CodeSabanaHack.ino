#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     20000
#define pinPresionSensor A0
#define pinPum D5
#define pinValve D6

// Variables del PulsoOximetro
PulseOximeter pox;
uint32_t tsLastReport = 0;
uint8_t nSamples = 0;
uint8_t nSamplesPulOxi = 0;
bool isPulOxi = false;
bool isPresion = false;
float BPMArray[25];
float BPMProm=0;

float spO2[25];
float spO2Prom=0;

float presionValue=0;
//-------- Customise these values -----------
// const char* ssid = "FAMILIA RAMIREZ";
// const char* password = "41724812";

const char* ssid = "Jairo WiFi";
const char* password = "123456789";

#define ORG "j8g7yn"
#define DEVICE_TYPE "NodeMCU"
#define DEVICE_ID "NodeTeleciudado"
#define TOKEN "g1GoBB24JUjO!u3n+)"

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

const char eventTopic[] = "iot-2/evt/status/fmt/json";
const char cmdTopic[] = "iot-2/cmd/led/fmt/json";



WiFiClient wifiClient;
void callback(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < payloadLength; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if (payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}
PubSubClient client(server, 1883, callback, wifiClient);

void setup() {
  pinMode(pinPum,OUTPUT);
  pinMode(pinValve,OUTPUT);
  Serial.begin(115200);

  //   //Inicializa WIFI y MQTT
  // wifiConnect();
  // mqttConnect();

  //Inicializa PulOxi
  Serial.print("Initializing pulse oximeter..");

  pox.begin();
  if (!pox.begin()) {
      Serial.println("FAILED");
      //for(;;);
  } else {
      Serial.println("SUCCESS");
      isPulOxi = true;
  }
  // The default current for the IR LED is 50mA and it could be changed
  //   by uncommenting the following line. Check MAX30100_Registers.h for all the
  //   available options.
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
    if (isPulOxi)
      pox.update();
    if (isPresion){
      calculatePresion();
      Serial.println("Finalizado");
      isPresion = false;
    }
    if (millis() - tsLastReport > REPORTING_PERIOD_MS && isPulOxi) {
      cauculatePulOxi();
      nSamples=0;
      isPulOxi = false;
      isPresion = true;
      pox.shutdown();
    }
}

void wifiConnect() {
  Serial.println();
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
    Serial.println();
  }
}

// Callback (registered below) fired when a pulse is detected
void onBeatDetected(){   
  BPMArray[nSamples] = pox.getHeartRate();
  spO2[nSamples] = pox.getSpO2();
  Serial.print("Heart rate:");
  Serial.print(BPMArray[nSamples]);
  Serial.print("bpm / SpO2: ");
  Serial.print(spO2[nSamples]);
  Serial.println("%");
  nSamples++;
}

void publishData(float dataBPM, float dataSpO2) {
  String payload = "{\"d\":{\"user\":";
  payload+=String("\"diana\",");
  payload += String("\"sp02\":");
  payload += String((int)round(dataSpO2), DEC);
  payload += String(",\"bpm\":");
  payload += String((int)round(dataBPM), DEC);
  payload += "}}";

  Serial.print("Sending payload: "); Serial.println(payload);
  if (client.publish(eventTopic, (char*) payload.c_str())) {
      Serial.println("Publish OK");
  } else {
      Serial.println("Publish FAILED");
  }
}

void cauculatePulOxi(){
  BPMProm=0;
  spO2Prom=0;
  if (nSamples==0){
    BPMProm = 0;
    spO2Prom = 0;
  }
  else {
    for (int i=0;i<nSamples;i++){
        BPMProm += BPMArray[i];
        spO2Prom += spO2[i];
    }
    BPMProm = BPMProm/nSamples;
    spO2Prom = spO2Prom/nSamples;
  }
  Serial.print("BPM Promedio: ");
  Serial.print(BPMProm);
  Serial.print(" | SpO2 Promedio: ");
  Serial.println(spO2Prom);
}

void calculatePresion(){
  presionValue = analogRead(pinPresionSensor);
  Serial.println(presionValue);
  delay(100);
  presionValue = analogRead(pinPresionSensor);
  digitalWrite(pinPum,HIGH);  
  while (presionValue<700){
    Serial.println((int)presionValue);
    presionValue = analogRead(pinPresionSensor);
    delay(50);
  } 
  digitalWrite(pinPum,LOW);
  delay(3000);
  presionValue = (presionValue*3.1);
  presionValue = (presionValue/1024);
  Serial.print("ADC Value: ");
  Serial.print(presionValue);
  presionValue = presionValue - 1.5233;
  presionValue = presionValue / 0.004465;
  Serial.print(" | Presion: ");
  Serial.println(presionValue);
  digitalWrite(pinValve,HIGH);
  delay(500);
  //publishData(BPMProm,spO2Prom);
}