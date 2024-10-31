/* Grupo 1
Alumnos: Alejandro Arroyo Velazquez y Maria de las Mercedes Ramos Santana
Multiactuador básico de la práctica 1 fase 1, compuesto por un actuador de enchufe y
un actuador de interruptor.
El actuador de enchufe tiene conectado como carga un led blanco, y para simular
su funcionamiento, se enciende y apaga a intervalos regulares.
El actuador de interruptor tiene conectado como carga un led rojo, que es
activado por un relé. Mediante un botón se activa/desactiva la carga.
El actuador de persiana tiene conectado como carga un led verde que simula la subida 
y un led amarillo que simula la bajada. Cuando se da la orden de parar se apagan ambos.
*/

#include <ESP8266WiFi.h>        // Librería para la conexión WiFi
#include <ESP8266WebServer.h>   // Librería para el servidor web

// Configuración de la red WiFi y el servidor web
const char* ssid = "JoseMiguel";         // Nombre de la red WiFi
const char* password = "jordicalatrava"; // Contraseña de la red WiFi
IPAddress wifiIP(192, 168,43, 35);       // Dirección IP fija
IPAddress wifiNET(255, 255, 255, 0);     // Máscara de red
IPAddress wifiON(192, 168, 43, 255);     // Puerta de enlace
ESP8266WebServer servidorWeb(80);        // Creación del objeto servidor en el puerto 80

/* Definición de constantes COMUNES */
#define ENCENDER HIGH
#define APAGADO LOW
#define ENCENDER_INTERNO LOW
#define APAGADO_INTERNO HIGH
#define NUM_LECTURAS_CONSECUTIVAS (5)// Número de lecturas consecutivas necesarias
#define NUMERO_REINTENTOS (3)

/* Definición de constantes y variables del ACTUADOR DE ENCHUFE */
const int ledBlanco = 12;                // Pin del LED blanco (enchufe)
const int ledInterno = LED_BUILTIN;      // Pin del LED interno
unsigned long tiempoAnterior = 0;        // Variable para medir tiempo
const long intervaloEncendido = 2000;    // Tiempo de encendido del enchufe
const long intervaloApagado = 1000;      // Tiempo de apagado del enchufe
bool estadoEnchufe = APAGADO;            // Estado inicial del enchufe
bool estadoPulsador;              // Estado del pulsador

/* Definición de constantes y variables del ACTUADOR DE INTERRUPTOR */
const int ledRojo = 16;        // Pin del LED rojo (interruptor)
const int pinPulsador = 15;    // Pin del pulsador
const int ledVerde = 2;        // Pin del LED verde (persiana)
const int ledAmarillo = 13;    // Pin del LED amarillo (persiana)
bool estadoInterruptor = APAGADO;       // Estado actual del interruptor
bool estadoPulsadorActual;  // Estado actual del pulsador
bool estadoPulsadorAnterior=0;        
int lecturasConsecutivas;
bool conectadoWifi=0;
int intentos=0;
int tiempoTranscurrido = 0;
int tiempoEspera = 5000; 


void setup() {
  // Inicialización de pines como salidas
  pinMode(ledRojo, OUTPUT);
  digitalWrite(ledRojo, APAGADO);
  pinMode(ledInterno, OUTPUT);
  digitalWrite(ledInterno, APAGADO_INTERNO);
  pinMode(pinPulsador, INPUT);          // Inicialización del pulsador como entrada
  pinMode(ledBlanco, OUTPUT);
  digitalWrite(ledBlanco, APAGADO);
  pinMode(ledVerde, OUTPUT);
  digitalWrite(ledVerde, APAGADO);
  pinMode(ledAmarillo, OUTPUT);
  digitalWrite(ledAmarillo, APAGADO);
  
  // Inicialización de la comunicación serie para depuración
  Serial.begin(115200);
  Serial.println("Iniciando programa...");
  
  // Conexión a la red WiFi y configuración del servidor
  conexionWifi();
  configurarServer();
  servidorWeb.begin();                  // Inicio del servidor
}

void loop() {
  servidorWeb.handleClient();            // Manejo de las peticiones al servidor web
  comprobar_pulsacion();   // Comprobación del estado del pulsador

  if(estadoPulsador==HIGH){
    manejadorEncenderInterruptor();
  }
  if(estadoPulsador==LOW){
    manejadorApagarInterruptor();
  }              
}
void comprobar_pulsacion(){
  estadoPulsadorActual = digitalRead(pinPulsador);

  // Si el estado actual es igual al estado anterior, incrementa el contador de lecturas consecutivas
  if (estadoPulsadorActual == estadoPulsadorAnterior) {
    lecturasConsecutivas++;
  } else {
    // Reinicia el contador si el estado cambió
    lecturasConsecutivas = 0;
  }
  estadoPulsadorAnterior=estadoPulsadorActual;
  // Si alcanzamos el número requerido de lecturas consecutivas, cambia el valor de estadoPulsador
  if (lecturasConsecutivas >= NUM_LECTURAS_CONSECUTIVAS) {
    estadoPulsador = estadoPulsadorActual; // Cambia estadoPulsador al estado confirmado (HIGH o LOW)
    Serial.println(estadoPulsador == HIGH ? "Pulsado" : "No pulsado");
    lecturasConsecutivas = 0; // Reinicia el contador después de confirmar
  }
  // Pequeño retraso para estabilizar las lecturas
  delay(10);                                 // Pequeño retraso para evitar rebotes
}
 

