//Biblotheken einbinden
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include <time.h>
#include <PubSubClient.h>

// WLAN-Zugangsdaten (bitte mit eigenen Daten ersetzen)
const char* ssid = "DEIN_WLAN_NAME";
const char* password = "DEIN_WLAN_PASSWORT";
 
// MQTT Broker (bitte IP-Adresse und Zugangsdaten anpassen)
const char* mqtt_server = "192.168.x.x";  // Beispiel: "192.168.1.100"
const int mqtt_port = 1883;

//topics abonnieren
const char* pvLeistung_topic = "dein/topic"; 
const char* weather_topic = "dein/topic";
const char* bezug_topic = "dein/topic";
const char* einspeisung_topic = "dein/topic";

// MQTT Benutzername und Passwort (bitte anpassen)
const char* mqtt_user = "DEIN_MQTT_BENUTZER";
const char* mqtt_pass = "DEIN_MQTT_PASSWORT";

//Globale Variablen für MQTT & Status
WiFiClient espClient;
PubSubClient client(espClient);
String pvLeistung = "0 W";
String weatherState = "rainy";  // Standardwert
int bezugWert = 0;        // Netzbezug (W)
int einspeisungWert = 0;  // Netzeinspeisung (W)


// Matrix-Panel Konfiguration (64x32 Pixel, 1 Panel)
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32
#define PANELS_NUMBER 1

// Pinbelegung für HUB75-Panel
#define R1 25
#define G1 26
#define B1 27
#define R2 14
#define G2 12
#define B2 13
#define A 23
#define B 19
#define C 5
#define D 17
#define E 32
#define CLK 16
#define LAT 4
#define OE 15


// Matrix-Setup initialisieren
HUB75_I2S_CFG mxconfig(PANEL_WIDTH, PANEL_HEIGHT, PANELS_NUMBER);
MatrixPanel_I2S_DMA matrix(mxconfig);


// NTP-Zeit konfigurieren (Mitteleuropäische Zeit inkl. Sommerzeit)
void setupTime() {
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
}


