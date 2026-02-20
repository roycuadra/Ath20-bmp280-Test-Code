#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_Sensor.h>

// Sensors
Adafruit_BMP280 bmp;
Adafruit_AHTX0 aht;

// Smoothing buffers
float temperatureBuffer[5] = {22,22,22,22,22};
float humidityBuffer[5] = {50,50,50,50,50};
int bufferIndex = 0;

// LED
const int ledPin = LED_BUILTIN;

// NO PASSWORD - Open WiFi
const char* ssid = "WeatherStation";

ESP8266WebServer server(80);
DNSServer dnsServer;
WebSocketsServer webSocket = WebSocketsServer(81);

// Calculate average
float average(float *arr, int size) {
  float sum = 0;
  for (int i = 0; i < size; i++) sum += arr[i];
  return sum / size;
}

// Status functions
String getTempStatus(float temp) {
  if (temp < 18) return "Cold ❄️";
  else if (temp <= 26) return "Comfortable 😊";
  else return "Hot 🔥";
}

String getHumStatus(float hum) {
  if (hum < 30) return "Dry 🏜️";
  else if (hum <= 60) return "Comfortable 😊";
  else return "Humid 💧";
}

String getPresStatus(float pres) {
  if (pres < 1000) return "Low ⛈️ Storm";
  else if (pres <= 1020) return "Normal ☁️";
  else return "High 🌤️ Clear";
}

// IMPROVED: Accurate Real Feel calculation for Celsius
float calculateRealFeel(float tempC, float humidity) {
  // Convert to Fahrenheit for standard NOAA formulas
  float tempF = tempC * 9.0/5.0 + 32.0;
  
  // Heat Index (when hot and humid)
  float heatIndex;
  if (tempC >= 27.0 && humidity >= 40) {
    float hi = -42.379 + 2.04901523*tempF + 10.14333127*humidity 
             - 0.22475541*tempF*humidity - 6.83783e-3*pow(tempF,2)
             - 5.481717e-2*pow(humidity,2) + 1.22874e-3*pow(tempF,2)*humidity
             + 8.5282e-4*tempF*pow(humidity,2) - 1.99e-6*pow(tempF,2)*pow(humidity,2);
    heatIndex = hi;
  } else {
    heatIndex = tempF;
  }
  
  // Wind Chill (when cold) - simplified, assumes indoor/no wind
  float windChill = tempF;
  if (tempC < 10.0) {
    windChill = 35.74 + 0.6215*tempF - 35.75*pow(3.6, 0.16) + 0.4275*tempF*pow(3.6, 0.16);
  }
  
  // Return the more extreme value (what you "feel")
  float realFeelF = max(heatIndex, windChill);
  
  // Convert back to Celsius
  return (realFeelF - 32.0) * 5.0/9.0;
}

String getRealFeelStatus(float realFeel, float temp, float humidity) {
  float diff = realFeel - temp;
  
  if (realFeel < 10) return "Freezing ❄️❄️";
  else if (realFeel < 16) return "Very Cold 🥶";
  else if (realFeel <= 24) return "Comfortable 😊";
  else if (realFeel <= 30) return "Warm 🌡️";
  else if (realFeel <= 36) return "Hot 🔥";
  else return "Extreme 🔥🔥";
}

