#include "BluetoothSerial.h"
#include <DHT.h>

// Verifica si la librería BluetoothSerial está disponible en el entorno de compilación
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth no está habilitado! Por favor, activa el Bluetooth en el menú de configuración.
#endif


#define DHT11_PIN 23  // Pin GPIO al que está conectado el DHT11
DHT dht(DHT11_PIN, DHT11);

BluetoothSerial SerialBT;

const int ldrPin = 34;
int ldrValue;
const int ledPins[] = { 15, 2, 4, 16, 17, 5, 18, 19, 21 };  // Pins de los LEDs
const int numLeds = 9;
const int pwmChannel[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };  // Canales PWM para cada LED
bool isAutomatic = false;                                // Variable para controlar el modo automatico

void setup() {
  Serial.begin(115200);
  SerialBT.begin("EcoLumen");
  dht.begin();  // Inicializa el sensor DHT11
  // Configurar cada pin de LED como salida y asignar un canal PWM
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    ledcAttachPin(ledPins[i], pwmChannel[i]);
    ledcSetup(pwmChannel[i], 12000, 8);
  }

  // Configurar la función de callback para cuando se reciban datos por Bluetooth
  SerialBT.onData(onBluetoothDataReceived);
}

void loop() {
  Temperature();

  if (isAutomatic) {
    AutomaticMode();
  }
}

// Esta función se llama automáticamente cuando llegan datos por Bluetooth
void onBluetoothDataReceived(const uint8_t* buffer, size_t size) {
  // Convertir los datos recibidos a String para procesarlos
  String message = "";
  for (size_t i = 0; i < size; i++) {
    message += (char)buffer[i];
  }
  controlLed(message);
}

void controlLed(String message) {
  if (message.equals("AUTO")) {
    isAutomatic = true;
  } else if (message.equals("NOAUTO")) {
    isAutomatic = false;
  }

  if (message.equals("ON")) {
    for (int i = 0; i < numLeds; i++) {
      ledcWrite(pwmChannel[i], 255);
    }
  } else if (message.equals("OFF")) {
    for (int i = 0; i < numLeds; i++) {
      ledcWrite(pwmChannel[i], 0);
    }
  }

  if (!isAutomatic) {
    int idLed = message.substring(3, message.indexOf(':')).toInt();
    int brillo = message.substring(message.indexOf(':') + 1).toInt();

    if (idLed >= 1 && idLed <= numLeds && brillo >= 0 && brillo <= 255) {
      ledcWrite(pwmChannel[idLed - 1], brillo);
    } else {
      SerialBT.println("Mensaje inválido");
    }
  }
}


void Temperature() {
  delay(1000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Error al leer el sensor DHT11"));
    return;
  }

  // SerialBT.print(F("Humedad: "));
  // SerialBT.print(h);
  // SerialBT.print(F("% - Temperatura: "));
  // SerialBT.print(t);
  // SerialBT.println(F("°C"));
  String msg = (String) h + " " + t;
  SerialBT.println(msg);
}

void AutomaticMode() {
  ldrValue = analogRead(ldrPin);

  if (ldrValue > 2300) {
    ldrValue = 2300;
  }

  int autoBrightness = map(ldrValue, 0, 2300, 255, 0);
  Serial.println(ldrValue);
  for (int i = 0; i < numLeds; i++) {
    ledcWrite(pwmChannel[i], autoBrightness);
  }
  delay(1000);
}