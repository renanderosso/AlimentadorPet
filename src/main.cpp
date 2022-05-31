#define BLYNK_PRINT Serial
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <WidgetRTC.h> 
#include <TimeLib.h> 
#ifdef __AVR__
#include <avr/power.h>
#endif

BlynkTimer timer;
WidgetRTC rtc; 

unsigned long startMillis;                              /* start counting time for display refresh*/
unsigned long currentMillis;                            /* current counting time for display refresh */
const unsigned long period = 1000;                      // refresh every X seconds (in seconds) Default 60000 = 1 minute 

//---- Pinos de controle -----
#define STP 27 // Avanço do passo
#define DIR 26 // Direção do passo
#define ENA 25 // Função ENABLE
//---- Variáveis de controle ----

char auth[] = "pw5i09G8QO__X1bx2Pk7SwjvSfi2AmQz";
char ssid[] = "Rosso-Intel";
char pass[] = "055A64F7";

bool set_motor = false;
int PPR = 3000;
int passo = 0;    // passos
int temp = 200;   // tempo entre os passos

BLYNK_CONNECTED()              /* When Blynk server is connected, initiate Real Time Clock function */
{ rtc.begin(); }

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  setSyncInterval(1);                                           /* Synchronise or read time from the Blynk Server every 1 second */
  while (Blynk.connect() == false ) {}                          /* If the Blynk Server not yet connected to nodeMCU, keep waiting here */
  setSyncInterval(10*60);                                       /* After successful login, change Synchornise Time reading for every 10 minute (Do not need to always check for the time)*/
  
    startMillis = millis();                                       /* Start record initial time for display refresh */
    pinMode(STP, OUTPUT);
    pinMode(ENA, OUTPUT);
    pinMode(DIR, OUTPUT);
    digitalWrite(DIR, LOW);
    digitalWrite(STP, LOW);
    Serial.println("End");
}


BLYNK_WRITE(V4) //Button Widget is writing to pin V4
{
  set_motor = param.asInt();
  Serial.println("Motor ->" + String(set_motor));
}

void HR()
{ // Sentido Horário
  passo = 0;
  Serial.println("Sentido - Horario");
  digitalWrite(DIR, HIGH);
}

void ciclo()
{
  HR();
  for (passo = 0; PPR > passo; passo++)
  { // Enquanto PPR for maior que passo
    // Avança o passo
    digitalWrite(STP, LOW);
    delayMicroseconds(temp); //Tempo em Microseconds
    digitalWrite(STP, HIGH);
    delayMicroseconds(temp);
    yield();
  }
  passo = 0; // valor de passso muda pra 0
  // delay(1000);
  // Inicia o Sentido Anti-horário
  passo = 0;
}

void loop() {
  
  Blynk.run();                                                                    /* allow the communication between Blynk server and Node MCU*/ 
  timer.run();                                                                    /* allow the Blynk timer to keep counting */

  /* 2- Display refresh*/

  currentMillis = millis();                                                       /* Keep counting time for display refresh */
  if(currentMillis - startMillis > period)                                        /* For every 1 second, run the set of code*/
  {
    String currentTime = String(hour()) + ":" + minute() + ":" + second();        /* Define "currentTime" by combining hour, minute and second */
    String currentDate = String(day()) + " " + month() + " " + year();            /* Define "currentDate" by combining day, month, and year */
    Serial.print("Current time: ");                                               /* Display values on Serial Monitor */
    Serial.print(currentTime);
    Serial.print(" ");
    Serial.print(currentDate);
    Serial.println();

    Blynk.virtualWrite(V1, currentTime); ////ASIAASIUASIUBAS                                         /* Send Time parameters to Virtual Pin V1 on Blynk App */
    Blynk.virtualWrite(V2, currentDate);                                          /* Send Date parameters to Virtual Pin V2 on Blynk App */
    
    int getSecond = second();                                                     /* Assign "getHour" as the hour now */
    if (getSecond > 30)
    { digitalWrite(2,HIGH);}                                                      /* Turn OFF the LED if seconds count is more than 30 */
    if (getSecond < 30)
    { digitalWrite(2,LOW); }                                                      /* Turn ON the LED if the seconds count is less than 30 */                                                 
    startMillis = millis();                                                       /* Reset time for the next counting cycle */
  }
////////////////////////
    if (set_motor)
  { // Se receber 1
    // set_motor = false;
    Serial.println("Resolução 3000 passo");
    ciclo();
  }
}