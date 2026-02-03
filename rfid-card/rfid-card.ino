#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>

// Pinout
/*
| MFRC522  | ESP32       |
| -------- | ----------- |
| SDA / SS | GPIO   25   |
| SCK      | GPIO   26   |
| MOSI     | GPIO   32   |
| MISO     | GPIO   33   |
| RST      | GPIO   27   |
| 3.3V     | 3.3V        |
| GND      | GND         |

*/

/* ---------- WIFI ---------- */
const char* ssid = "AsusE";
const char* password = "23011edpi";

/* ---------- RFID ---------- */
#define SS_PIN   25
#define RST_PIN  27
#define SCK_PIN  26
#define MOSI_PIN 32
#define MISO_PIN 33

MFRC522 rfid(SS_PIN, RST_PIN);

/* UID AUTORIZADO */
byte uidPermitido[4] = {0x4C, 0x6A, 0x18, 0x03};

/* ---------- WEB ---------- */
WebServer server(80);
bool accesoPermitido = false;

/* ---------- HTML ---------- */
String paginaHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='utf-8'>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<title>Acceso RFID</title>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;background:#111;color:#fff;}";
  html += ".ok{color:#2ecc71;font-size:40px;}";
  html += ".bad{color:#e74c3c;font-size:40px;}";
  html += "</style></head><body>";

  if (accesoPermitido) {
    html += "<h1 class='ok'>ACCESO PERMITIDO</h1>";
  } else {
    html += "<h1 class='bad'>ID INCORRECTO</h1>";
  }

  html += "<p>Acerque una tarjeta RFID</p>";
  html += "</body></html>";
  return html;
}

/* ---------- COMPARAR UID ---------- */
bool compararUID(byte *uid) {
  for (byte i = 0; i < 4; i++) {
    if (uid[i] != uidPermitido[i]) return false;
  }
  return true;
}

void handleRoot() {
  server.send(200, "text/html", paginaHTML());
}

void setup() {
  Serial.begin(115200);

  /* SPI personalizado */
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  /* WIFI */
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  /* WEB */
  server.on("/", handleRoot);
  server.begin();

  Serial.println("Servidor web iniciado");
  Serial.println("RC522 listo...");
}

void loop() {
  server.handleClient();

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print("UID leído: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  accesoPermitido = compararUID(rfid.uid.uidByte);

  if (accesoPermitido) {
    Serial.println(">> ACCESO PERMITIDO");
  } else {
    Serial.println(">> ID INCORRECTO");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
