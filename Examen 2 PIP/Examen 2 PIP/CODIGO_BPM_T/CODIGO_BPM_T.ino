#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Configuración pantalla OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Pines sensores ---
#define HEARTBEAT_SENSOR_PIN 2  // KY-039 (mejor A0 o GPIO34)
#define ONE_WIRE_BUS 4          // DS18B20

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define samp_siz 4

float reads[samp_siz], sum;
float last, reader, start;
float before = 0;
int n = 0;
long int ptr = 0;

long int last_beat = 0;   // Último latido detectado
float bpm = 0.0;          // BPM calculado

int threshold = 500;      // Umbral mínimo para detectar un latido

// Filtro pasa bajos
float factor = 0.75;
float maximo = 0.0;
int minimoEntreLatidos = 300;
float valorAnterior = 500;
int latidos = 0;

void setup() {
  Serial.begin(115200);
  sensors.begin();

  pinMode(13, OUTPUT);

  for (int i = 0; i < samp_siz; i++) reads[i] = 0;
  sum = 0;
  ptr = 0;
  Serial.println("Iniciando mediciones");

  // --- Inicializar pantalla OLED ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("No se encontró pantalla OLED");
    while (true); // Detener ejecución si falla OLED
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();
  delay(1000);
}

void loop() {
  n = 0;
  start = millis();
  reader = 0.;
  do {
    reader += analogRead(HEARTBEAT_SENSOR_PIN);
    n++;
  } while (millis() < start + 20);
  reader /= n;

  sum -= reads[ptr];
  sum += reader;
  reads[ptr] = reader;
  last = sum / samp_siz;

  float valorFiltrado = factor * valorAnterior + (1 - factor) * reader;
  float cambio = valorFiltrado - valorAnterior;
  valorAnterior = valorFiltrado;

  if ((cambio >= maximo) && (millis() > last_beat + minimoEntreLatidos)) {
    maximo = cambio;
    digitalWrite(13, HIGH);
    last_beat = millis();
    latidos++;
  } else {
    digitalWrite(13, LOW);
  }

  maximo = maximo * 0.97;

  if (millis() >= last_beat + 1000) {
    bpm = latidos * 4;
    latidos = 0;
    last_beat = millis();
  }

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  Serial.print("Ritmo cardíaco: ");
  Serial.print(last);
  Serial.print(" BPM: ");
  Serial.print(60000 / last);
  Serial.print(" Temperatura: ");
  Serial.println(tempC);

  // --- Mostrar en pantalla OLED ---
  display.clearDisplay();

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("BPM:");
  display.println((int)60000/last);

  display.setTextSize(2);
  display.setCursor(0, 30);
  display.print("T:");
  display.print(tempC, 1);
  display.cp437(true);
  display.write(167); // símbolo °
  display.print("C");

  display.display();

  delay(1000);
}
