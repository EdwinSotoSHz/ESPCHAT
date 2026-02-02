#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN   25
#define RST_PIN  27
#define SCK_PIN  26
#define MOSI_PIN 32
#define MISO_PIN 33

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);

  // SPI personalizado
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);

  rfid.PCD_Init();
  Serial.println("RC522 listo. Acerque una tarjeta...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
