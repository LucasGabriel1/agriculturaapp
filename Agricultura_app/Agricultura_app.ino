
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <stdlib.h>

const char* ssid = "Valdecir 2GHz";  
const char* password = "02040810";

const char* mqtt_server = "54.207.34.223";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

int valvula = 0;
float vazao;
int humidity;
int Umin;
int Umax;
float umidadeAtual;

// LED Pin
const int ledPin = 33;
const int fluxo = 27;

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}


void setup() {
  Serial.begin(115200);
  pinMode(33, OUTPUT);
  pinMode (34, INPUT);
  pinMode (27, INPUT);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(fluxo), pulseCounter, FALLING);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message

  if (String(topic) == "esp32/umidadeMaxDesejada") {
    Serial.print("Umidade Max: ");
    Umax = messageTemp.toInt();
    Serial.println(Umax);
}
  if (String(topic) == "esp32/umidadeMinDesejada") {
    Serial.print("Umidade Min: ");
    Umin = messageTemp.toInt();
    Serial.println(Umin);
  }

  if (String(topic) == "esp32/umidadeAtual") {
    Serial.print("Umidade Atual: ");
    umidadeAtual = messageTemp.toInt();
    Serial.println(umidadeAtual);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/umidadeMaxDesejada");
      client.subscribe("esp32/umidadeMinDesejada");
      client.subscribe("esp32/umidadeAtual");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(1000);
    }
  }
}
void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    
    pulse1Sec = pulseCount;
    pulseCount = 0;
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalMilliLitres / 1000);
    Serial.println("L");
  }

    vazao = flowRate;
    // Convert the value to a char array
    char vzString[8];
    dtostrf(vazao, 1, 2, vzString);
    Serial.print("Vazao: ");
    Serial.println(vzString);
    client.publish("esp32/Vazão", vzString);

    //humidity = analogRead(34);
    humidity = analogRead(34);
    // Convert the value to a char array
    char humString[8];
    dtostrf(humidity, 1, 2, humString);
    Serial.print("Humidity: ");
    Serial.println(humString);
    client.publish("esp32/Umidade", humString);

    char valvulaString [8];

    if ( umidadeAtual < Umin)
        {
          valvula = 1;
          dtostrf(valvula, 1, 2, valvulaString);
          digitalWrite(ledPin, HIGH);
          client.publish("esp32/valvula", valvulaString);
          Serial.println ("Ligado");
         }
  
        if ( umidadeAtual > Umax)
        {
          valvula = 0;
          digitalWrite(ledPin, LOW);
          dtostrf(valvula, 1, 2, valvulaString);
          client.publish("esp32/valvula", valvulaString);
          Serial.println ("Desligado");
        
        }
  } 
}
