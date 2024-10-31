/* Grupo 1
Alumnos: Alejandro Arroyo Velazquez y Maria de las Mercedes Ramos Santana
Multiactuador básico de la práctica 1 fase 2, compuesto por un actuador de enchufe y
un actuador de interruptor. El programa recibi las ordenes por MQTT aunque 
conserva el uso del interruptor fisico.
*/

#include <WiFi.h>
#include <PubSubClient.h> // Biblioteca para el cliente MQTT

// Configuración de la red WiFi y el servidor web
const char* ssid = "JoseMiguel";         // Nombre de la red WiFi
const char* password = "jordicalatrava"; // Contraseña de la red WiFi
IPAddress wifiIP(192, 168,43, 35);       // Dirección IP fija
IPAddress wifiNET(255, 255, 255, 0);     // Máscara de red
IPAddress wifiON(192, 168, 43, 255);     // Puerta de enlace

IPAddress mqtt_server (192, 168, 43, 32); 

const String clientId = "esp32Cliente-1"; 
WiFiClient clienteWifi;    
PubSubClient clienteMQTT(clienteWifi);

/* Definición de constantes COMUNES */
#define ENCENDIDO_INTERNO LOW
#define APAGADO_INTERNO HIGH
#define ENCENDER "encender"
#define APAGAR "apagar"
#define MSG_BUFFER_SIZE  (50)
#define NUMERO_REINTENTOS (3)
#define TIEMPO_RECONEXION (5000)
#define TIEMPO_ENTRE_MENSAJES (5000)
#define TOPIC_SUBSCRIPCION_ENCHUFE "casa/dormitorio/enchufe/orden" 
#define TOPIC_SUBSCRIPCION_INTERRUPTOR "casa/dormitorio/luztecho/orden"
#define TOPIC_PUBLICACION_ESTADO "casa/dormitorio/luztecho/estado"
#define ENCENDIDO HIGH
#define APAGADO LOW
#define NUM_LECTURAS_CONSECUTIVAS (5)// Número de lecturas consecutivas necesarias

/* Definición de constantes y variables del ACTUADOR DE ENCHUFE */

unsigned long tiempoAnterior = 0;
unsigned long tiempoUltimoMensaje = 0;        // Variable para medir tiempo
// Variables manejo tiempos WIFI
int tiempoEspera = 5000; // Tiempo máximo para intentar conectar en milisegundos (5 segundos)
int tiempoTranscurrido = 0;
bool estadoEnchufe = APAGADO;            // Estado inicial del enchufe
bool estadoPulsador = 0;                 // Estado del pulsador

/* Definición de constantes y variables del ACTUADOR DE INTERRUPTOR */
const int pinEnchufe = 13;                // Pin del LED blanco (enchufe)
const int ledInterno = LED_BUILTIN;      // Pin del LED interno
const int pinInterruptor = 2;        // Pin del LED ambar (interruptor)
const int pinPulsador = 4;    // Pin del pulsador
int intentos=0;
bool estadoInterruptor = APAGADO;       // Estado actual del interruptor
bool estadoPulsadorActual;       // Estado actual del pulsador
bool estadoPulsadorAnterior=0;       
bool estadoPulsador;
bool conectadoWifi=0;
bool conectadoBroker=0;
char mensaje[MSG_BUFFER_SIZE];


void setup() {
  // Inicialización de pines como salidas
  pinMode(pinInterruptor, OUTPUT);
  digitalWrite(pinInterruptor, APAGADO);
  pinMode(ledInterno, OUTPUT);
  digitalWrite(ledInterno, APAGADO_INTERNO);
  pinMode(pulsadorPin, INPUT);          // Inicialización del pulsador como entrada
  pinMode(pinEnchufe, OUTPUT);
  digitalWrite(pinEnchufe, APAGADO);
  
  // Inicialización de la comunicación serie para depuración
  Serial.begin(115200);
  Serial.println("Iniciando programa...");
  
  // Conexión a la red WiFi y configuración del cliente MQTT
  conexionWifi();
  clienteMQTT.setServer(mqtt_server, 1883);
  clienteMQTT.setCallback(manejador_mensajes);

}

void loop() {
  // Se asegura de que existe conexión con el broker
  if (!clienteMQTT.connected()) {
    conectar_MQTT();
  }else if(clienteMQTT.connected()){
    // Se comprueban mensajes entrantes
    clienteMQTT.loop();
  }

  loop_actuador_interruptor();                 // Comprobación del estado del pulsador
}

//void comprobar_pulsacion(){
  //estadoPulsador = digitalRead(pulsadorPin);   // Leer el estado del pulsador
  //delay(50);                                  // Pequeño retraso para evitar rebotes
//}


void manejadorEncenderInterruptor () {
  // Maneja el encendido del interruptor
  digitalWrite(pinInterruptor, ENCENDIDO);              // ENCENDIDO LED rojo (interruptor)
  estadoInterruptor = ENCENDIDO;                 // Actualizar estado del interruptor
  Serial.println("Interruptor encendido");
}

