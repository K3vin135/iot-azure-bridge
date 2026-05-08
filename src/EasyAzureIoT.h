#pragma once
// ════════════════════════════════════════════════════════════════════
//  EasyAzureIoT.h — Librería Arduino para Azure IoT Hub
//  Encapsula: WiFi, NTP, SAS token, MQTT, reconexión y renovación
//
//  DEPENDENCIAS (Manage Libraries):
//    - azure-sdk-for-c  (Microsoft)
//    - PubSubClient     (Nick O'Leary)
//    - ArduinoJson      (Benoit Blanchon)
// ════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <az_core.h>
#include <az_iot.h>
#include <mbedtls/base64.h>
#include <mbedtls/md.h>

// ════════════════════════════════════════════════════════════════════
//  PAYLOAD — Diccionario clave/valor para publicar
//
//  Uso:
//    Payload()
//      .add("state",   "ON")
//      .add("bunches", 60)
//      .add("temp",    36.5f)
// ════════════════════════════════════════════════════════════════════
class Payload
{
public:
  Payload &add(const char *key, const char *val)
  {
    _doc[key] = val;
    return *this;
  }
  Payload &add(const char *key, String val)
  {
    _doc[key] = val;
    return *this;
  }
  Payload &add(const char *key, int val)
  {
    _doc[key] = val;
    return *this;
  }
  Payload &add(const char *key, long val)
  {
    _doc[key] = val;
    return *this;
  }
  Payload &add(const char *key, float val)
  {
    _doc[key] = val;
    return *this;
  }
  Payload &add(const char *key, double val)
  {
    _doc[key] = val;
    return *this;
  }
  Payload &add(const char *key, bool val)
  {
    _doc[key] = val;
    return *this;
  }

private:
  StaticJsonDocument<512> _doc;
  friend class EasyAzureIoT;
};

// ════════════════════════════════════════════════════════════════════
//  EasyAzureIoT — Clase principal
// ════════════════════════════════════════════════════════════════════
class EasyAzureIoT
{
public:
  // ── Constructor ─────────────────────────────────────────────────
  // Parámetros obligatorios: credenciales WiFi + Azure
  // Opcionales: duración del token (por defecto 24h)
  EasyAzureIoT(
      const char *wifi_ssid,
      const char *wifi_pass,
      const char *hub_fqdn,
      const char *device_id,
      const char *device_key,
      int token_duration_mins = 1440, // 24 horas
      int token_refresh_mins = 1380   // renovar a las 23 horas
  );

  // ── API pública ──────────────────────────────────────────────────

  // Llamar en setup(): conecta WiFi, NTP, Azure, MQTT
  void begin();

  // Llamar al inicio de loop(): mantiene MQTT vivo y renueva token
  void loop();

  // Publicar un Payload
  // device_id y ts se agregan automáticamente — no hace falta incluirlos
  void publish(Payload payload);

  // Publicar un JsonDocument de ArduinoJson directamente
  void publish(JsonDocument &doc);

  // Publicar un JSON raw como String (para casos avanzados)
  void publishRaw(const String &json);

  // Estado de la conexión
  bool connected();

  // Timestamp actual en formato ISO 8601
  String timestamp();

private:
  // ── Credenciales ─────────────────────────────────────────────────
  const char *_wifi_ssid;
  const char *_wifi_pass;
  const char *_hub_fqdn;
  const char *_device_id;
  const char *_device_key;
  int _token_duration_mins;
  int _token_refresh_mins;

  // ── Buffers internos MQTT/Azure ──────────────────────────────────
  char _mqtt_client_id[64];
  char _mqtt_username[160];
  char _mqtt_password[320];
  char _telemetry_topic[128];

  // ── Estado interno ───────────────────────────────────────────────
  unsigned long _token_generated_at = 0;

  // ── Objetos SDK ──────────────────────────────────────────────────
  az_iot_hub_client _hub_client;
  WiFiClientSecure _wifi_client;
  PubSubClient _mqtt_client;

  // ── Métodos privados ─────────────────────────────────────────────
  void _connectWifi();
  void _syncNtp();
  void _initAzureClient();
  void _generateSasToken();
  void _connectMqtt();
  void _checkTokenRenewal();
  void _sendPayload(const char *json);
  void _log(const char *section);
};
