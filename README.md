# ESP32 HUB75 Display: Uhrzeit, PV-Leistung & Wetteranzeige

Dieses Projekt zeigt Uhrzeit, aktuelle PV-Leistung, Netzeinspeisung/-bezug und ein Wettersymbol in Echtzeit auf einem HUB75 LED-Matrix-Display (64x32 Pixel) mit einem ESP32.

## 📷 Anzeige-Inhalte

- **Uhrzeit** (oben links)
- **PV-Leistung** (mittig links, z. B. "2300W")
- **Netzstatus** (unten links, z. B. "–200W Grid" bei Einspeisung oder "450W Grid" bei Netzbezug)
- **Wettersymbol** (oben rechts, 18×18 Pixel)

## 🧰 Verwendete Hardware

- ESP32 (z. B. ESP32-WROOM-32)
- HUB75 LED-Matrix Display (z. B. 64x32 Pixel, P4)
- 5V Netzteil für Matrix (mind. 2–3 A empfohlen)

## 📦 Benötigte Bibliotheken

Installiere diese Bibliotheken über den Library Manager oder via GitHub:

- [`ESP32-HUB75-MatrixPanel-I2S-DMA`](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA)
- `WiFi.h`
- `PubSubClient.h`
- `time.h` (Standard)

## ⚙️ Setup & Konfiguration

### WLAN-Zugangsdaten
```cpp
const char* ssid = "DEIN_WLAN_NAME";
const char* password = "DEIN_WLAN_PASSWORT";
```

### MQTT-Broker
```cpp
const char* mqtt_server = "192.168.x.x";
const char* mqtt_user = "DEIN_MQTT_BENUTZER";
const char* mqtt_pass = "DEIN_MQTT_PASSWORT";
```

### MQTT Topics (Beispiele)
| Topic               | Beschreibung                  | Beispielwert     |
|---------------------|-------------------------------|------------------|
| `sma/pv_Leistung`   | Aktuelle PV-Erzeugung in W    | `2500`           |
| `sma/bezug`         | Aktueller Netzbezug in W      | `300`            |
| `sma/einspeisung`   | Aktuelle Netzeinspeisung in W | `0`              |
| `wetter/zustand`    | Wetterstatus als Text         | `sunny`          |

### Unterstützte Wetterzustände

Folgende Werte werden unterstützt (entsprechen dem MQTT-Topic `wetter/zustand`):

- `sunny`
- `cloudy`
- `partlycloudy`
- `rainy`
- `snowy`

Für jeden Zustand gibt es ein kleines Pixel-Art-Symbol.

## 📀 Display-Anschluss (Beispiel-Pinbelegung)

| Signal | ESP32 GPIO |
|--------|------------|
| R1     | 25         |
| G1     | 26         |
| B1     | 27         |
| R2     | 14         |
| G2     | 12         |
| B2     | 13         |
| A      | 23         |
| B      | 19         |
| C      | 5          |
| D      | 17         |
| E      | 32         |
| CLK    | 16         |
| LAT    | 4          |
| OE     | 15         |

## 🧪 Beispielanzeige

```
[12:45]      [☀️]
[ 2300W       ]
[–250W Grid   ]
```

## 🔧 Anpassungen

- Das Layout und die Farben können leicht angepasst werden.
- Wetterdaten kannst du z. B. aus Home Assistant über MQTT senden:
  - Automation oder Template-Sensor erstellen, der den Zustand wie `sunny` oder `rainy` ins Topic `wetter/zustand` veröffentlicht.

## 📜 Lizenz

Dieses Projekt ist für den **privaten Gebrauch** freigegeben.  
Eine **kommerzielle Nutzung** (Verkauf, Integration in Produkte etc.) ist nur nach vorheriger Zustimmung des Autors erlaubt.  

Kontakt für Anfragen: bitte über GitHub oder die im Repository hinterlegte E-Mail.