// Manejadores para las rutas del servidor web

void manejadorEstado() {
  // Muestra el estado actual del interruptor (encendido o apagado)
  if(estadoInterruptor) {
    servidorWeb.send(200, "text/plain", "encendido");
  } else { 
    servidorWeb.send(200, "text/plain", "apagado");
  }
}

void manejadorEncenderEnchufe () {
  // Maneja el encendido del actuador de enchufe
  digitalWrite(ledInterno, ENCENDER_INTERNO);   // Encender LED interno
  digitalWrite(ledBlanco, ENCENDER);            // Encender LED blanco
  estadoEnchufe = ENCENDER;                     // Actualizar estado del enchufe
  servidorWeb.send(200, "text/plain", "OK, actuador de enchufe encendido");
  Serial.println("Enchufe encendido");
}

void manejadorApagarEnchufe () {
  // Maneja el apagado del actuador de enchufe
  digitalWrite(ledInterno, APAGADO_INTERNO);    // Apagar LED interno
  digitalWrite(ledBlanco, APAGADO);             // Apagar LED blanco
  estadoEnchufe = APAGADO;                      // Actualizar estado del enchufe
  servidorWeb.send(200, "text/plain", "OK, actuador de enchufe apagado");
  Serial.println("Enchufe apagado");
}

void manejadorEncenderInterruptor () {
  // Maneja el encendido del interruptor
  if(estadoInterruptor == APAGADO){
    digitalWrite(ledInterno, ENCENDER_INTERNO);   // Encender LED interno
    digitalWrite(ledRojo, ENCENDER);              // Encender LED rojo (interruptor)
    estadoInterruptor = ENCENDER;                 // Actualizar estado del interruptor
    servidorWeb.send(200, "text/plain", "OK, actuador de interruptor encendido");
    Serial.println("Interruptor encendido");
  }else{
    Serial.println("Interruptor encendido");
  }
}

void manejadorApagarInterruptor () {
  // Maneja el apagado del interruptor
  if(estadoInterruptor == ENCENDER){
    digitalWrite(ledInterno, APAGADO_INTERNO);    // Apagar LED interno
    digitalWrite(ledRojo, APAGADO);               // Apagar LED rojo (interruptor)
    estadoInterruptor = APAGADO;                  // Actualizar estado del interruptor
    servidorWeb.send(200, "text/plain", "OK, actuador de interruptor apagado");
    Serial.println("Interruptor apagado");
  }else{
    Serial.println("Interruptor apagado");
  }
}

// Manejadores de la persiana
void manejadorPersianaSubir () {
  digitalWrite(ledVerde, ENCENDER);             // Encender LED verde (subir persiana)
  digitalWrite(ledAmarillo, APAGADO);           // Apagar LED amarillo
  servidorWeb.send(200, "text/plain", "OK, actuador de persiana: orden de subir");
  Serial.println("Subiendo persiana");
}

void manejadorPersianaBajar () {
  digitalWrite(ledVerde, APAGADO);              // Apagar LED verde
  digitalWrite(ledAmarillo, ENCENDER);          // Encender LED amarillo (bajar persiana)
  servidorWeb.send(200, "text/plain", "OK, actuador de persiana: orden de bajar");
  Serial.println("Bajando persiana");
}

void manejadorPersianaParar () {
  digitalWrite(ledVerde, APAGADO);              // Apagar ambos LEDs
  digitalWrite(ledAmarillo, APAGADO);
  servidorWeb.send(200, "text/plain", "OK, actuador de persiana: orden de parar");
  Serial.println("Parando persiana");
}

// Función para la conexión WiFi
void conexionWifi() {
  while (intentos < NUMERO_REINTENTOS && !conectadoWifi) {  // Intenta conectarse un máximo de 3 veces
      delay(10);  // Da tiempo a que se inicialice el hardware de la Wifi
      WiFi.mode(WIFI_STA);
      WiFi.config(wifiIP, wifiON, wifiNET); 
      WiFi.begin(ssid, password);

      Serial.print("Intentando conectar al WiFi (intento ");
      Serial.print(intentos + 1);
      Serial.println(") ...");

      while (WiFi.status() != WL_CONNECTED && tiempoTranscurrido < tiempoEspera) {
        delay(500);  // Espera medio segundo antes de verificar otra vez
        tiempoTranscurrido += 500;
      }

      // Verifica si se conectó exitosamente
      if (WiFi.status() == WL_CONNECTED) {
        conectadoWifi = true;
        Serial.println("\nConectado a WiFi!");
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP()); // Imprimir dirección IP al conectar
      } else {
        intentos++;  // Incrementa el contador de intentos
        Serial.println("No se pudo conectar, reintentando...");
      }
    }

  if (!conectadoWifi) {
    Serial.println("No se pudo conectar a WiFi después de 3 intentos.");
  }
}

