# EasyAzureIoT

Arduino library to connect an ESP32 to **Azure IoT Hub** with minimal setup.  
Wraps WiFi, NTP, auto-renewable SAS token, and MQTT into a single class.

## Installation

### Option A — Arduino Library Manager (recommended)
Sketch → Include Library → Manage Libraries → search **EasyAzureIoT**

### Option B — Manual
Sketch → Include Library → Add .ZIP Library → select the downloaded zip

## Dependencies
Install from Manage Libraries:
- `azure-sdk-for-c` (Microsoft)
- `PubSubClient` (Nick O'Leary)
- `ArduinoJson` (Benoit Blanchon)

## Basic Usage

```cpp
#include <EasyAzureIoT.h>

EasyAzureIoT iot(
  "WIFI_SSID",
  "WIFI_PASSWORD",
  "your-hub.azure-devices.net",
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
    .add("status",  "ON")
    .add("bunches", 60)
    .add("temp",    36.5f)
  );

  delay(10000);
}
```

## Payload

`device` and `ts` are added automatically — no need to include them.

```cpp
iot.publish(Payload()
  .add("field",   "text")    // String
  .add("number",  42)        // int
  .add("decimal", 3.14f)     // float
  .add("active",  true)      // bool
);
```

## Advanced Configuration

```cpp
// Custom token duration (default 24h, renews at 23h)
EasyAzureIoT iot(
  "ssid", "pass", "hub.azure-devices.net", "device", "key",
  1440,  // token_duration_mins
  1380   // token_refresh_mins
);
```

## What it does automatically

- Connects and reconnects WiFi
- Syncs time with NTP (Los Angeles, UTC-8/UTC-7 DST)
- Generates the SAS token with mbedTLS — no external scripts
- Renews the token before it expires
- Reconnects MQTT with exponential backoff
- Appends `device` and `ts` to every message

## Compatibility

- ESP32 (all models including C6)
- Azure IoT Hub (all regions)
- Certificate: DigiCert Global Root G2 (expires 2038)

## License
MIT
