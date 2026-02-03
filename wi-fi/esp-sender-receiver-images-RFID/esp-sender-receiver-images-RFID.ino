#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

/* ---------- WIFI ---------- */
const char* ssid = "AsusE";
const char* password = "23011edpi";

/* ---------- IDENTIDAD ---------- */
const char* identificador_propio = "esp-rfid";

/* ---------- RFID ---------- */
#define SS_PIN   25
#define RST_PIN  27
#define SCK_PIN  26
#define MOSI_PIN 32
#define MISO_PIN 33

MFRC522 rfid(SS_PIN, RST_PIN);
byte uidPermitido[4] = {0x4C, 0x6A, 0x18, 0x03};

/* ---------- WEB ---------- */
WebServer server(80);
String mensajesRecibidos = "";
String listaDispositivos = "";

/* ---------- RFID CHECK ---------- */
bool uidValido() {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != uidPermitido[i]) {
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return false;
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return true;
}

/* ---------- HTML ---------- */
String paginaHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial}";
  html += ".msg{border-bottom:1px solid #ccc;padding:8px}";
  html += "img{width:320px;border:2px solid #555;margin-top:6px}";
  html += "</style></head><body>";

  html += "<h2>Nodo: " + String(identificador_propio) + "</h2>";

  html += "IP Destino: <input id='targetIp'><br><br>";
  html += "Texto: <input id='msgText'>";
  html += "<button onclick='enviarTexto()'>Enviar</button><hr>";

  html += "Imagen: <input type='file' id='imgInput' accept='image/*'>";
  html += "<button onclick='procesarYEnviar()'>Enviar Imagen</button><br><br>";

  html += "<button onclick='limpiarChat()' style='background:#c00;color:#fff'>Limpiar chat</button>";
  html += "<p><a href='/buscar'>Actualizar dispositivos</a></p>";
  html += "<ul>" + listaDispositivos + "</ul><hr>";
  html += "<div>" + mensajesRecibidos + "</div>";

  html += R"rawliteral(
<script>
async function enviarTexto(){
  const ip=targetIp.value,msg=msgText.value;
  if(!ip||!msg)return alert('Faltan datos');
  const d=new URLSearchParams();
  d.append('ip',ip);d.append('mensaje',msg);
  const r=await fetch('/enviarTexto',{method:'POST',body:d});
  if((await r.text())!='OK')alert('UID incorrecto');
  location.reload();
}

function procesarYEnviar(){
  const f=imgInput.files[0],ip=targetIp.value;
  if(!f||!ip)return alert('Faltan datos');
  const r=new FileReader();
  r.onload=e=>{
    const i=new Image();
    i.onload=()=>{
      const c=document.createElement('canvas');
      const MAX=120;
      const s=MAX/i.width;
      c.width=MAX;c.height=i.height*s;
      c.getContext('2d').drawImage(i,0,0,c.width,c.height);
      const d=new URLSearchParams();
      d.append('ip',ip);
      d.append('img',encodeURIComponent(c.toDataURL('image/jpeg',0.5)));
      fetch('/enviarImagen',{method:'POST',body:d})
      .then(r=>r.text()).then(t=>{
        if(t!='OK')alert('UID incorrecto');
        location.reload();
      });
    };
    i.src=e.target.result;
  };
  r.readAsDataURL(f);
}

function limpiarChat(){
  fetch('/limpiar',{method:'POST'}).then(()=>location.reload());
}
</script>
)rawliteral";

  html += "</body></html>";
  return html;
}

/* ---------- BUSCAR ---------- */
void buscarDispositivos() {
  listaDispositivos = "";
  int n = MDNS.queryService("esp-link", "tcp");
  for (int i = 0; i < n; i++) {
    listaDispositivos += "<li>" + MDNS.hostname(i) +
      " (" + MDNS.address(i).toString() + ")</li>";
  }
}

/* ---------- EMISOR ---------- */
void handleEnviarTexto() {
  if (!uidValido()) {
    server.send(200, "text/plain", "UID incorrecto");
    return;
  }

  HTTPClient http;
  http.begin("http://" + server.arg("ip") + "/recibirTexto");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST("remitente=" + String(identificador_propio) +
            "&mensaje=" + server.arg("mensaje"));
  http.end();

  mensajesRecibidos =
    "<div class='msg'><b>Tú:</b> " + server.arg("mensaje") + "</div>" +
    mensajesRecibidos;

  server.send(200, "text/plain", "OK");
}

void handleEnviarImagen() {
  if (!uidValido()) {
    server.send(200, "text/plain", "UID incorrecto");
    return;
  }

  HTTPClient http;
  http.begin("http://" + server.arg("ip") + "/recibirImagen");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST("remitente=" + String(identificador_propio) +
            "&img=" + server.arg("img"));
  http.end();

  mensajesRecibidos =
    "<div class='msg'><b>Tú:</b> Imagen enviada</div>" +
    mensajesRecibidos;

  server.send(200, "text/plain", "OK");
}

/* ---------- RECEPTOR ---------- */
void handleRecibirTexto() {
  mensajesRecibidos =
    "<div class='msg'><b>" + server.arg("remitente") +
    ":</b> " + server.arg("mensaje") + "</div>" +
    mensajesRecibidos;
  server.send(200, "text/plain", "OK");
}

void handleRecibirImagen() {
  String img = server.arg("img");
  img.replace("%2B","+");img.replace("%2F","/");
  img.replace("%3D","=");img.replace("%3A",":");
  img.replace("%2C",",");img.replace("%25","%");
  mensajesRecibidos =
    "<div class='msg'><b>" + server.arg("remitente") +
    ":</b><br><img src='" + img + "'></div>" +
    mensajesRecibidos;
  server.send(200, "text/plain", "OK");
}

void handleLimpiar() {
  mensajesRecibidos = "";
  server.send(200, "text/plain", "OK");
}

/* ---------- SETUP ---------- */
void setup() {
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  MDNS.begin(identificador_propio);
  MDNS.addService("esp-link", "tcp", 80);

  server.on("/", [](){ server.send(200,"text/html",paginaHTML()); });
  server.on("/buscar", [](){ buscarDispositivos(); server.sendHeader("Location","/"); server.send(303); });

  server.on("/enviarTexto", HTTP_POST, handleEnviarTexto);
  server.on("/enviarImagen", HTTP_POST, handleEnviarImagen);
  server.on("/recibirTexto", HTTP_POST, handleRecibirTexto);
  server.on("/recibirImagen", HTTP_POST, handleRecibirImagen);
  server.on("/limpiar", HTTP_POST, handleLimpiar);

  server.begin();
}

/* ---------- LOOP ---------- */
void loop() {
  server.handleClient();
}
