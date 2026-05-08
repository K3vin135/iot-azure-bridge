# EasyAzureIoT

Librería Arduino para conectar ESP32 a **Azure IoT Hub** de forma simple.  
Encapsula WiFi, NTP, SAS token auto-renovable y MQTT en una sola clase.

## Instalación

### Opción A — Arduino Library Manager (recomendado)
Sketch → Include Library → Manage Libraries → buscar **EasyAzureIoT**

### Opción B — Manual
Sketch → Include Library → Add .ZIP Library → seleccionar el zip descargado

## Dependencias
Instalar desde Manage Libraries:
- `azure-sdk-for-c` (Microsoft)
- `PubSubClient` (Nick O'Leary)
- `ArduinoJson` (Benoit Blanchon)

## Uso básico

```cpp
#include <EasyAzureIoT.h>

EasyAzureIoT iot(
  "WIFI_SSID",
  "WIFI_PASSWORD",
  "tu-hub.azure-devices.net",
  "device_id",
  "SharedAccessKey="
);

void setup() {
  Serial.begin(115200);
  iot.begin();
}

void loop() {
  iot.loop();

  iot.publish(Payload()
    .add("estado",  "ON")
    .add("bunches", 60)
    .add("temp",    36.5f)
  );

  delay(10000);
}
```

## Payload

`device` y `ts` se agregan automáticamente — no hace falta incluirlos.

```cpp
iot.publish(Payload()
  .add("campo",   "texto")   // String
  .add("numero",  42)        // int
  .add("decimal", 3.14f)     // float
  .add("activo",  true)      // bool
);
```

## Configuración avanzada

```cpp
// Token personalizado (por defecto 24h, renueva a las 23h)
EasyAzureIoT iot(
  "ssid", "pass", "hub.azure-devices.net", "device", "key",
  1440,  // token_duration_mins
  1380   // token_refresh_mins
);
```

## Lo que hace automáticamente

- Conecta y reconecta WiFi
- Sincroniza hora con NTP (Los Ángeles, UTC-8/UTC-7 DST)
- Genera el SAS token con mbedTLS — sin scripts externos
- Renueva el token antes de que venza
- Reconecta MQTT con backoff exponencial
- Agrega `device` y `ts` a cada mensaje

## Compatibilidad

- ESP32 (todos los modelos incluyendo C6)
- Azure IoT Hub (todas las regiones)
- Certificado: DigiCert Global Root G2 (vence 2038)

## Licencia
MIT
