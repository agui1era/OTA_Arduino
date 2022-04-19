#include <esp_task_wdt.h>
//120 seconds WDT
#define WDT_TIMEOUT 120
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#define  QoS 1

const char* ssid = "WifiAX";        
const char* password = "hkmhkm1234566";   
const char* mqttServer = "mqtt.cloud.kaaiot.com";
const char* mqtt_server_THB = "iot.igromi.com";
const char* mqtt_server_Local = "192.168.3.177";
const char* mqtt_id = "contador1000";
const char* mqtt_user = "igromi";
const char* mqtt_pass = "imagina12";
const char* topic="localTopic";
const char* suscriber ="v1/devices/me/attributes";


//Definicion MQTT
WiFiClient espClient;
PubSubClient client(espClient);

WiFiClient espClient_THB;
PubSubClient client_THB(espClient_THB);

WiFiClient espClient_Local;
PubSubClient client_Local(espClient_Local);


//KAA configuración
const String TOKEN = "bridge_ota";        // Endpoint token - you get (or specify) it during device provisioning
const String APP_VERSION = "c8u9t4tah5mis9vv3hdg-v1";  // Application version - you specify it during device provisioning

//Variables varias
String str;
char payload[100];
unsigned long previous_time = 0;
unsigned long previous_time_THB = 0; 
const long interval = 60000;
const long interval_THB = 1000;

int contador=0;
int contador2=0;
int contador3=0;
int contador4=0;

boolean estado=false;
boolean estado2=false;
boolean estado3=false;
boolean estado4=false;


boolean estadoB=false;
boolean estadoB2=false;
boolean estadoB3=false;
boolean estadoB4=false;

//IO pines
const int pinIN1=27;
const int pinIN2=26;
const int pinIN3=25;
const int pinIN4=14;

const int pinADC1=32;
const int pinADC2=33;
const int pinADC3=34;
const int pinADC4=35;
    
const int rele1=12;
const int rele2=13;

int ciclos=0;

int flag_reconectar=1;
    
void setup() {
  
  Serial.begin(115200);

  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch 
  
  pinMode(pinIN1, INPUT);
  pinMode(pinIN2, INPUT);
  pinMode(pinIN3, INPUT);
  pinMode(pinIN4, INPUT);
  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  
  client.setServer(mqttServer, 1883);
  client.setCallback(handleOtaUpdate);
  client_THB.setServer(mqtt_server_THB, 1883);
  client_THB.setCallback(callback_THB);
  client_Local.setServer(mqtt_server_Local, 1883);
  initServerConnection();

  if (client.setBufferSize(1023)) {
    Serial.println("Successfully reallocated internal buffer size");
  } else {
    Serial.println("Failed to reallocated internal buffer size");
  };

  delay(1000);
  reportCurrentFirmwareVersion();
  requestNewFirmware();
  Serial.println("TERMINO SETUP");
}

void loop() {
  
  // Do work here
  estado=digitalRead(pinIN1);
  estado2=digitalRead(pinIN2);
  estado3=digitalRead(pinIN3);
  estado4=digitalRead(pinIN4);
  
  delay(25); 
  
  estadoB=digitalRead(pinIN1);
  estadoB2=digitalRead(pinIN2);
  estadoB3=digitalRead(pinIN3);
  estadoB4=digitalRead(pinIN4);
  
  if ((estado == false) && (estadoB == true))
    {
    contador++;
    Serial.print("contador: ");
    Serial.println(contador);
    };

  if ((estado2 == false) &&( estadoB2 == true))
    {
    contador2++;
    Serial.print("contador2: ");
    Serial.println(contador2);
    
    };

  if ((estado3 == false) && (estadoB3 == true))
    {
    contador3++;
    Serial.print("contador3: ");
    Serial.println(contador3);
    };
  
  if ((estado4 == false) && (estadoB4 == true))
    {
    contador4++;
    Serial.print("contador4: ");
    Serial.println(contador4);
    };
      
    MuestreoTHB();
    esp_task_wdt_reset();  

};

//Envio Firmware actual
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


//Rutina de manejo cola de actualización firmware
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
};

//Topico de suscripcion de actualizacion de firmware
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

//Rutina de suscripción cola THB
void callback_THB(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    //Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  if (messageTemp.equals("{\"0\":0}"))
      {
        digitalWrite(rele1,0);
      };
  if (messageTemp.equals("{\"0\":1}"))
      {
        digitalWrite(rele1,1);
      };
  if (messageTemp.equals("{\"1\":0}"))
      {
        digitalWrite(rele2,0);
      };
  if (messageTemp.equals("{\"1\":1}"))
      {
        digitalWrite(rele2,1);
      };       
};

