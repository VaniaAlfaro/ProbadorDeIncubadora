//------------------Librerias-------------------------------------
#include <WiFi.h>      //Modulo de wifi
#include <DHT.h>       //Modulo de HUMEDAD
#include <Wire.h>
#include <Firebase_ESP_Client.h>  //Conexión a la Base de Datos
#include "addons/TokenHelper.h" 
#include "addons/RTDBHelper.h"

//------------------Servidor Web en puerto 80---------------------
WiFiServer server(80);

//------------------Variables del Sesnor Humedad-----------------------------
#define DHTPIN 13 // Definimos el pin digital 
#define DHTTYPE DHT11 // Dependiendo del tipo de sensor
DHT dht(DHTPIN, DHTTYPE); // Inicializamos el sensor DHT11

//---------------------Credenciales de WiFi-----------------------
const char* ssid     = "VKAN";
const char* password = "123456789";

#define USER_EMAIL "andre@biogem.com"
#define USER_PASSWORD "123456"

#define API_KEY "AIzaSyBoRR-PgB0kB8_GLw9_GdhZRLl2SdBiqyQ"
#define DATABASE_URL "https://monitorincubadora-97450-default-rtdb.firebaseio.com/"

//---------------------VARIABLES GLOBALES-------------------------
int contconexion = 0;
String header; //Varible para guardar el HTTP request

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

int lm35;
float temperatura = 0.0;
int humedad = 0;

bool alarmaTempBaja = false;
bool alarmaTempAlta = false;

bool alarmaHumedadBaja = false;
bool alarmaHumedadAlta = false;

//-----------HTML--------
/*String paginaInicio = "<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset='utf-8' />"
"<META HTTP-EQUIV='Refresh' CONTENT='1'>"
"<title>Servidor Web ESP32</title>"
"</head>"
"<body>"
"<center>"
"<h3>Monitor de Incubadora</h3>";
//Muestra el final de la pagina
String paginaFin = "</center>"
"</body>"
"</html>";*/
    String html = "<html><body><h1>Datos en tiempo real</h1><div id='chart_div'></div></body>";
    html += "<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>";
    html += "<script type='text/javascript'>";
    html += "google.charts.load('current', {'packages':['corechart']});";
    html += "google.charts.setOnLoadCallback(drawChart);";
    html += "function drawChart() {";
    html += "var data = new google.visualization.DataTable();";
    html += "data.addColumn('number', 'Tiempo');";
    html += "data.addColumn('number', 'Dato');";
    html += "var chart = new google.visualization.LineChart(document.getElementById('chart_div'));";
    html += "chart.draw(data, {width: 800, height: 400});";
    html += "}";
    html += "</script></html>";
//---------------------------SETUP---------------------------------
void setup() 
{
  Serial.begin(115200);
  // Conexión Modulo de Wifi 
  dht.begin();
  Serial.println("");
  
  // Conexión WIFI
  WiFi.begin(ssid, password);
  //Cuenta hasta 50 si no se puede conectar lo cancela
  while (WiFi.status() != WL_CONNECTED and contconexion <50) 
  { 
    ++contconexion;
    delay(500);
    Serial.print(".");
  }
  
  if (contconexion <50) 
  {      
      Serial.println("");
      Serial.println("WiFi conectado");
      Serial.println(WiFi.localIP());
      server.begin(); // iniciamos el servidor
  }
  else 
  { 
      Serial.println("");
      Serial.println("Error de conexion");
  }
  
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  
  if (Firebase.signUp(&config, &auth, "",""))
  {
    Serial.println("signUp OK");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

//----------------------------LOOP---------------------------------
void loop()
{
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
//--------------------------HUMEDAD y TEMPERATURA LOOP---------------------------
    lm35 = analogRead(34); //Declaracion de las variables. 
    Serial.println (lm35);
    temperatura = ((lm35*5000.0) / 1023 / 10);
    humedad = dht.readHumidity();   // Leemos la humedad relativa
    //temperatura = dht.readTemperature(); // Leemos la temperatura en grados centígrados (por defecto)
    
    if (temperatura <= 36)
    {
      alarmaTempBaja = true;
    }
    if (temperatura >= 36.5)
    {
      alarmaTempAlta = true;
    }
    
    if (humedad <= 30)
    {
      alarmaHumedadBaja = true;
    }
    if (humedad >= 95)
    {
      alarmaHumedadAlta = true;
    }

//--------------------------FIREBASE DATA LOOP---------------------------
    //HUMEDAD
    if (Firebase.RTDB.setInt(&fbdo, "Sensor/humedad", humedad))
    {
     Serial.println();
     Serial.print(humedad);
     Serial.print("-Guardado en: " + fbdo.dataPath());
     Serial.println("("+ fbdo.dataType()+")");
    }
    else 
    {
      Serial.println("FAILED" +  fbdo.errorReason());
    }
    //TEMPERATURA
    if (Firebase.RTDB.setInt(&fbdo, "Sensor/temperatura", temperatura))
    {
     Serial.println();
     Serial.print(temperatura);
     Serial.print("-Guardado en: " + fbdo.dataPath());
     Serial.println("("+ fbdo.dataType()+")");
    }
    else 
    {
      Serial.println("FAILED" + fbdo.errorReason());
    }
    
    //ALARMA TEMPERATURA
    if (Firebase.RTDB.setInt(&fbdo, "Alarma/Temperatura Baja", alarmaTempBaja))
    {
     Serial.println();
     Serial.print(alarmaTempBaja);
     Serial.print("-Guardado en: " + fbdo.dataPath());
     Serial.println("("+ fbdo.dataType()+")");
    }
    else 
    {
      Serial.println("FAILED" + fbdo.errorReason());
    }
    if (Firebase.RTDB.setInt(&fbdo, "Alarma/Temperatura Alta", alarmaTempAlta))
    {
     Serial.println();
     Serial.print(alarmaTempAlta);
     Serial.print("-Guardado en: " + fbdo.dataPath());
     Serial.println("("+ fbdo.dataType()+")");
    }
    else 
    {
      Serial.println("FAILED" + fbdo.errorReason());
    }
    //ALARMA HUMEDAD
    if (Firebase.RTDB.setInt(&fbdo, "Alarma/Humedad Baja", alarmaHumedadBaja))
    {
     Serial.println();
     Serial.print(alarmaHumedadBaja);
     Serial.print("-Guardado en: " + fbdo.dataPath());
     Serial.println("("+ fbdo.dataType()+")");
    }
    else 
    {
      Serial.println("FAILED" + fbdo.errorReason());
    }
    if (Firebase.RTDB.setInt(&fbdo, "Alarma/Humedad Alta", alarmaHumedadAlta))
    {
     Serial.println();
     Serial.print(alarmaHumedadAlta);
     Serial.print("-Guardado en: " + fbdo.dataPath());
     Serial.println("("+ fbdo.dataType()+")");
    }
    else 
    {
      Serial.println("FAILED" + fbdo.errorReason());
    }
    
    WiFiClient client = server.available();   // Escucha a los clientes entrantes
    client.println(paginaInicio + String(temperatura) + paginaFin);
  }   
}
