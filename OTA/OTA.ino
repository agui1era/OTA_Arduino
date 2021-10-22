#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

const char* ssid = "WifiAX";        // WiFi name
const char* password = "hkmhkm1234566";    // WiFi password

//Definicion MQTT
WiFiClient espClient;
PubSubClient client(espClient);
PubSubClient client_THB(espClient);

const char* mqttServer = "mqtt.cloud.kaaiot.com";
const char* mqtt_server_THB = "iot.igromi.com";
const char* mqtt_id = "bridge001";
const char* mqtt_user = "igromi";
const char* mqtt_pass = "imagina12";
const char* topic="v1/devices/me/telemetry";

//KAA configuración
const String TOKEN = "imagina12";        // Endpoint token - you get (or specify) it during device provisioning
const String APP_VERSION = "c5mum6104t2n6tdhrln0-v1";  // Application version - you specify it during device provisioning

String str;
char payload[100];
int pinState;


void setup() {
  Serial.begin(115200);
  client.setServer(mqttServer, 1883);
  client_THB.setServer(mqtt_server_THB, 1883);
  client.setCallback(handleOtaUpdate);
  initServerConnection();

  if (client.setBufferSize(1023)) {
    Serial.println("Successfully reallocated internal buffer size");
  } else {
    Serial.println("Failed to reallocated internal buffer size");
  }

  delay(1000);
  reportCurrentFirmwareVersion();
  requestNewFirmware();
}

void loop() {
  // Do work here
  Serial.println("Test A");
  EnvioMQTT(1,"KeepAlive"); 
  initServerConnection();
  delay(1000);
}

void reportCurrentFirmwareVersion() {
  String reportTopic = "kp1/" + APP_VERSION + "/cmx_ota/" + TOKEN + "/applied/json";
  String reportPayload = "{\"configId\":\"1.0.0\"}";
  Serial.println("Reporting current firmware version on topic: " + reportTopic + " and payload: " + reportPayload);
  client.publish(reportTopic.c_str(), reportPayload.c_str());
}

void requestNewFirmware() {
  int requestID = random(0, 99);
  String firmwareRequestTopic = "kp1/" + APP_VERSION + "/cmx_ota/" + TOKEN + "/config/json/" + requestID;
  Serial.println("Requesting firmware using topic: " + firmwareRequestTopic);
  client.publish(firmwareRequestTopic.c_str(), "{\"observe\":true}"); // observe is used to specify whether the client wants to accept server pushes
}

void initServerConnection() {
  setupWifi();
  if (!client.connected()) {
    reconnect();
  };
  if (!client_THB.connected()) {
    reconnect_THB();
  }
  client.loop();
}

void handleOtaUpdate(char* topic, byte* payload, unsigned int length) {
  Serial.printf("\nHandling firmware update message on topic: %s and payload: ", topic);

  DynamicJsonDocument doc(1023);
  deserializeJson(doc, payload, length);
  JsonVariant json_var = doc.as<JsonVariant>();
  Serial.println(json_var.as<String>());
  if (json_var.isNull()) {
    Serial.println("No new firmware version is available");
    return;
  }

  unsigned int statusCode = json_var["statusCode"].as<unsigned int>();
  if (statusCode != 200) {
    Serial.printf("Firmware message's status code is not 200, but: %d\n", statusCode);
    return;
  }

  String firmwareLink = json_var["config"]["link"].as<String>();

  t_httpUpdate_return ret = httpUpdate.update(espClient, firmwareLink.c_str());

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

void setupWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.println();
    Serial.printf("Connecting to [%s]", ssid);
    WiFi.begin(ssid, password);
    connectWiFi();
  }
}

void connectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    char *client_id = "bqf1uai03p4cop6jr3u0";
    if (client.connect(client_id)) {
      Serial.println("Connected to WiFi");
      subscribeToFirmwareUpdates();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
};

void reconnect_THB() 
{
  // Loop de reconexion MQTT
  while (!client_THB.connected()) {
    Serial.print("Conectando a MQTT...");
    // Intentado conectar
    if (client_THB.connect(mqtt_id,mqtt_user,mqtt_pass)) {
      Serial.println("conectado al server");
      
    } else {
      Serial.print("RC-");
      Serial.println(client.state());
      Serial.println("Reintentando....");
      delay(500);      
  }
 }
};

//Rutina para el envio de datos por MQTT
void EnvioMQTT(float Data,String ID) {
      //Se genera estructura de thingsboard
      str= "{\""+ID+"\":\""+String(Data)+"\"}";
      str.toCharArray(payload,100);
      Serial.println(payload);
      client_THB.publish(topic,payload);
};

void subscribeToFirmwareUpdates() {
  String serverPushOnConnect = "kp1/" + APP_VERSION + "/cmx_ota/" + TOKEN + "/config/json/#";
  client.subscribe(serverPushOnConnect.c_str());
  Serial.println("Subscribed to server firmware push on topic: " + serverPushOnConnect);

  String serverFirmwareResponse = "kp1/" + APP_VERSION + "/cmx_ota/" + TOKEN + "/config/json/status/#";
  client.subscribe(serverFirmwareResponse.c_str());
  Serial.println("Subscribed to server firmware response on topic: " + serverFirmwareResponse);

  String serverFirmwareErrorResponse = "kp1/" + APP_VERSION + "/cmx_ota/" + TOKEN + "/config/json/status/error";
  client.subscribe(serverFirmwareErrorResponse.c_str());
  Serial.println("Subscribed to server firmware response on topic: " + serverFirmwareErrorResponse);
}
