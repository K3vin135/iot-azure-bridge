// ════════════════════════════════════════════════════════════════════
//  HavatecIoT.cpp — Implementación
// ════════════════════════════════════════════════════════════════════
#include "HavatecIoT.h"
#include <azure_ca.h>

// ── Constructor ──────────────────────────────────────────────────────
HavatecIoT::HavatecIoT(
    const char *wifi_ssid,
    const char *wifi_pass,
    const char *hub_fqdn,
    const char *device_id,
    const char *device_key,
    int token_duration_mins,
    int token_refresh_mins) : _mqtt_client(_wifi_client)
{
  _wifi_ssid = wifi_ssid;
  _wifi_pass = wifi_pass;
  _hub_fqdn = hub_fqdn;
  _device_id = device_id;
  _device_key = device_key;
  _token_duration_mins = token_duration_mins;
  _token_refresh_mins = token_refresh_mins;
}

// ════════════════════════════════════════════════════════════════════
//  begin() — Secuencia completa de inicio
// ════════════════════════════════════════════════════════════════════
void HavatecIoT::begin()
{
  Serial.println();
  Serial.println("╔══════════════════════════════════╗");
  Serial.println("║         HavatecIoT v1.0          ║");
  Serial.println("╚══════════════════════════════════╝");

  _connectWifi();
  _syncNtp();
  _initAzureClient();
  _generateSasToken();
  _connectMqtt();
}

// ════════════════════════════════════════════════════════════════════
//  loop() — Mantener conexión viva, renovar token si hace falta
// ════════════════════════════════════════════════════════════════════
void HavatecIoT::loop()
{
  _checkTokenRenewal();

  if (!_mqtt_client.connected())
  {
    _log("MQTT caído — reconectando");
    _connectMqtt();
  }

  _mqtt_client.loop();
}

// ════════════════════════════════════════════════════════════════════
//  publish(Payload) — Recibe el diccionario del usuario
//  Agrega automáticamente: device, ts
// ════════════════════════════════════════════════════════════════════
void HavatecIoT::publish(Payload payload)
{
  // Agregar campos automáticos (el usuario no necesita incluirlos)
  payload._doc["device"] = _device_id;
  payload._doc["ts"] = timestamp();

  char json[512];
  serializeJson(payload._doc, json, sizeof(json));
  _sendPayload(json);
}

// ════════════════════════════════════════════════════════════════════
//  publish(JsonDocument) — Para usuarios que arman su propio JSON
// ════════════════════════════════════════════════════════════════════
void HavatecIoT::publish(JsonDocument &doc)
{
  doc["device"] = _device_id;
  doc["ts"] = timestamp();

  char json[512];
  serializeJson(doc, json, sizeof(json));
  _sendPayload(json);
}

// ════════════════════════════════════════════════════════════════════
//  publishRaw(String) — JSON manual, sin agregados automáticos
// ════════════════════════════════════════════════════════════════════
void HavatecIoT::publishRaw(const String &json)
{
  _sendPayload(json.c_str());
}

// ════════════════════════════════════════════════════════════════════
//  connected() / timestamp()
// ════════════════════════════════════════════════════════════════════
bool HavatecIoT::connected()
{
  return _mqtt_client.connected();
}

String HavatecIoT::timestamp()
{
  struct tm t;
  if (!getLocalTime(&t))
    return "1970-01-01T00:00:00";
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &t);
  return String(buf);
}

// ════════════════════════════════════════════════════════════════════
//  PRIVADOS
// ════════════════════════════════════════════════════════════════════

void HavatecIoT::_log(const char *section)
{
  Serial.print(">>> ");
  Serial.println(section);
}

