#include <EasyAzureIoT.h>

EasyAzureIoT iot(
  "SSID",
  "SSID_PASSWORD",
  "xxx.azure-devices.net",
  "deviceName",
  "deviceprimarykey"
);

void setup() {
  Serial.begin(115200);
  iot.begin();
}

void loop() {
  iot.loop();

  // Marca ANTES de publicar
  unsigned long antes = micros();

  iot.publish(Payload()
    .add("estado", "ON")
    .add("bunches", 42)
  );

  // Marca DESPUÉS de publicar
  unsigned long despues = micros();

  unsigned long duracion_us = despues - antes;
  float duracion_ms = duracion_us / 1000.0;

  Serial.print("Tiempo publish(): ");
  Serial.print(duracion_ms);
  Serial.println(" ms");

  delay(10000);
}