String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP8266 Pro Weather Station</title>
  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;600&display=swap" rel="stylesheet">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Poppins', sans-serif; 
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
      min-height: 100vh; 
      display: flex; 
      flex-direction: column; 
      align-items: center; 
      justify-content: center; 
      padding: 20px; 
      color: #333;
    }
    .header { 
      color: white; 
      font-size: 2.5rem; 
      font-weight: 600; 
      margin-bottom: 10px; 
      text-shadow: 0 2px 10px rgba(0,0,0,0.3);
      animation: fadeInDown 1s ease;
    }
    .subtitle {
      color: rgba(255,255,255,0.9);
      font-size: 1.1rem;
      font-weight: 300;
      margin-bottom: 40px;
      animation: fadeInDown 1s ease 0.3s both;
    }
    .dashboard { 
      display: grid; 
      grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); 
      gap: 25px; 
      max-width: 1200px; 
      width: 100%;
    }
    .card { 
      background: rgba(255, 255, 255, 0.1); 
      backdrop-filter: blur(20px); 
      border-radius: 25px; 
      padding: 30px 25px; 
      text-align: center; 
      box-shadow: 0 8px 32px rgba(31, 38, 135, 0.37);
      border: 1px solid rgba(255, 255, 255, 0.18);
      transition: all 0.3s ease;
      animation: fadeInUp 1s ease;
    }
    .card:hover { 
      transform: translateY(-10px); 
      box-shadow: 0 20px 40px rgba(0,0,0,0.2);
    }
    .value { 
      font-size: 3.5rem; 
      font-weight: 600; 
      color: white; 
      margin-bottom: 10px; 
      text-shadow: 0 2px 10px rgba(0,0,0,0.3);
    }
    .label { 
      font-size: 1.1rem; 
      color: rgba(255,255,255,0.9); 
      font-weight: 400;
      margin-bottom: 15px;
    }
    .icon { font-size: 3rem; margin-bottom: 15px; opacity: 0.9; }
    .status { 
      margin-top: 15px; 
      padding: 12px 20px; 
      background: rgba(255,255,255,0.25); 
      border-radius: 25px; 
      font-size: 1rem; 
      color: white;
      font-weight: 500;
      box-shadow: 0 4px 15px rgba(0,0,0,0.1);
    }
    @keyframes fadeInDown { from { opacity: 0; transform: translateY(-30px); } }
    @keyframes fadeInUp { from { opacity: 0; transform: translateY(30px); } }
    @media (max-width: 768px) { 
      .header { font-size: 2rem; }
      .subtitle { font-size: 1rem; }
      .value { font-size: 2.5rem; }
      .dashboard { grid-template-columns: 1fr; gap: 20px; }
    }
  </style>
</head>
<body>
  <h1 class="header">🌤️ Smart Weather Station Pro</h1>
  <div class="subtitle">Real-time monitoring + Accurate Real Feel</div>
  <div class="dashboard">
    <div class="card">
      <div class="icon">🌡️</div>
      <div class="value" id="temp">--</div>
      <div class="label">Temperature</div>
      <div class="status" id="temp-status">Initializing...</div>
    </div>
    <div class="card">
      <div class="icon">💧</div>
      <div class="value" id="hum">--</div>
      <div class="label">Humidity</div>
      <div class="status" id="hum-status">Initializing...</div>
    </div>
    <div class="card">
      <div class="icon">📊</div>
      <div class="value" id="pres">--</div>
      <div class="label">Pressure</div>
      <div class="status" id="pres-status">Initializing...</div>
    </div>
    <div class="card">
      <div class="icon">🌡️</div>
      <div class="value" id="realfeel">--</div>
      <div class="label">Real Feel</div>
      <div class="status" id="realfeel-status">Calculating...</div>
    </div>
  </div>

  <script>
    const ws = new WebSocket('ws://' + window.location.hostname + ':81/');
    ws.onopen = () => console.log('✅ WebSocket connected');
    ws.onclose = () => {
      console.log('❌ WebSocket disconnected');
      document.querySelectorAll('.status').forEach(s => s.textContent = 'Disconnected 🔌');
    };
    ws.onerror = (e) => console.error('WebSocket error:', e);
    
    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        console.log('Received:', data);
        document.getElementById('temp').textContent = data.temp.toFixed(1) + '°C';
        document.getElementById('hum').textContent = data.hum.toFixed(1) + '%';
        document.getElementById('pres').textContent = data.pres.toFixed(1) + ' hPa';
        document.getElementById('realfeel').textContent = data.realFeel.toFixed(1) + '°C';
        
        document.getElementById('temp-status').textContent = data.tempStatus;
        document.getElementById('hum-status').textContent = data.humStatus;
        document.getElementById('pres-status').textContent = data.presStatus;
        document.getElementById('realfeel-status').textContent = data.realFeelStatus;
      } catch(e) { 
        console.error('JSON parse error:', e, event.data);
      }
    };
  </script>
