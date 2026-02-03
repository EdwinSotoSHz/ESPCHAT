# ESP32 WiFi Chat con Control RFID

Este proyecto implementa un **chat local entre ESP32** utilizando **WiFi y HTTP**, con soporte para **mensajes de texto e imágenes** desde un navegador web.
Una de las ESP32 incorpora un **lector RFID (RC522)** que funciona como **capa de seguridad física**, permitiendo el envío de información **solo si se presenta una tarjeta autorizada**.

## Características principales

* Comunicación entre ESP32 por **WiFi (red local)**
* Interfaz web embebida (HTML + JavaScript)
* Envío de mensajes de texto
* Envío de imágenes comprimidas (Base64)
* Descubrimiento automático de nodos mediante **mDNS**
* Control de acceso mediante **RFID (UID autorizado)**
* Gestión básica de memoria (limpieza de historial)

## Tecnologías utilizadas

* ESP32
* WiFi (HTTP / TCP-IP)
* mDNS
* RFID RC522 (SPI)
* HTML, JavaScript (Frontend embebido)

## Notas

* El control RFID solo se aplica a una ESP específica.
* Los demás nodos funcionan como receptores normales del chat.
* El sistema está pensado para prácticas académicas y redes locales.

---

Si quieres, también puedo:

* Hacer una versión **todavía más corta**
* Ajustarlo a formato **práctica escolar**
* Añadir una sección mínima de **instalación** o **conexiones**
