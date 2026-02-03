#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>

const char* ssid = "AsusE";
const char* password = "23011edpi";

const char* identificador_propio = "esp-1";
// http://esp-1.local/

WebServer server(80);

String listaDispositivos = "";
String mensajesRecibidos = "";

// ===================== HTML + JS =====================
String paginaHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

  html += "<style>";
  html += "body{font-family:Arial}";
  html += ".msg{border-bottom:1px solid #ccc; padding:8px}";
  html += "img{";
  html += "width:200px;";          
  html += "border:2px solid #555;";
  html += "margin-top:6px;";
  html += "}";
  html += "</style>";

  html += "<title>ESP Chat Duo</title></head><body>";

  html += "<h2>Nodo: " + String(identificador_propio) + "</h2>";

  html += "IP Destino: <input id='targetIp' placeholder='Ej: 192.168.1.50'><br><br>";

  html += "Texto: <input id='msgText'>";
  html += "<button onclick='enviarTexto()'>Enviar</button><hr>";

  html += "Imagen: <input type='file' id='imgInput' accept='image/*'>";
  html += "<button id='btnImg' onclick='procesarYEnviar()'>Enviar Imagen</button><br><br>";

  html += "<button onclick='limpiarChat()' style='background:#c00;color:#fff'>Limpiar chat</button>";

  html += "<p><a href='/buscar'>Actualizar dispositivos</a></p>";
  html += "<ul>" + listaDispositivos + "</ul><hr>";

  html += "<h3>Chat:</h3><div id='box'>" + mensajesRecibidos + "</div>";

  html += R"rawliteral(
<script>
async function enviarTexto() {
  const ip = targetIp.value;
  const msg = msgText.value;
  if (!ip || !msg) return alert('Falta IP o mensaje');

  const data = new URLSearchParams();
  data.append('ip', ip);
  data.append('mensaje', msg);

  await fetch('/enviarTexto', { method:'POST', body:data });
  location.reload();
}

function procesarYEnviar() {
  const file = imgInput.files[0];
  const ip = targetIp.value;
  if (!file || !ip) return alert('Selecciona imagen e IP');

  btnImg.disabled = true;
  btnImg.innerText = 'Enviando...';

  const reader = new FileReader();
  reader.onload = e => {
    const img = new Image();
    img.onload = () => {
      const canvas = document.createElement('canvas');
      const MAX = 120;   // NO SUBIR (RAM ESP32)
      const scale = MAX / img.width;
      canvas.width = MAX;
      canvas.height = img.height * scale;
      canvas.getContext('2d').drawImage(img, 0, 0, canvas.width, canvas.height);

      const dataUrl = canvas.toDataURL('image/jpeg', 0.5);

      const data = new URLSearchParams();
      data.append('ip', ip);
      data.append('img', encodeURIComponent(dataUrl));

      fetch('/enviarImagen', { method:'POST', body:data })
        .then(() => location.reload());
    };
    img.src = e.target.result;
  };
  reader.readAsDataURL(file);
}

function limpiarChat() {
  fetch('/limpiar', { method:'POST' })
    .then(() => location.reload());
}
</script>
)rawliteral";

  html += "</body></html>";
  return html;
}

// ===================== EMISOR =====================
void handleEnviarTexto() {
  if (server.hasArg("ip") && server.hasArg("mensaje")) {
    HTTPClient http;
    http.begin("http://" + server.arg("ip") + "/recibirTexto");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST("remitente=" + String(identificador_propio) +
              "&mensaje=" + server.arg("mensaje"));
    http.end();

    mensajesRecibidos =
      "<div class='msg' style='color:blue'><b>Tú:</b> " +
      server.arg("mensaje") + "</div>" + mensajesRecibidos;
  }
  server.send(200, "text/plain", "OK");
}

void handleEnviarImagen() {
  if (server.hasArg("ip") && server.hasArg("img")) {
    HTTPClient http;
    http.begin("http://" + server.arg("ip") + "/recibirImagen");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST("remitente=" + String(identificador_propio) +
              "&img=" + server.arg("img"));
    http.end();

    mensajesRecibidos =
      "<div class='msg' style='color:blue'><b>Tú:</b> Imagen enviada</div>" +
      mensajesRecibidos;
  }
  server.send(200, "text/plain", "OK");
}

// ===================== RECEPTOR =====================
void handleRecibirTexto() {
  mensajesRecibidos =
    "<div class='msg'><b>" + server.arg("remitente") +
    ":</b> " + server.arg("mensaje") + "</div>" +
    mensajesRecibidos;

  server.send(200, "text/plain", "OK");
}

void handleRecibirImagen() {
  String img = server.arg("img");

  // DECODIFICAR URL
  img.replace("%2B", "+");
  img.replace("%2F", "/");
  img.replace("%3D", "=");
  img.replace("%3A", ":");
  img.replace("%3B", ";");
  img.replace("%2C", ",");
  img.replace("%25", "%");

  mensajesRecibidos =
    "<div class='msg'><b>" + server.arg("remitente") +
    ":</b><br><img src='" + img + "'></div>" +
    mensajesRecibidos;

  server.send(200, "text/plain", "OK");
}

// ===================== LIMPIAR CHAT =====================
void handleLimpiarChat() {
  mensajesRecibidos = "";
  server.send(200, "text/plain", "OK");
}

// ===================== mDNS =====================
void buscarDispositivos() {
  listaDispositivos = "";
  int n = MDNS.queryService("esp-link", "tcp");
  for (int i = 0; i < n; i++) {
    listaDispositivos += "<li>" + MDNS.hostname(i) +
                         " (" + MDNS.address(i).toString() + ")</li>";
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  MDNS.begin(identificador_propio);
  MDNS.addService("esp-link", "tcp", 80);

  server.on("/", []() {
    server.send(200, "text/html", paginaHTML());
  });

  server.on("/buscar", []() {
    buscarDispositivos();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/enviarTexto", HTTP_POST, handleEnviarTexto);
  server.on("/enviarImagen", HTTP_POST, handleEnviarImagen);
  server.on("/recibirTexto", HTTP_POST, handleRecibirTexto);
  server.on("/recibirImagen", HTTP_POST, handleRecibirImagen);
  server.on("/limpiar", HTTP_POST, handleLimpiarChat);

  server.begin();
}

// ===================== LOOP =====================
void loop() {
  server.handleClient();
}