// ── WiFi ─────────────────────────────────────────────────────────────
void HavatecIoT::_connectWifi()
{
  _log("WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(_wifi_ssid, _wifi_pass);

  Serial.print("    Conectando");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("    IP: ");
  Serial.println(WiFi.localIP());
}

// ── NTP ──────────────────────────────────────────────────────────────
void HavatecIoT::_syncNtp()
{
  _log("NTP (Los Angeles)");
  // UTC-8 base (PST), +3600 de DST en verano (PDT) — el NTP maneja el cambio solo
  configTime(-8 * 3600, 3600, "pool.ntp.org", "time.nist.gov");

  Serial.print("    Sincronizando");
  time_t now = time(NULL);
  while (now < 1700000000)
  {
    Serial.print(".");
    delay(500);
    now = time(NULL);
  }
  Serial.println();
  Serial.print("    Hora: ");
  Serial.println(timestamp());
}

// ── Inicializar cliente Azure ─────────────────────────────────────────
void HavatecIoT::_initAzureClient()
{
  _log("Azure IoT Hub Client");

  az_iot_hub_client_init(
      &_hub_client,
      az_span_create_from_str((char *)_hub_fqdn),
      az_span_create_from_str((char *)_device_id),
      NULL);

  size_t len;
  az_iot_hub_client_get_client_id(
      &_hub_client, _mqtt_client_id, sizeof(_mqtt_client_id), &len);

  az_iot_hub_client_get_user_name(
      &_hub_client, _mqtt_username, sizeof(_mqtt_username), &len);

  az_iot_hub_client_telemetry_get_publish_topic(
      &_hub_client, NULL, _telemetry_topic, sizeof(_telemetry_topic), NULL);

  Serial.print("    Device:  ");
  Serial.println(_device_id);
  Serial.print("    Topic:   ");
  Serial.println(_telemetry_topic);
}

// ── Generar SAS Token con mbedTLS ─────────────────────────────────────
void HavatecIoT::_generateSasToken()
{
  _log("SAS Token");

  time_t now = time(NULL);
  uint64_t expiry = (uint64_t)now + ((uint64_t)_token_duration_mins * 60);

  // 1. Cadena a firmar
  uint8_t sig_buf[256];
  az_span sig_span = AZ_SPAN_FROM_BUFFER(sig_buf);
  az_result rc = az_iot_hub_client_sas_get_signature(
      &_hub_client, expiry, sig_span, &sig_span);

  if (az_result_failed(rc))
  {
    Serial.printf("    [ERROR] get_signature: 0x%x\n", rc);
    return;
  }

  // 2. Decodificar key de base64
  uint8_t decoded_key[64];
  size_t decoded_len = 0;
  mbedtls_base64_decode(
      decoded_key, sizeof(decoded_key), &decoded_len,
      (const unsigned char *)_device_key, strlen(_device_key));

  // 3. HMAC-SHA256
  uint8_t hmac[32];
  mbedtls_md_hmac(
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
      decoded_key, decoded_len,
      az_span_ptr(sig_span), az_span_size(sig_span),
      hmac);

  // 4. Base64 encode del HMAC
  uint8_t b64_hmac[48];
  size_t b64_len = 0;
  mbedtls_base64_encode(
      b64_hmac, sizeof(b64_hmac), &b64_len, hmac, sizeof(hmac));

  // 5. Token SAS completo
  size_t pwd_len;
  rc = az_iot_hub_client_sas_get_password(
      &_hub_client, expiry,
      az_span_create(b64_hmac, b64_len),
      AZ_SPAN_EMPTY,
      _mqtt_password, sizeof(_mqtt_password), &pwd_len);

  if (az_result_failed(rc))
  {
    Serial.printf("    [ERROR] get_password: 0x%x\n", rc);
    return;
  }

  _token_generated_at = millis();
  Serial.printf("    OK — vence en %d horas\n", _token_duration_mins / 60);
}

// ── Conectar MQTT ─────────────────────────────────────────────────────
void HavatecIoT::_connectMqtt()
{
  _log("MQTT");
  _wifi_client.setCACert((const char *)ca_pem);
  //_wifi_client.setInsecure(); // deshabilita verificacion TLS temporalmente
  _mqtt_client.setServer(_hub_fqdn, 8883);
  _mqtt_client.setBufferSize(512);
  _mqtt_client.setKeepAlive(60);

  int attempts = 0;
  while (!_mqtt_client.connected())
  {
    attempts++;
    Serial.printf("    Conectando (intento %d)... ", attempts);

    if (_mqtt_client.connect(_mqtt_client_id, _mqtt_username, _mqtt_password))
    {
      Serial.println("OK");
    }
    else
    {
      int wait_s = min(attempts * 3, 30);
      Serial.printf("FALLÓ rc=%d — reintento en %ds\n",
                    _mqtt_client.state(), wait_s);
      delay(wait_s * 1000);

      if (attempts >= 5)
      {
        Serial.println("    Regenerando token...");
        _generateSasToken();
        attempts = 0;
      }
    }
  }
}

// ── Chequear renovación de token ──────────────────────────────────────
void HavatecIoT::_checkTokenRenewal()
{
  unsigned long refresh_ms = (unsigned long)_token_refresh_mins * 60UL * 1000UL;
  if (millis() - _token_generated_at >= refresh_ms)
  {
    _log("Renovando SAS token");
    _mqtt_client.disconnect();
    delay(300);
    _generateSasToken();
    _connectMqtt();
  }
}

// ── Enviar JSON a Azure vía MQTT ──────────────────────────────────────
void HavatecIoT::_sendPayload(const char *json)
{
  if (!_mqtt_client.connected())
  {
    _log("MQTT caído — reconectando antes de publicar");
    _connectMqtt();
  }

  bool ok = _mqtt_client.publish(_telemetry_topic, json);
  Serial.printf("[%s] %s\n", ok ? "PUB OK  " : "PUB FAIL", json);
}
