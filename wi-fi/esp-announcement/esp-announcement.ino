#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

const char* ssid = "AsusE";
const char* password = "23011edpi";
const char* identificador_propio = "esp-1"; 
//const char* identificador_propio = "esp-2"; 

WebServer server(80);
String listaDispositivos = "";

void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'></head><body>";
  html += "<h1>Dispositivos ESP32 encontrados</h1>";
  html += "<ul>" + listaDispositivos + "</ul>";
  html += "<p><a href='/buscar'>Actualizar lista</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void buscarDispositivos() {
  listaDispositivos = "";
  Serial.println("Iniciando búsqueda mDNS...");
  
  // Buscar servicio de esp-link
  int n = MDNS.queryService("esp-link", "tcp");
  
  if (n == 0) {
    listaDispositivos = "<li>No se encontraron otras placas.</li>";
    Serial.println("No se encontraron servicios.");
  } else {
    Serial.printf("%d dispositivos encontrados\n", n);
    for (int i = 0; i < n; ++i) {
      String nombreEncontrado = MDNS.hostname(i);
      String ipEncontrada = MDNS.address(i).toString(); // CORRECCIÓN AQUÍ
      
      listaDispositivos += "<li>Nombre: " + nombreEncontrado;
      listaDispositivos += " | IP: " + ipEncontrada + "</li>";
      
      Serial.println("Encontrado: " + nombreEncontrado + " en IP: " + ipEncontrada);
    }
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

  // 1. Iniciar mDNS
  if (MDNS.begin(identificador_propio)) {
    Serial.println("mDNS iniciado como: " + String(identificador_propio));
  }

  // 2. Anunciar servicio
  MDNS.addService("esp-link", "tcp", 80);

  // 3. Servidor Web
  server.on("/", handleRoot);
  server.on("/buscar", []() {
    buscarDispositivos();
    handleRoot();
  });
  server.begin();
  Serial.println("Servidor web listo.");
}

void loop() {
  server.handleClient();
  delay(100);
}

/*
http://esp-1.local
http://esp-2.local
*/