/* Client side utilities for esp32-weather-epd.
 * Copyright (C) 2022-2024  Luke Marzen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// built-in C++ libraries
#include <cstring>
#include <vector>

// arduino/esp32 libraries
#include <Arduino.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <time.h>

#include "ArduinoUZlib.h"

// additional libraries
#include <Adafruit_BusIO_Register.h>
#include <ArduinoJson.h>

// header files
#include "_locale.h"
#include "api_response.h"
#include "aqi.h"
#include "client_utils.h"
#include "config.h"
#include "display_utils.h"
#include "renderer.h"
#ifndef USE_HTTP
#include <WiFiClientSecure.h>
#endif

#ifdef USE_HTTP
static const uint16_t OWM_PORT = 80;
#else
static const uint16_t OWM_PORT = 443;
#endif
static ArduinoUZlib azlib;
static size_t _bufferSize;
static uint8_t _buffer[1024 * 4];  // gzip流最大缓冲区
/* Power-on and connect WiFi.
 * Takes int parameter to store WiFi RSSI, or “Received Signal Strength
 * Indicator"
 *
 * Returns WiFi status.
 */
wl_status_t startWiFi(int &wifiRSSI) {
  WiFi.mode(WIFI_STA);
  Serial.printf("%s '%s'", TXT_CONNECTING_TO, WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // timeout if WiFi does not connect in WIFI_TIMEOUT ms from now
  unsigned long timeout = millis() + WIFI_TIMEOUT;
  wl_status_t connection_status = WiFi.status();

  while ((connection_status != WL_CONNECTED) && (millis() < timeout)) {
    Serial.print(".");
    delay(50);
    connection_status = WiFi.status();
  }
  Serial.println();

  if (connection_status == WL_CONNECTED) {
    wifiRSSI = WiFi.RSSI();  // get WiFi signal strength now, because the WiFi
                             // will be turned off to save power!
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.printf("%s '%s'\n", TXT_COULD_NOT_CONNECT_TO, WIFI_SSID);
  }
  return connection_status;
}  // startWiFi

/* Disconnect and power-off WiFi.
 */
void killWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}  // killWiFi

/* Prints the local time to serial monitor.
 *
 * Returns true if getting local time was a success, otherwise false.
 */
bool printLocalTime(tm *timeInfo) {
  int attempts = 0;
  while (!getLocalTime(timeInfo) && attempts++ < 3) {
    Serial.println(TXT_FAILED_TO_GET_TIME);
    return false;
  }
  Serial.println(timeInfo, "%A, %B %d, %Y %H:%M:%S");
  return true;
}  // printLocalTime

/* Waits for NTP server time sync, adjusted for the time zone specified in
 * config.cpp.
 *
 * Returns true if time was set successfully, otherwise false.
 *
 * Note: Must be connected to WiFi to get time from NTP server.
 */
bool waitForSNTPSync(tm *timeInfo) {
  // Wait for SNTP synchronization to complete
  unsigned long timeout = millis() + NTP_TIMEOUT;
  if ((sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) &&
      (millis() < timeout)) {
    Serial.print(TXT_WAITING_FOR_SNTP);
    delay(100);  // ms
    while ((sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) &&
           (millis() < timeout)) {
      Serial.print(".");
      delay(100);  // ms
    }
    Serial.println();
  }
  return printLocalTime(timeInfo);
}  // waitForSNTPSync

