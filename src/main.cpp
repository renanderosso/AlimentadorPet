#define BLYNK_PRINT Serial
#include <Arduino.h>
#include "esp_camera.h"
#include "sensor.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <WidgetRTC.h>
#include <TimeLib.h>
#include <DHT.h>
#include <Ticker.h>
//#include <Adafruit_Sensor.h>
#include "camera_index.h"
#ifdef __AVR__
#include <avr/power.h>
#endif

#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

BlynkTimer timer;
WidgetRTC rtc;

#define PHOTO 14 // ESP32 CAM
#define LED 4

Ticker humildade_timer;

const char *ssid = "Rosso";
const char *password = "055A64F7";

String local_IP;
int count = 0;
void startCameraServer();

void takePhoto()
{
  digitalWrite(LED, HIGH);
  delay(200);
  uint32_t randomNum = random(50000);
  Serial.println("http://" + local_IP + "/capture?_cb=" + (String)randomNum);
  Blynk.setProperty(V1, "urls", "http://" + local_IP + "/capture?_cb=" + (String)randomNum); // ESP32 CAM 1
  digitalWrite(LED, LOW);
  delay(1000);
}

unsigned long startMillis;         /* start counting time for display refresh*/
unsigned long currentMillis;       /* current counting time for display refresh */
const unsigned long period = 1000; // refresh every X seconds (in seconds) Default 60000 = 1 minute

#define DHTPIN 16     // pino que estamos conectado
#define DHTTYPE DHT11 // DHT 11

//---- Pinos de controle -----
#define STP 2  // Avanço do passo laranja
#define DIR 15 // Direção do passo vermelho
#define ENA 13 // Função ENABLE amarelo
//---- Variáveis de controle ----

char auth[] = "pw5i09G8QO__X1bx2Pk7SwjvSfi2AmQz";
char rede[] = "Rosso";
char pass[] = "055A64F7";
DHT dht(DHTPIN, DHTTYPE);

bool set_motor = false;
int PPR = 3000;
int passo = 0;  // passos
int temp = 200; // tempo entre os passos

BLYNK_CONNECTED() /* When Blynk server is connected, initiate Real Time Clock function */
{
  rtc.begin();
}

void humildade_timer_handle()
{
  Serial.println("Atualizando humildade");

  // A leitura da temperatura e umidade pode levar 250ms!
  // O atraso do sensor pode chegar a 2 segundos.
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // testa se retorno é valido, caso contrário algo está errado.
  if (isnan(t) || isnan(h))
  {
    // Serial.println("Failed to read from DHT");
  }
  else
  {
    Blynk.virtualWrite(V3, t);
    Blynk.virtualWrite(V5, h);
  }
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  pinMode(LED, OUTPUT);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);       // flip it back
    s->set_brightness(s, 1);  // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }

  s->set_framesize(s, FRAMESIZE_QVGA);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Blynk.begin(auth, rede, pass);
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  local_IP = WiFi.localIP().toString();
  Serial.println("' to connect");

  setSyncInterval(1); /* Synchronise or read time from the Blynk Server every 1 second */
  while (Blynk.connect() == false)
  {
  }                         /* If the Blynk Server not yet connected to nodeMCU, keep waiting here */
  setSyncInterval(10 * 60); /* After successful login, change Synchornise Time reading for every 10 minute (Do not need to always check for the time)*/

  startMillis = millis(); /* Start record initial time for display refresh */
  pinMode(STP, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(DIR, OUTPUT);
  digitalWrite(DIR, LOW);
  digitalWrite(STP, LOW);

  humildade_timer.attach_ms(5000, humildade_timer_handle);
  Serial.println("End");
}

BLYNK_WRITE(V4) // Button Widget is writing to pin V4
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
    delayMicroseconds(temp); // Tempo em Microseconds
    digitalWrite(STP, HIGH);
    delayMicroseconds(temp);
    yield();
  }
  passo = 0; // valor de passso muda pra 0
}

void loop()
{

  Blynk.run(); /* allow the communication between Blynk server and Node MCU*/
  timer.run(); /* allow the Blynk timer to keep counting */

  /* 2- Display refresh*/

  if (digitalRead(PHOTO) == HIGH)
  {
    takePhoto();
  }

  currentMillis = millis();                 /* Keep counting time for display refresh */
  if (currentMillis - startMillis > period) /* For every 1 second, run the set of code*/
  {
    String currentTime = String(hour()) + ":" + minute() + ":" + second(); /* Define "currentTime" by combining hour, minute and second */
    String currentDate = String(day()) + " " + month() + " " + year();     /* Define "currentDate" by combining day, month, and year */
    Serial.print("Current time: ");                                        /* Display values on Serial Monitor */
    Serial.print(currentTime);
    Serial.print(" ");
    Serial.print(currentDate);
    Serial.println();

    Blynk.virtualWrite(V6, currentTime); /* Send Time parameters to Virtual Pin V6 on Blynk App */
    Blynk.virtualWrite(V2, currentDate); /* Send Date parameters to Virtual Pin V2 on Blynk App */

    int getSecond = second(); /* Assign "getHour" as the hour now */
    if (getSecond > 30)
    {
      digitalWrite(2, HIGH);
    } /* Turn OFF the LED if seconds count is more than 30 */
    if (getSecond < 30)
    {
      digitalWrite(2, LOW);
    }                       /* Turn ON the LED if the seconds count is less than 30 */
    startMillis = millis(); /* Reset time for the next counting cycle */
  }

  if (set_motor)
  { // Se receber 1
    // set_motor = false;
    Serial.println("Resolução 3000 passo");
    ciclo();
  }
  // // A leitura da temperatura e umidade pode levar 250ms!
  // // O atraso do sensor pode chegar a 2 segundos.
  // float h = dht.readHumidity();
  // float t = dht.readTemperature();
  // // testa se retorno é valido, caso contrário algo está errado.
  // if (isnan(t) || isnan(h))
  // {
  //   // Serial.println("Failed to read from DHT");
  // }
  // else
  // {
  //   Blynk.virtualWrite(V3, t);
  //   Blynk.virtualWrite(V5, h);
  // }
}