// Wetter-Symbol "bewölkt" zeichnen
void drawCloudyIcon(int offsetX, int offsetY, uint16_t color) {
  uint8_t icon[11][18] = {
    // 1 = Pixel gesetzt, 0 = leer
    { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
  };
  // Bitmap auf Matrix zeichnen
  for (int y = 0; y < 11; y++) {
    for (int x = 0; x < 18; x++) {
      if (icon[y][x]) matrix.drawPixel(offsetX + x, offsetY + y, color);
    }
  }
}
// Wetter-Symbol "sonnig" zeichnen
void drawSunnyIcon(int offsetX, int offsetY, uint16_t color) {
  // Kreis (Sonne)
  matrix.drawCircle(offsetX + 9, offsetY + 9, 5, color);
  // Strahlen im 45°-Winkel
  for (int i = 0; i < 360; i += 45) {
    int dx = 9 + cos(i * DEG_TO_RAD) * 7;
    int dy = 9 + sin(i * DEG_TO_RAD) * 7;
    matrix.drawPixel(offsetX + dx, offsetY + dy, color);
  }
}
// Wetter-Symbol "regnerisch" zeichnen
void drawRainyIcon(int offsetX, int offsetY, uint16_t color) {
  drawCloudyIcon(offsetX, offsetY, color); // Wolke zeichnen
  uint16_t rainColor = matrix.color565(0, 150, 255); // Blau für Regen
  // Drei Tropfen als Linien darunter
  for (int i = 0; i < 3; i++) {
    matrix.drawLine(offsetX + 4 + i * 4, offsetY + 12, offsetX + 4 + i * 4, offsetY + 15, rainColor);
  }
}
// Wetter-Symbol "teils bewölkt" zeichnen
void drawPartlyCloudyIcon(int offsetX, int offsetY, uint16_t color) {
  drawSunnyIcon(offsetX + 3, offsetY + 3, matrix.color565(255, 255, 0)); // Sonne leicht versetzt
  drawCloudyIcon(offsetX, offsetY + 4, color); // Wolke teilweise drüber
}
// Wetter-Symbol "Schnee" zeichnen
void drawSnowyIcon(int offsetX, int offsetY, uint16_t color) {
  drawCloudyIcon(offsetX, offsetY, color);  // Wolke zeichnen
  uint16_t snowColor = matrix.color565(255, 255, 255); // Weiß für Schnee
   // Drei einfache Schneeflocken
  for (int i = 0; i < 3; i++) {
    matrix.drawPixel(offsetX + 4 + i * 4, offsetY + 13, snowColor);
    matrix.drawPixel(offsetX + 4 + i * 4 + 1, offsetY + 14, snowColor);
  }
}
// Wetterzustand basierend auf Text zeichnen
void drawWeatherIcon(String state, int x, int y) {
  uint16_t color = matrix.color565(200, 200, 255); // Basisfarbe für neutrale Symbole
  if (state == "cloudy") drawCloudyIcon(x, y, color);
  else if (state == "sunny") drawSunnyIcon(x, y, matrix.color565(255, 255, 0));
  else if (state == "rainy") drawRainyIcon(x, y, color);
  else if (state == "partlycloudy") drawPartlyCloudyIcon(x, y, color);
  else if (state == "snowy") drawSnowyIcon(x, y, color);
}
// Callback-Funktion für empfangene MQTT-Nachrichten
void callback(char* topic, byte* payload, unsigned int length) {
    // Nachricht in lesbaren String umwandeln
  char msg[32];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  // Daten verarbeiten je nach Topic
  if (strcmp(topic, pvLeistung_topic) == 0) {
    pvLeistung = String(msg) + "W";
    Serial.println("PV-Leistung empfangen: " + pvLeistung);
  } else if (strcmp(topic, weather_topic) == 0) {
    weatherState = String(msg);
    Serial.println("Wetterzustand empfangen: " + weatherState);
  } else if (strcmp(topic, bezug_topic) == 0) {
    // Bezug empfangen
    String bezugStr = String(msg);
    bezugStr.trim();
    bezugWert = bezugStr.toInt();
    Serial.println("Bezug empfangen: " + bezugStr + "W");
  } else if (strcmp(topic, einspeisung_topic) == 0) {
    // Einspeisung empfangen
    String einspeisungStr = String(msg);
    einspeisungStr.trim();
    einspeisungWert = einspeisungStr.toInt();
    Serial.println("Einspeisung empfangen: " + einspeisungStr + "W");
  }
}

// MQTT-Verbindung aufbauen & Subscribes setzen
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Verbindung zu MQTT...");
    if (client.connect("ESP32-Matrix", mqtt_user, mqtt_pass)) {
      Serial.println(" verbunden!");
      client.subscribe(pvLeistung_topic);
      client.subscribe(weather_topic);
      client.subscribe(bezug_topic);        // Netzbezug
      client.subscribe(einspeisung_topic);  // Netzeinspeisung
    } else {
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println(" - erneuter Versuch in 5 Sekunden");
      delay(5000);
    }
  }
}
// Einmaliger Setup-Aufruf beim Start
void setup() {
  Serial.begin(115200);

// WLAN verbinden
  WiFi.begin(ssid, password);
  Serial.print("Verbinde mit WLAN...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" verbunden!");

// Zeit initialisieren
  setupTime();
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Warte auf NTP-Zeit...");
    delay(1000);
  }

// Matrix initialisieren
  matrix.begin();
  delay(100);
  matrix.setBrightness8(80); // Helligkeit 0–255
  matrix.setTextWrap(false);
  matrix.fillScreen(0); // Bildschirm löschen

  // MQTT konfigurieren
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}
// Haupt-Loop – wird ständig wiederholt
void loop() {
  // Verbindung prüfen
  if (!client.connected()) reconnectMQTT();
  client.loop();



 // Zeit abrufen
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Zeit konnte nicht abgerufen werden");
    return;
  }
 // Stunden und Minuten als Strings extrahieren
  char hh[3], mm[3];
  strftime(hh, sizeof(hh), "%H", &timeinfo);
  strftime(mm, sizeof(mm), "%M", &timeinfo);
// Bildschirm löschen und neue Anzeige zeichnen
  matrix.fillScreen(0);
  matrix.setTextColor(matrix.color565(0, 0, 255));
  matrix.setTextSize(1);
// Uhrzeit oben links anzeigen
  int baseX = 2;
  int y = 2;
  matrix.setCursor(baseX, y);
  matrix.print(hh);
  matrix.setCursor(baseX + 12, y);
  matrix.print(":");
  matrix.setCursor(baseX + 18, y);
  matrix.print(mm);

 // PV-Leistung direkt darunter anzeigen
  String displayLeistung = pvLeistung;
  if (displayLeistung.length() > 10) displayLeistung = displayLeistung.substring(0, 10);
  matrix.setTextSize(1);
  int xPv = 2;
  int yPv = y + 10;                                   // Direkt unter der Uhr
  matrix.setTextColor(matrix.color565(255, 255, 0));  // PV-Leistung in Gelb
  matrix.setCursor(xPv, yPv);
  matrix.print(displayLeistung);


  // Netzbezug oder Einspeisung darunter
  String netzStatus;
  if (bezugWert >= 1) {
    netzStatus = String(bezugWert) + "W";
    matrix.setTextColor(matrix.color565(255, 0, 0));  // Bezug in Rot
  } else {
    netzStatus = "-" + String(einspeisungWert) + "W";
    matrix.setTextColor(matrix.color565(0, 255, 0));  // Einspeisung in Grün
  }

  matrix.setTextSize(1);
  int xStatus = 2;
  int yStatus = yPv + 10;  // Direkt unter der PV-Leistung
  matrix.setCursor(xStatus, yStatus);
  matrix.print(netzStatus);
  matrix.print(" Grid");  // "Grid" hinter Bezug/Einspeisung

   // Wetter-Icon oben rechts anzeigen (Position: rechts mit Abstand)
  drawWeatherIcon(weatherState, 46, 2);

  delay(1000); // 1 Sekunde warten bis zur nächsten Aktualisierung
}
