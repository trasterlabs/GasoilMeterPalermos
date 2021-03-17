/**
 *@file Gasoil_V3.ino
 *@brief Control del consumo de Gasoil
 *
 *@authorDr.Zhao Zhang
 *@note: Se ha utilizado la siguiente configuración hardware para su uso: Wemos D1 + Sensor de corriente efecto Hall + Servo indicador de litros + Relé SSR avisador de reserva + Thingspeak + OTA
 */

#include <ESP8266WiFi.h>
#include <Timer.h>
#include <Servo.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>



//----------------------------------------------------
// 2. Conexiones
//----------------------------------------------------

#define HALL A0      // ¿Qué es hall?
#define SSR D4       // ¿Qué es ssr?
#define ServoPIN D9  // ¿Qué es ServoPIN?

//----------------------------------------------------
// 3. Variables y Comandos
//----------------------------------------------------

#ifndef STASSID
#define STASSID "SSID"                            // Poner el nombre de la red WiFi entre las comillas   
#define STAPSK  "PASS"                         // Poner la contraseña de la red WiFi entre las comillas 
#endif

String apiKey = "API-KEY";                  // Poner el API KEY del canal de Thingspeak entre las comillas
const char* server = "api.thingspeak.com";

bool monitorserie = true;                            // Poner true para hacer debug por monitor serie o false para desactivarlo
int litros_medidos = 2975;                          // Actualizar cantidad de gasoil cuando se rellene el depósito
/**
 * Parámetro de la bomba medido en litros por segundo
 */
const float l_seg = 0.078;

const char* ssid = STASSID;           
const char* password = STAPSK;            
const char* host = "OTA-LEDS";
int litros_restantes = litros_medidos;
int litros_consumidos;
int angle = 170;
int i = 0;
float corriente = 0.0;
unsigned long tiempoinicio = 0;
unsigned long tiempofin = 0;
unsigned long tiempobomba = 0;
unsigned long tiempoacumulado = 0;


//----------------------------------------------------
// 4. Objetos
//----------------------------------------------------

WiFiClient client;
Timer tiempo;
Servo myServo;



//====================================================
// SETUP
//====================================================


void setup() {
  
    if (monitorserie == true)
     {
    Serial.begin(9600);                             // Poner monitor serie a 9600 baudios
    delay(10);    
     }


  WiFi.mode(WIFI_STA);
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onError([](ota_error_t error) 
      {
          (void)error;
          ESP.restart();
      });

  ArduinoOTA.begin();
  
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, LOW); 

    myServo.attach(ServoPIN);
    myServo.write(angle);
    delay (500);
    
    Thingspeak();
    
    
    tiempo.every (60000, Thingspeak);
    }



//====================================================
// LOOP
//====================================================


void loop() {

  ArduinoOTA.handle();
  
tiempo.update();

corriente = 0;

    for (i=0; i<100; i++)
       {
          corriente = analogRead (HALL) + corriente;
       }
                                
                    
                       corriente = corriente/100;

    
    
    if (corriente < 1000 )
      {

          tiempoinicio = millis();
   
      
              while (corriente < 1000 )
                  {

                       ESP.wdtFeed();
                
                       corriente = 0;
                       
                
                               for (i=0; i<100; i++)
                                {
                                  corriente = analogRead (HALL) + corriente;
                                }
                                
                    
                       corriente = corriente/100;
            
      
                               if (monitorserie == true)
                                  {
                                      Serial.print ("A0 ACTIVADO = ");
                                      Serial.println (corriente);
                                  }
                    }
         

              tiempofin = millis();
              
              tiempobomba = tiempofin - tiempoinicio;  
            
              tiempoacumulado = tiempoacumulado + tiempobomba;
  
       }


  litros_consumidos = (tiempoacumulado * l_seg) / 1000;

  litros_restantes = litros_medidos - litros_consumidos;

  

      angle = map(litros_restantes,0,4000,170,-5);
      myServo.write(angle);
      






          if (litros_restantes < 1000)
                    {
                        digitalWrite(SSR, (millis() / 1000) % 2);
                    }
            
                        else 
                          {
                              digitalWrite (SSR, LOW);
                          }






    if (monitorserie == true)
        {
            Serial.print ("Tiempo inicio = ");
            Serial.println (tiempoinicio);
            Serial.print ("Tiempo fin = ");
            Serial.println (tiempofin);
            Serial.print ("Tiempo bomba = ");
            Serial.println (tiempobomba);
            Serial.print ("Tiempo acumulado = ");
            Serial.println (tiempoacumulado);
            Serial.print ("MILLIS = ");
            Serial.println (millis());
            Serial.print ("A0 = ");
            Serial.println (corriente);
            Serial.print ("Litros consumidos = ");
            Serial.println (litros_consumidos);
            Serial.print ("Litros restantes = ");
            Serial.println (litros_restantes);
            Serial.print ("Angulo = ");
            Serial.print (angle);
            Serial.println(" º");
        }


      
                



}




//====================================================
// Función conectar a la red WiFi
//====================================================

void conectawifi()
  {
  
    WiFi.begin(ssid, password);

        if (monitorserie == true)
            {
                Serial.print("Conectando a WiFi ");
 
                while (WiFi.status() != WL_CONNECTED) 
                    {
                        Serial.print(".");
                        delay (100);
                    }

              Serial.println(" ");
              Serial.print("Conexion satisfactoria a ");
              Serial.println(ssid);
              Serial.println(" ");


            }
   }



//====================================================
// Función enviar datos a Thingspeak
//====================================================

void Thingspeak()
    {

  
          if  (WiFi.status() != WL_CONNECTED) 
              {
                conectawifi();
              }


  if (client.connect(server,80)) 
        {
            String postStr = apiKey;

            postStr +="&field1=";                              // El dato de litros restantes lo envía al campo 1 del canal de Thingspeak
            postStr += String(litros_restantes);
            postStr +="&field2=";                              // El dato de temperatura objetivo lo envía al campo 2 del canal de Thingspeak
            postStr += String(tiempoacumulado);
            postStr +="&field3=";                              // El dato del estado del termostato lo envía al campo 3 del canal de Thingspeak
            postStr += String(tiempobomba);
            postStr +="&field4=";                              // El dato del funcionamiento lo envía al campo 4 del canal de Thingspeak
            postStr += String(millis());
            postStr += "\r\n\r\n";



                   if (monitorserie == true)
                      {
                    Serial.println("ENVIANDO DATOS A THINGSPEAK");
                      } 

 
            client.print("POST /update HTTP/1.1\n");
            client.print("Host: api.thingspeak.com\n");
            client.print("Connection: close\n");
            client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
            client.print("Content-Type: application/x-www-form-urlencoded\n");
            client.print("Content-Length: ");
            client.print(postStr.length());
            client.print("\n\n");
            client.print(postStr);
           
         } 
            delay(1000);
            client.stop();

    }