</body>
</html>
)rawliteral";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void sendSensorData() {
  sensors_event_t humidityEvent, tempEvent;
  
  if (!aht.getEvent(&humidityEvent, &tempEvent)) {
    Serial.println("AHT read failed!");
    return;
  }

  float temperature = tempEvent.temperature;
  float humidity = humidityEvent.relative_humidity;
  float pressure = bmp.readPressure() / 100.0F;

  // Update smoothing buffers
  temperatureBuffer[bufferIndex] = temperature;
  humidityBuffer[bufferIndex] = humidity;
  bufferIndex = (bufferIndex + 1) % 5;

  float smoothTemp = average(temperatureBuffer, 5);
  float smoothHum = average(humidityBuffer, 5);

  // Calculate improved Real Feel
  float realFeelTemp = calculateRealFeel(smoothTemp, smoothHum);

  String tempStatus = getTempStatus(smoothTemp);
  String humStatus = getHumStatus(smoothHum);
  String presStatus = getPresStatus(pressure);
  String realFeelStatus = getRealFeelStatus(realFeelTemp, smoothTemp, smoothHum);

  Serial.println("=== WEATHER UPDATE ===");
  Serial.println("Temp: " + String(smoothTemp,1) + "°C " + tempStatus);
  Serial.println("Hum:  " + String(smoothHum,1) + "% " + humStatus);
  Serial.println("Pres: " + String(pressure,1) + " hPa " + presStatus);
  Serial.println("RealFeel: " + String(realFeelTemp,1) + "°C " + realFeelStatus);
  Serial.println("====================");

  String payload = "{";
  payload += "\"temp\":" + String(smoothTemp, 2) + ",";
  payload += "\"hum\":" + String(smoothHum, 2) + ",";
  payload += "\"pres\":" + String(pressure, 2) + ",";
  payload += "\"realFeel\":" + String(realFeelTemp, 2) + ",";
  payload += "\"tempStatus\":\"" + tempStatus + "\",";
  payload += "\"humStatus\":\"" + humStatus + "\",";
  payload += "\"presStatus\":\"" + presStatus + "\",";
  payload += "\"realFeelStatus\":\"" + realFeelStatus + "\"";
  payload += "}";

  webSocket.broadcastTXT(payload);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.printf("[WS] Client %u connected\n", num);
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected\n", num);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n🚀 === ESP8266 CAPTIVE PORTAL WEATHER STATION v3.2 === 🚀");
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  // Initialize sensors first
  Wire.begin();
  Serial.print("AHT30... ");
  if (!aht.begin()) {
    Serial.println("❌ FAILED!");
    while(1) { digitalWrite(ledPin, !digitalRead(ledPin)); delay(200); }
  }
  Serial.println("✅");

  Serial.print("BMP280... ");
  if (!bmp.begin(0x76) && !bmp.begin(0x77)) {
    Serial.println("❌ FAILED!");
    while(1) { digitalWrite(ledPin, !digitalRead(ledPin)); delay(500); }
  }
  Serial.println("✅");

  // CAPTIVE PORTAL SETUP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("\n🌐 === CAPTIVE PORTAL READY ===");
  Serial.println("📶 SSID: " + String(ssid));
  Serial.println("🔓 NO PASSWORD");
  Serial.println("🌐 IP: " + IP.toString());
  Serial.println("📱 Phone will AUTO-OPEN browser!");
  Serial.println("============================");

  // DNS Server for Captive Portal (port 53)
  dnsServer.start(53, "*", IP);

  // HTTP Server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("✅ HTTP + Captive Portal OK");

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("✅ WebSocket OK");
  Serial.println("🎉 Connect phone - auto-opens weather page!\n");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  webSocket.loop();

  // LED heartbeat
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    digitalWrite(ledPin, LOW); delay(50);
    digitalWrite(ledPin, HIGH);
    lastBlink = millis();
  }

  // Update every 3 seconds
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 3000) {
    sendSensorData();
    lastUpdate = millis();
  }

  delay(10);
}