void manejadorApagarInterruptor () {
  // Maneja el apagado del interruptor
  digitalWrite(pinInterruptor, APAGADO);               // APAGADO LED rojo (interruptor)
  estadoInterruptor = APAGADO;                  // Actualizar estado del interruptor
  Serial.println("Interruptor apagado");
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

void conectar_MQTT() {
  
  for (int i=0 ; i<NUMERO_REINTENTOS ; i++) {
    // Intento de conexión
    if (clienteMQTT.connect(clientId.c_str())) {
      Serial.println("Conectado al broker");
      clienteMQTT.subscribe(TOPIC_SUBSCRIPCION_ENCHUFE);
      clienteMQTT.subscribe(TOPIC_SUBSCRIPCION_INTERRUPTOR);
      conectadoBroker=1;
      i=NUMERO_REINTENTOS;
      break;
    } else {
      conectadoBroker=0;
      Serial.print("Error al conectar al broker. Resultado: ");
      Serial.print(clienteMQTT.state());
      Serial.println(". Se volverá a intentar en " + String(TIEMPO_RECONEXION/1000) +" segundos. ");
      delay(TIEMPO_RECONEXION);
    }
  }
  if (!conectadoBroker) {
    Serial.println("No se pudo conectar al broker MQTT después de múltiples intentos.");
    // Aquí puedes decidir qué hacer, como reiniciar o deshabilitar la conexión
  }
}

void manejador_mensajes(char *canal, byte *mensaje, unsigned int longitud) {
    Serial.print("Ha llegado un mensaje para el canal: ");
    Serial.println(canal);
    if (strcmp(canal, TOPIC_SUBSCRIPCION_ENCHUFE) == 0) {
        tratamiento_mensaje_enchufe(mensaje, longitud);
    } else if (strcmp(canal, TOPIC_SUBSCRIPCION_INTERRUPTOR) == 0) {
        tratamiento_mensaje_interruptor(mensaje, longitud);
    } else {
        Serial.println("*** ERROR ***. Formato del topic recibido erróneo.");
    }
    
}

void tratamiento_mensaje_enchufe (byte *mensaje, unsigned int longitud) {

  char bufferTemporal [MSG_BUFFER_SIZE];
  strncpy (bufferTemporal, (char *) mensaje, longitud);
  bufferTemporal [longitud] = '\0';
  
  Serial.print("Mensaje recibido: ");
  Serial.println(bufferTemporal);

  if (strcmp(bufferTemporal,ENCENDER) == 0) {
    Serial.println("Se procede a encender la luz.");
    digitalWrite(pinEnchufe, ENCENDIDO);   // ENCENDIDO LED interno
  } else if (strcmp(bufferTemporal,APAGAR) == 0) {
    digitalWrite(pinEnchufe, APAGADO);
    Serial.println("Se procede a apagar la luz.");
  } else {
    Serial.println("*** ERROR ***. Formato del mensaje recibido erróneo.");
  }
    
  Serial.println("--------");
}
void tratamiento_mensaje_interruptor (byte *mensaje, unsigned int longitud) {

  char bufferTemporal [MSG_BUFFER_SIZE];
  strncpy (bufferTemporal, (char *) mensaje, longitud);
  bufferTemporal [longitud] = '\0';
  
  Serial.print("Mensaje recibido: ");
  Serial.println(bufferTemporal);

  if (strcmp(bufferTemporal,ENCENDER) == 0) {
    Serial.println("Se procede a encender la luz.");
    manejadorEncenderInterruptor();
    digitalWrite(ledInterno, ENCENDIDO_INTERNO);   // ENCENDIDO LED interno
  } else if (strcmp(bufferTemporal,APAGAR) == 0) {
    manejadorApagarInterruptor();
    digitalWrite(ledInterno, APAGADO_INTERNO);
    Serial.println("Se procede a apagar la luz.");
  } else {
    Serial.println("*** ERROR ***. Formato del mensaje recibido erróneo.");
  }
    
  Serial.println("--------");
}

void publicarEstado(const char * topic, const char * mensaje){
  Serial.println(mensaje);
  clienteMQTT.publish(TOPIC_PUBLICACION_ESTADO, mensaje);
}
void loop_actuador_interruptor() {

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
  delay(10);

  // Actualizar el último estado del pulsador
  long tiempo = millis();
  if (tiempo - tiempoUltimoMensaje > TIEMPO_ENTRE_MENSAJES) {
    tiempoUltimoMensaje = tiempo;
    if(conectadoBroker){
      snprintf(mensaje, MSG_BUFFER_SIZE, "%s", estadoInterruptor ? "ON" : "OFF");
      publicarEstado(TOPIC_PUBLICACION_ESTADO, mensaje);
    }
  }
}