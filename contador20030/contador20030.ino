#include <esp_task_wdt.h>
//120 seconds WDT
#define WDT_TIMEOUT 120
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#define  QoS 1
#define DAC1 26
#define DAC2 25
#define arraySize 15

 
//Configs
int         id = 20030;
const char* ssid = "WWW_IGROMI_COM";        
const char* password = "wifiiGromi12";
const char* mqtt_server_Local = "192.168.1.200";
  
const char* mqtt_server_THB = "iot.igromi.com";
const char* mqtt_id_local = "bridge_id";
const char* mqtt_user = "igromi";
const char* mqtt_pass = "imagina12";

const char* topic="localTopic";

//Definicion MQTT

WiFiClient espClient_Local;
PubSubClient client_Local(espClient_Local);

//Variables varias
String str;
String str2;
char payload[200];

//variables para el loop de muestreos
String str_loop[arraySize];
int indice_loop=0;
int z=0;

unsigned long previous_time = 0;
unsigned long previous_time_THB = 0; 
const long interval = 60000;

int contador1=0;
int contador2=0;


boolean estado=false;
boolean estado2=false;

boolean estadoB=false;
boolean estadoB2=false;


//IO pines
const int pinIN1=36;
const int pinIN2=39;
const int pinIN3=27;
const int pinIN4=14;

const int pinADC1=32;
const int pinADC2=33;
const int pinADC3=34;
const int pinADC4=35;


int ciclos=0;
int id_msg;
    
void setup() {
  
  Serial.begin(115200);

  int value = 127;  // 255 = 10V
  dacWrite(DAC1, value);
  dacWrite(DAC2, value);

  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch 
  
  pinMode(pinIN1, INPUT);
  pinMode(pinIN2, INPUT);
  pinMode(pinIN3, INPUT);
  pinMode(pinIN4, INPUT);

  

  client_Local.setServer(mqtt_server_Local, 1883);
  initServerConnection();

  delay(1000);
  Serial.println("TERMINO SETUP");
  Serial.println("");

}

void loop() {
  
  // Do work here
  estado=digitalRead(pinIN1);
  estado2=digitalRead(pinIN2);
  
  delay(25); 
  
  estadoB=digitalRead(pinIN1);
  estadoB2=digitalRead(pinIN2);

  if ((estado == false) && (estadoB == true))
    {
    contador1++;
    Serial.print("contador: ");
    Serial.println(contador1);
    };

  if ((estado2 == false) &&( estadoB2 == true))
    {
    contador2++;
    Serial.print("contador2: ");
    Serial.println(contador2);
    
    };

    MuestreoTHB();
    esp_task_wdt_reset();  

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
    if (client_Local.connect(mqtt_id_local,mqtt_user,mqtt_pass)) {
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


//Conexion inicial THB 
void initServerConnection() {
  
   setupWifi();
   //Verifico conexión Local
  if (!client_Local.connected()) {
    reconnect_Local();
  };
}

//Rutina de envio de datos a THB 60s y verifico la suscripción 1s
void MuestreoTHB(){

  unsigned long current_time = millis();
  if (current_time - previous_time >= interval) {
     
     previous_time = current_time;
     Serial.println("");
     Serial.println("Inicio de muestreo");
     Serial.print("######## Tiempo inicial: "); 
     Serial.println(current_time);
    
     initServerConnection();

     id_msg=random(1000000000, 9999999999);

     str= "{\"id_msg\":\""+String(id_msg)+"\",\"id\":\""+String(id)+"\",\"contador1\":\""+String(contador1)+"\",\"contador2\":\""+String(contador2)+"\",\"Muestreos\":\""+String(ciclos)+"\"}"; 
     str_loop[indice_loop]=str;
  
    if(indice_loop==(arraySize-1) )
        {
          indice_loop=-1;        
        };

    indice_loop=indice_loop+1;

    for (z=0;z < arraySize  ;z++)
        {
        str_loop[z].toCharArray(payload,200);
        Serial.print("Enviando array indice:");
        Serial.println(z); 
        Serial.println(payload);
        client_Local.publish(topic,payload,QoS);
         
        };

     Serial.println("");
     Serial.print("Indice array: ");
     Serial.println(indice_loop);
     Serial.println("");
        
     contador1=0;
     contador2=0;

     
     Serial.print("Muestreo: "); 
     Serial.println(ciclos);
     ciclos++;
     
     current_time = millis();
     Serial.print("######## Tiempo final: "); 
     Serial.println(current_time);
     Serial.println("");

     
  };

 };