int getWeatherNowcall(WiFiClientSecure &client, owm_resp_onecall_t &r) {
  int attempts = 0;
  bool rxSuccess = false;
  DeserializationError jsonErr = {};
  // String uri = "/data/" + OWM_ONECALL_VERSION
  //              + "/onecall?lat=" + LAT + "&lon=" + LON + "&lang=" + OWM_LANG
  //              + "&units=standard&exclude=minutely";
  // https://devapi.qweather.com/v7/weather/7d?location=101010100&key=c1ff043362be465b8f56486df917a35c
  // String uri = "/v7/weather/7d?location=" +LOCATION_CODE +"&lang=en";
  String uri = "/v7/weather/now?location=" + LOCATION_CODE + "&lang=en";

#if !DISPLAY_ALERTS
  // exclude alerts
  uri += ",alerts";
#endif

  // This string is printed to terminal to help with debugging. The API key is
  // censored to reduce the risk of users exposing their key.
  String sanitizedUri = OWM_ENDPOINT + uri + "&key={API key}";

  uri += "&key=" + OWM_APIKEY;
  String test_url = "https://" + OWM_ENDPOINT + uri;
  Serial.println("test url: " + test_url);
  //  String url = "https://devapi.heweather.net/v7/weather/3d?location=" +
  //  _reqLocation +            "&key=" + _requserKey + "&unit=" + _reqUnit +
  //  "&lang=" + _reqLang;

  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + sanitizedUri);
  int httpResponse = 0;
  while (!rxSuccess && attempts < 3) {
    wl_status_t connection_status = WiFi.status();
    if (connection_status != WL_CONNECTED) {
      // -512 offset distinguishes these errors from httpClient errors
      return -512 - static_cast<int>(connection_status);
    }

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);  // default 5000ms
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);         // default 5000ms
    // http.begin(client, OWM_ENDPOINT, OWM_PORT, uri);
    http.begin(client, test_url);

    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK) {
      int len = http.getSize();  // get length of document (is -1 when Server
                                 // sends no Content-Length header)
      static uint8_t buff[128 * 1] = {0};  // create buffer for read
      int offset = 0;                      // read all data from server
      while (http.connected() && (len > 0 || len == -1)) {
        size_t size = client.available();  // get available data size
        if (size) {
          int c = client.readBytes(
              buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          memcpy(_buffer + offset, buff, sizeof(uint8_t) * c);
          offset += c;
          if (len > 0) {
            len -= c;
          }
        }
        delay(1);
      }
      _bufferSize = offset;
      if (_bufferSize) {
        Serial.print("bufferSize:");
        Serial.println(_bufferSize, DEC);
        uint8_t *outBuf = NULL;
        size_t outLen = 0;
        ArduinoUZlib::decompress(_buffer, _bufferSize, outBuf,
                                 outLen);  // GZIP解压
        // // 输出解密后的数据到控制台。
        // Serial.write(outBuf, outLen);

        jsonErr = deserializenowQweather((const char *)outBuf, r);

        if (jsonErr)

        {
          // -256 offset distinguishes these errors from httpClient errors
          httpResponse = -256 - static_cast<int>(jsonErr.code());
        }
        free(outBuf);
      }
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " " +
                   getHttpResponsePhrase(httpResponse));
    ++attempts;
  }

  return httpResponse;
}