// Función para configurar las rutas del servidor web
void configurarServer(){
  // Define la ruta raíz ("/") que llamará a la función manejadorRaiz
  servidorWeb.on ("/", manejadorRaiz);

  // Define la ruta para encender el enchufe, llamando a la función manejadorEncenderEnchufe
  servidorWeb.on("/enchufe/encender", manejadorEncenderEnchufe);

  // Define la ruta para apagar el enchufe, llamando a la función manejadorApagarEnchufe
  servidorWeb.on ("/enchufe/apagar", manejadorApagarEnchufe);

  // Define la ruta para encender el interruptor, llamando a la función manejadorEncenderInterruptor
  servidorWeb.on("/interruptor/encender", manejadorEncenderInterruptor);

  // Define la ruta para apagar el interruptor, llamando a la función manejadorApagarInterruptor
  servidorWeb.on ("/interruptor/apagar", manejadorApagarInterruptor);

  // Define la ruta para consultar el estado del interruptor, llamando a la función manejadorEstado
  servidorWeb.on ("/interruptor/estado", manejadorEstado);

  // Define la ruta para subir la persiana, llamando a la función manejadorPersianaSubir
  servidorWeb.on("/persiana/subir", manejadorPersianaSubir);

  // Define la ruta para bajar la persiana, llamando a la función manejadorPersianaBajar
  servidorWeb.on ("/persiana/bajar", manejadorPersianaBajar);

  // Define la ruta para parar la persiana, llamando a la función manejadorPersianaParar
  servidorWeb.on ("/persiana/parar", manejadorPersianaParar);

  // Define la función para manejar los casos en que se solicite una página no encontrada
  servidorWeb.onNotFound(paginaNoEncontrada);
}

// Función que gestiona la página principal del servidor web
void manejadorRaiz () {
  // Creación de la variable que contendrá el contenido HTML
  String mensaje;
  mensaje = "<!DOCTYPE HTML>\r\n<html>\r\n"; // Definición del tipo de documento
  mensaje += "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"; // Configuración del meta charset
  mensaje += "<title>Multiactuador</title>\r\n"; // Título de la página
  mensaje += "<h1>Multiactuador:</h1>"; // Encabezado principal
  mensaje += "</head>\r\n";
  
  // Contenido relacionado con el actuador de enchufe
  mensaje += "<body>";
  mensaje += "<h3>Actuador de enchufe:</h3>";
  mensaje += "Pulsa para <a href=\"/enchufe/encender\">Encender</a><br>"; // Enlace para encender
  mensaje += "Pulsa para <a href=\"/enchufe/apagar\">Apagar</a><br>"; // Enlace para apagar
  mensaje += "</body>";

  // Contenido relacionado con el actuador de interruptor
  mensaje += "<body>";
  mensaje += "<h3>Actuador de interruptor:</h3>";
  mensaje += "Pulsa para <a href=\"/interruptor/encender\">Encender</a><br>"; // Enlace para encender
  mensaje += "Pulsa para <a href=\"/interruptor/apagar\">Apagar</a><br>"; // Enlace para apagar
  mensaje += "Pulsa para <a href=\"/interruptor/estado\">Consulta de Estado</a>"; // Enlace para consultar el estado
  mensaje += "</body>";

  // Contenido relacionado con el actuador de persiana
  mensaje += "<body>";
  mensaje += "<h3>Actuador de persiana:</h3>";
  mensaje += "Pulsa para <a href=\"/persiana/subir\">Subir</a><br>"; // Enlace para subir la persiana
  mensaje += "Pulsa para <a href=\"/persiana/bajar\">Bajar</a><br>"; // Enlace para bajar la persiana
  mensaje += "Pulsa para <a href=\"/persiana/parar\">Parar</a>"; // Enlace para parar la persiana
  mensaje += "</body>";

  // Final del contenido HTML
  mensaje += "</html>\n";

  // Envía la respuesta HTML al cliente
  servidorWeb.send(200, "text/html; charset=UTF-8", mensaje);
}

// Función que gestiona los casos en los que una página solicitada no existe
void paginaNoEncontrada() {
  // Crea un mensaje informando que la página no fue encontrada
  String mensaje = "Página no encontrada\n\n"; 

  // Añade la URI que el cliente intentó acceder
  mensaje += "URI: ";
  mensaje += servidorWeb.uri ();

  // Añade el método de la solicitud (GET o POST)
  mensaje += "\nMetodo: ";
  mensaje += (servidorWeb.method () == HTTP_GET) ? "GET": "POST";

  // Añade los argumentos pasados en la solicitud
  mensaje += "\nArgumentos: ";
  mensaje += servidorWeb.args();
  mensaje += "\n";

  // Recorre y muestra los nombres y valores de los argumentos
  for (uint8_t i = 0; i < servidorWeb.args (); i++) {
    mensaje += " " + servidorWeb.argName (i) + ": " + servidorWeb.arg (i) + "\n";
  }

  // Envía la respuesta de error 404 (Página no encontrada) al cliente
  servidorWeb.send (404, "text/plain", mensaje);
}