//Configuracón WIFI
void setupWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.println();
    Serial.printf("Connecting to [%s]", ssid);
    WiFi.begin(ssid, password);
    connectWiFi();
  }
}

//Conexión WIFI
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


//Conexion a MQTT Local
void reconnect_Local(){
  while (!client_Local.connected()) {
    Serial.println("Intentando conectar a MQTT Local...");
    // Intentado conectar
    if (client_Local.connect(mqtt_id,mqtt_user,mqtt_pass)) {
      Serial.println("conectado al server Local");
     
    } else {
      Serial.print("RC-");
      Serial.println(client_Local.state());
      Serial.println("Reintentando....");
      delay(500);      
    }
  }
  Serial.println("Saliendo rutina de conexion MQTT Local");
 };

 //Conexion a MQTT Thingsboard
void reconnect_THB(){
  if(!client_THB.connected()&&(flag_reconectar==1)) {
    Serial.println("Intentando conectar a MQTT THB...");
    // Intentado conectar
    if (client_THB.connect(mqtt_id,mqtt_user,mqtt_pass)) {
      Serial.println("conectado al server THB");
      client_THB.subscribe(suscriber,QoS);
    } else {
      Serial.println("No se pudo conectar a THB");
      flag_reconectar=0;     
    }
  }
  Serial.println("Saliendo rutina de conexion MQTT THB");
 };

//Conexion a MQTT KAA
void reconnect() {
   if (!client.connected()&&(flag_reconectar==1)) {
    Serial.println("Intentando conectar MQTT KAA...");
    char *client_id = "bqf1uai03p4cop6jr3u0";
    if (client.connect(client_id)) {
      Serial.println("conectado al server KAA");
      subscribeToFirmwareUpdates();
    } else {
      Serial.println("No se puedo conectar a KAA");
      flag_reconectar=0;   
    }
  }
  Serial.println("Saliendo rutina de conexion MQTT KAA");
};



//Conexion inicial THB y KAA
void initServerConnection() {
  
   setupWifi();

   //Verifico conexión Local
  if (!client_Local.connected()) {
    reconnect_Local();
  };

  //Verifico conexión MQTT THB
  if (!client_THB.connected()) {
    reconnect_THB();
  };

  //Verifico conexión KAA
  if (!client.connected()) {
    reconnect();
  };
   client.loop();
   client_THB.loop();
     
}


//Rutina para el envio de datos por MQTT
void EnvioMQTT(float Data,String ID) {
      //Se genera estructura de thingsboard
      str= "{\""+ID+"\":\""+String(Data)+"\"}";
      str.toCharArray(payload,100);
      Serial.println(payload);
      client_Local.publish(topic,payload,QoS);
};



//Rutina de envio de datos a THB 60s y verifico la suscripción 1s
void MuestreoTHB(){

  unsigned long current_time = millis();
  if (current_time - previous_time >= interval) {
     
     previous_time = current_time;
     Serial.println("Inicio de muestreo");
     Serial.print("Tiempo inicial: "); 
     Serial.println(current_time);
    
     initServerConnection();

     //Envio de datos al broker MQTT
     EnvioMQTT(1.0,"version");
     EnvioMQTT(1000,"id");
     EnvioMQTT(contador,"contador");
     EnvioMQTT(contador2,"contador2");
     EnvioMQTT(contador3,"contador3");
     EnvioMQTT(contador4,"contador4");
     EnvioMQTT(digitalRead(pinIN1),"IN1");
     EnvioMQTT(digitalRead(pinIN2),"IN2");
     EnvioMQTT(digitalRead(pinIN3),"IN3");
     EnvioMQTT(digitalRead(pinIN4),"IN4");

     EnvioMQTT(analogRead(pinADC1),"ADC1");
     EnvioMQTT(analogRead(pinADC2),"ADC2");
     EnvioMQTT(analogRead(pinADC3),"ADC3");
     EnvioMQTT(analogRead(pinADC4),"ADC4");

     EnvioMQTT(digitalRead(rele1),"Rele1");
     EnvioMQTT(digitalRead(rele2),"Rele2");

     contador=0;
     contador2=0;
     contador3=0;
     contador4=0;
     
     Serial.print("Ciclos: "); 
     Serial.println(ciclos);
     ciclos++;

     current_time = millis();
     Serial.print("Tiempo final: "); 
     Serial.println(current_time);
     Serial.print("Flag de reconectar: ");
     Serial.println(flag_reconectar);
  };

  //Verifico cola MQTT THB y KAA
   current_time = millis();
  if (current_time - previous_time_THB >= interval_THB) {
   previous_time_THB = current_time;
   client.loop();
   client_THB.loop();

  if (ciclos > 1440 ){
    Serial.println("Vuelvo intentar reconectar: ");
    flag_reconectar=1;
    ciclos=0;   
    }
   
 }
};