int getWeatherHourcall(WiFiClientSecure &client, owm_resp_onecall_t &r) {
  int attempts = 0;
  bool rxSuccess = false;
  DeserializationError jsonErr = {};
  // String uri = "/data/" + OWM_ONECALL_VERSION
  //              + "/onecall?lat=" + LAT + "&lon=" + LON + "&lang=" + OWM_LANG
  //              + "&units=standard&exclude=minutely";
  // https://devapi.qweather.com/v7/weather/7d?location=101010100&key=c1ff043362be465b8f56486df917a35c
  // String uri = "/v7/weather/7d?location=" +LOCATION_CODE +"&lang=en";
  String uri = "/v7/weather/24h?location=" + LOCATION_CODE + "&lang=en";
  ;

#if !DISPLAY_ALERTS
  // exclude alerts
  uri += ",alerts";
#endif

  // This string is printed to terminal to help with debugging. The API key is
  // censored to reduce the risk of users exposing their key.
  String sanitizedUri = OWM_ENDPOINT + uri + "&key={API key}";

  uri += "&key=" + OWM_APIKEY;
  String test_url = "https://" + OWM_ENDPOINT + uri;
  Serial.println("test url: " + test_url);

  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + sanitizedUri);
  int httpResponse = 0;
  while (!rxSuccess && attempts < 3) {
    wl_status_t connection_status = WiFi.status();
    if (connection_status != WL_CONNECTED) {
      // -512 offset distinguishes these errors from httpClient errors
      return -512 - static_cast<int>(connection_status);
    }

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);  // default 5000ms
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);         // default 5000ms
    // http.begin(client, OWM_ENDPOINT, OWM_PORT, uri);
    http.begin(client, test_url);

    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK) {
      int len = http.getSize();  // get length of document (is -1 when Server
                                 // sends no Content-Length header)
      static uint8_t buff[128 * 1] = {0};  // create buffer for read
      int offset = 0;                      // read all data from server
      while (http.connected() && (len > 0 || len == -1)) {
        size_t size = client.available();  // get available data size
        if (size) {
          int c = client.readBytes(
              buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          memcpy(_buffer + offset, buff, sizeof(uint8_t) * c);
          offset += c;
          if (len > 0) {
            len -= c;
          }
        }
        delay(1);
      }
      _bufferSize = offset;
      if (_bufferSize) {
        Serial.print("bufferSize:");
        Serial.println(_bufferSize, DEC);
        uint8_t *outBuf = NULL;
        size_t outLen = 0;
        ArduinoUZlib::decompress(_buffer, _bufferSize, outBuf,
                                 outLen);  // GZIP解压
        // // 输出解密后的数据到控制台。
        // Serial.write(outBuf, outLen);

        jsonErr = deserializeHourQweather((const char *)outBuf, r);

        if (jsonErr)

        {
          // -256 offset distinguishes these errors from httpClient errors
          httpResponse = -256 - static_cast<int>(jsonErr.code());
        }
        free(outBuf);
      }
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " " +
                   getHttpResponsePhrase(httpResponse));
    ++attempts;
  }

  return httpResponse;
}
/* Perform an HTTP GET request to OpenWeatherMap's "One Call" API
 * If data is received, it will be parsed and stored in the global variable
 * owm_onecall.
 *
 * Returns the HTTP Status Code.
 */
