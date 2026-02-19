#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_Sensor.h>

// Sensor objects
Adafruit_BMP280 bmp;   // BMP280 object
Adafruit_AHTX0 aht;    // AHT10/AHT20 object

// Optional: simple smoothing
float temperatureBuffer[5] = {0};
float humidityBuffer[5] = {0};
int bufferIndex = 0;

// LED pin
const int ledPin = LED_BUILTIN;

void setup() {
  Serial.begin(115200);
  Serial.println(F("AHT20 + BMP280 Test"));

  pinMode(ledPin, OUTPUT); // Built-in LED as output

  // Initialize AHT20
  if (!aht.begin()) {
    Serial.println(F("Failed to find AHT10/AHT20 sensor!"));
    while (1) delay(10);
  }
  Serial.println(F("AHT sensor OK"));

  // Initialize BMP280
  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1) delay(10);
  }

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );
}

float average(float *arr, int size) {
  float sum = 0;
  for (int i = 0; i < size; i++) sum += arr[i];
  return sum / size;
}

void loop() {
  // Read AHT20 sensor using getEvent
  sensors_event_t humidityEvent, tempEvent;
  aht.getEvent(&humidityEvent, &tempEvent);

  float temperature = tempEvent.temperature; // °C
  float humidity = humidityEvent.relative_humidity; // %RH
  float pressure = bmp.readPressure() / 100.0F; // hPa

  // Store readings in buffer for smoothing
  temperatureBuffer[bufferIndex] = temperature;
  humidityBuffer[bufferIndex] = humidity;
  bufferIndex = (bufferIndex + 1) % 5;

  float smoothTemp = average(temperatureBuffer, 5);
  float smoothHum = average(humidityBuffer, 5);

  // Blink built-in LED
  digitalWrite(ledPin, LOW);
  delay(100); // LED on for 100ms
  digitalWrite(ledPin, HIGH);

  // Print sensor data
  Serial.print("Temperature: "); Serial.print(smoothTemp, 2); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(smoothHum, 2); Serial.println(" %RH");
  Serial.print("Pressure: "); Serial.print(pressure, 2); Serial.println(" hPa");

  // Real-life status messages
  if (smoothTemp < 18) Serial.println("Temperature Status: Cold");
  else if (smoothTemp <= 26) Serial.println("Temperature Status: Comfortable");
  else Serial.println("Temperature Status: Hot");

  if (smoothHum < 30) Serial.println("Humidity Status: Dry");
  else if (smoothHum <= 60) Serial.println("Humidity Status: Comfortable");
  else Serial.println("Humidity Status: Humid");

  if (pressure < 1000) Serial.println("Pressure Status: Low / Possible Storm");
  else if (pressure <= 1020) Serial.println("Pressure Status: Normal");
  else Serial.println("Pressure Status: High / Clear");

  Serial.println("-----------------------------");

  delay(3000); // Update every 3 seconds
}
