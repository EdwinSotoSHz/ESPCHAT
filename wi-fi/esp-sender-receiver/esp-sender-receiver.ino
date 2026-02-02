#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>

const char* ssid = "AsusE";
const char* password = "23011edpi";
//const char* identificador_propio = "esp-1"; 
const char* identificador_propio = "esp-2";

WebServer server(80);
String listaDispositivos = "";
String mensajesRecibidos = "";
String ultimoMensaje = "";

// Variables para enviar mensajes
String ipDestino = "";
String mensajeAEnviar = "";

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP Chat</title>";
  html += "</head><body>";
  html += "<h1>ESP Chat - " + String(identificador_propio) + "</h1>";
  
  // Lista de dispositivos
  html += "<h2>Dispositivos encontrados:</h2>";
  html += "<ul>" + listaDispositivos + "</ul>";
  html += "<p><a href='/buscar'>Actualizar lista</a></p>";
  html += "<hr>";
  
  // Formulario para enviar mensajes
  html += "<h2>Enviar mensaje:</h2>";
  html += "<form action='/enviar' method='POST'>";
  html += "IP Destino: <input type='text' name='ip' value='" + ipDestino + "'><br><br>";
  html += "Mensaje: <textarea name='mensaje' rows='3' cols='40'></textarea><br><br>";
  html += "<input type='submit' value='Enviar'>";
  html += "</form>";
  html += "<hr>";
  
  // Mensajes recibidos
  html += "<h2>Mensajes recibidos:</h2>";
  html += mensajesRecibidos;
  
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void buscarDispositivos() {
  listaDispositivos = "";
  Serial.println("Iniciando búsqueda mDNS...");
  
  int n = MDNS.queryService("esp-link", "tcp");
  
  if (n == 0) {
    listaDispositivos = "<li>No se encontraron otras placas.</li>";
    Serial.println("No se encontraron servicios.");
  } else {
    Serial.printf("%d dispositivos encontrados\n", n);
    for (int i = 0; i < n; ++i) {
      String nombreEncontrado = MDNS.hostname(i);
      String ipEncontrada = MDNS.address(i).toString();
      
      // No mostrar nuestro propio dispositivo
      if (nombreEncontrado != identificador_propio) {
        listaDispositivos += "<li>Nombre: " + nombreEncontrado;
        listaDispositivos += " | IP: " + ipEncontrada + "</li>";
        
        Serial.println("Encontrado: " + nombreEncontrado + " en IP: " + ipEncontrada);
      }
    }
  }
}

void handleEnviarMensaje() {
  if (server.hasArg("ip") && server.hasArg("mensaje")) {
    ipDestino = server.arg("ip");
    mensajeAEnviar = server.arg("mensaje");
    
    if (ipDestino.length() > 0 && mensajeAEnviar.length() > 0) {
      enviarMensajeHTTP(ipDestino, mensajeAEnviar);
    }
  }
  handleRoot();
}

void enviarMensajeHTTP(String ip, String mensaje) {
  HTTPClient http;
  String url = "http://" + ip + "/recibir";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Crear datos para enviar
  String datos = "remitente=" + String(identificador_propio) + "&mensaje=" + mensaje;
  
  int codigoRespuesta = http.POST(datos);
  
  if (codigoRespuesta > 0) {
    Serial.printf("Mensaje enviado. Código: %d\n", codigoRespuesta);
    if (codigoRespuesta == HTTP_CODE_OK) {
      String respuesta = http.getString();
      Serial.println("Respuesta: " + respuesta);
    }
  } else {
    Serial.printf("Error enviando mensaje: %s\n", http.errorToString(codigoRespuesta).c_str());
  }
  
  http.end();
}

void handleRecibirMensaje() {
  if (server.hasArg("remitente") && server.hasArg("mensaje")) {
    String remitente = server.arg("remitente");
    String mensaje = server.arg("mensaje");
    
    // Almacenar mensaje recibido
    ultimoMensaje = "De: " + remitente + " - Mensaje: " + mensaje;
    
    // Agregar a historial
    mensajesRecibidos = "<p><strong>" + remitente + ":</strong> " + mensaje + "</p>" + mensajesRecibidos;
    
    Serial.println("Mensaje recibido de " + remitente + ": " + mensaje);
    
    // Responder con confirmación
    server.send(200, "text/plain", "Mensaje recibido por " + String(identificador_propio));
  } else {
    server.send(400, "text/plain", "Faltan parámetros");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nWiFi Conectado!");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());

  // Iniciar mDNS
  if (MDNS.begin(identificador_propio)) {
    Serial.println("mDNS iniciado como: " + String(identificador_propio));
  }

  // Anunciar servicio
  MDNS.addService("esp-link", "tcp", 80);

  // Configurar rutas del servidor web
  server.on("/", handleRoot);
  server.on("/buscar", []() {
    buscarDispositivos();
    handleRoot();
  });
  server.on("/enviar", HTTP_POST, handleEnviarMensaje);
  server.on("/recibir", HTTP_POST, handleRecibirMensaje);
  
  server.begin();
  Serial.println("Servidor web listo.");
  
  // Realizar primera búsqueda
  buscarDispositivos();
}

void loop() {
  server.handleClient();
  
  // Actualizar periódicamente la lista de dispositivos (cada 30 segundos)
  static unsigned long ultimaBusqueda = 0;
  if (millis() - ultimaBusqueda > 30000) {
    buscarDispositivos();
    ultimaBusqueda = millis();
  }
  
  delay(10);
}