#ifdef USE_HTTP
int getOWMonecall(WiFiClient &client, owm_resp_onecall_t &r)
#else
int getOWMonecall(WiFiClientSecure &client, owm_resp_onecall_t &r)
#endif
{
  int attempts = 0;
  bool rxSuccess = false;
  DeserializationError jsonErr = {};
  // String uri = "/data/" + OWM_ONECALL_VERSION
  //              + "/onecall?lat=" + LAT + "&lon=" + LON + "&lang=" + OWM_LANG
  //              + "&units=standard&exclude=minutely";
  // https://devapi.qweather.com/v7/weather/7d?location=101010100&key=c1ff043362be465b8f56486df917a35c
  // String uri = "/v7/weather/7d?location=" +LOCATION_CODE +"&lang=en";
  String uri = "/v7/weather/7d?location=" + LOCATION_CODE + "&lang=en";

#if !DISPLAY_ALERTS
  // exclude alerts
  uri += ",alerts";
#endif

  // This string is printed to terminal to help with debugging. The API key is
  // censored to reduce the risk of users exposing their key.
  String sanitizedUri = OWM_ENDPOINT + uri + "&key={API key}";

  uri += "&key=" + OWM_APIKEY;
  String test_url = "https://" + OWM_ENDPOINT + uri;
  Serial.println("test url: " + test_url);
  //  String url = "https://devapi.heweather.net/v7/weather/3d?location=" +
  //  _reqLocation +            "&key=" + _requserKey + "&unit=" + _reqUnit +
  //  "&lang=" + _reqLang;

  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + sanitizedUri);
  int httpResponse = 0;
  while (!rxSuccess && attempts < 3) {
    wl_status_t connection_status = WiFi.status();
    if (connection_status != WL_CONNECTED) {
      // -512 offset distinguishes these errors from httpClient errors
      return -512 - static_cast<int>(connection_status);
    }

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);  // default 5000ms
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);         // default 5000ms
    // http.begin(client, OWM_ENDPOINT, OWM_PORT, uri);
    http.begin(client, test_url);

    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK) {
      int len = http.getSize();  // get length of document (is -1 when Server
                                 // sends no Content-Length header)
      static uint8_t buff[128 * 1] = {0};  // create buffer for read
      int offset = 0;                      // read all data from server
      while (http.connected() && (len > 0 || len == -1)) {
        size_t size = client.available();  // get available data size
        if (size) {
          int c = client.readBytes(
              buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          memcpy(_buffer + offset, buff, sizeof(uint8_t) * c);
          offset += c;
          if (len > 0) {
            len -= c;
          }
        }
        delay(1);
      }
      _bufferSize = offset;
      if (_bufferSize) {
        Serial.print("bufferSize:");
        Serial.println(_bufferSize, DEC);
        uint8_t *outBuf = NULL;
        size_t outLen = 0;
        ArduinoUZlib::decompress(_buffer, _bufferSize, outBuf,
                                 outLen);  // GZIP解压
        // // 输出解密后的数据到控制台。
        // Serial.write(outBuf, outLen);

        jsonErr = deserializeQweather((const char *)outBuf, r);

        if (jsonErr)

        {
          // -256 offset distinguishes these errors from httpClient errors
          httpResponse = -256 - static_cast<int>(jsonErr.code());
        }
        free(outBuf);
      }
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " " +
                   getHttpResponsePhrase(httpResponse));
    ++attempts;
  }

  return httpResponse;
}

/* Perform an HTTP GET request to OpenWeatherMap's "Air Pollution" API
 * If data is received, it will be parsed and stored in the global variable
 * owm_air_pollution.
 *
 * Returns the HTTP Status Code.
 */
#ifdef USE_HTTP
int getOWMairpollution(WiFiClient &client, owm_resp_air_pollution_t &r)
#else
int getOWMairpollution(WiFiClientSecure &client, owm_resp_air_pollution_t &r)
#endif
{
  int attempts = 0;
  bool rxSuccess = false;
  DeserializationError jsonErr = {};

  // set start and end to appropriate values so that the last 24 hours of air
  // pollution history is returned. Unix, UTC.
  time_t now;
  int64_t end = time(&now);
  // minus 1 is important here, otherwise we could get an extra hour of history
  int64_t start = end - ((3600 * OWM_NUM_AIR_POLLUTION) - 1);
  char endStr[22];
  char startStr[22];
  sprintf(endStr, "%lld", end);
  sprintf(startStr, "%lld", start);
  String uri = "/data/2.5/air_pollution/history?lat=" + LAT + "&lon=" + LON +
               "&start=" + startStr + "&end=" + endStr + "&key=" + OWM_APIKEY;
  // This string is printed to terminal to help with debugging. The API key is
  // censored to reduce the risk of users exposing their key.
  String sanitizedUri = OWM_ENDPOINT +
                        "/data/2.5/air_pollution/history?lat=" + LAT +
                        "&lon=" + LON + "&start=" + startStr +
                        "&end=" + endStr + "&key={API key}";

  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + sanitizedUri);
  int httpResponse = 0;
  while (!rxSuccess && attempts < 3) {
    wl_status_t connection_status = WiFi.status();
    if (connection_status != WL_CONNECTED) {
      // -512 offset distinguishes these errors from httpClient errors
      return -512 - static_cast<int>(connection_status);
    }

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);  // default 5000ms
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);         // default 5000ms
    http.begin(client, OWM_ENDPOINT, OWM_PORT, uri);
    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK) {
      jsonErr = deserializeAirQuality(http.getStream(), r);
      if (jsonErr) {
        // -256 offset to distinguishes these errors from httpClient errors
        httpResponse = -256 - static_cast<int>(jsonErr.code());
      }
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " " +
                   getHttpResponsePhrase(httpResponse));
    ++attempts;
  }

  return httpResponse;
}  // getOWMairpollution

int getQweatherairpollution(WiFiClientSecure &client,
                            owm_resp_air_pollution_t &r)
 {
  int attempts = 0;
  bool rxSuccess = false;
  DeserializationError jsonErr = {};

  // set start and end to appropriate values so that the last 24 hours of air
  // pollution history is returned. Unix, UTC.
  time_t now;
  int64_t end = time(&now);
  // minus 1 is important here, otherwise we could get an extra hour of history
  int64_t start = end - ((3600 * OWM_NUM_AIR_POLLUTION) - 1);
  char endStr[22];
  char startStr[22];
  sprintf(endStr, "%lld", end);
  sprintf(startStr, "%lld", start);

  String uri = "/v7/air/now?location=" + LOCATION_CODE + "&lang=en";

  // This string is printed to terminal to help with debugging. The API key is
  // censored to reduce the risk of users exposing their key.

  String sanitizedUri = OWM_ENDPOINT + uri + "&key={API key}";

  uri += "&key=" + OWM_APIKEY;
  String test_url = "https://" + OWM_ENDPOINT + uri;

  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + sanitizedUri);
  int httpResponse = 0;
  while (!rxSuccess && attempts < 3) {
    wl_status_t connection_status = WiFi.status();
    if (connection_status != WL_CONNECTED) {
      // -512 offset distinguishes these errors from httpClient errors
      return -512 - static_cast<int>(connection_status);
    }

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);  // default 5000ms
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);         // default 5000ms
    http.begin(client, test_url);
    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK) {
      int len = http.getSize();  // get length of document (is -1 when Server
                                 // sends no Content-Length header)
      static uint8_t buff[128 * 1] = {0};  // create buffer for read
      int offset = 0;                      // read all data from server
      while (http.connected() && (len > 0 || len == -1)) {
        size_t size = client.available();  // get available data size
        if (size) {
          int c = client.readBytes(
              buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          memcpy(_buffer + offset, buff, sizeof(uint8_t) * c);
          offset += c;
          if (len > 0) {
            len -= c;
          }
        }
        delay(1);
      }
      _bufferSize = offset;
      if (_bufferSize) {
        Serial.print("bufferSize:");
        Serial.println(_bufferSize, DEC);
        uint8_t *outBuf = NULL;
        size_t outLen = 0;
        ArduinoUZlib::decompress(_buffer, _bufferSize, outBuf,
                                 outLen);  // GZIP解压
        // // 输出解密后的数据到控制台。
        // Serial.write(outBuf, outLen);

        jsonErr = deserializeAirQualityQweather((const char *)outBuf, r);

        if (jsonErr)

        {
          // -256 offset distinguishes these errors from httpClient errors
          httpResponse = -256 - static_cast<int>(jsonErr.code());
        }
        free(outBuf);
      }
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " " +
                   getHttpResponsePhrase(httpResponse));
    ++attempts;
  }

  return httpResponse;
}  // getOWMairpollution

/* Prints debug information about heap usage.
 */
void printHeapUsage() {
  Serial.println("[debug] Heap Size       : " + String(ESP.getHeapSize()) +
                 " B");
  Serial.println("[debug] Available Heap  : " + String(ESP.getFreeHeap()) +
                 " B");
  Serial.println("[debug] Min Free Heap   : " + String(ESP.getMinFreeHeap()) +
                 " B");
  Serial.println("[debug] Max Allocatable : " + String(ESP.getMaxAllocHeap()) +
                 " B");
  return;
}
