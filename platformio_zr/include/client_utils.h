/*
 * @Author: zouzirun zouzirun@163.com
 * @Date: 2024-06-29 16:01:51
 * @LastEditors: zouzirun zouzirun@163.com
 * @LastEditTime: 2024-07-10 17:01:39
 * @FilePath: \platformio_zr\include\client_utils.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/* Client side utility declarations for esp32-weather-epd.
 * Copyright (C) 2022-2023  Luke Marzen
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

#ifndef __CLIENT_UTILS_H__
#define __CLIENT_UTILS_H__

#include <Arduino.h>
#include "api_response.h"
#include "config.h"
#ifdef USE_HTTP
  #include <WiFiClient.h>
#else
  #include <WiFiClientSecure.h>
#endif

wl_status_t startWiFi(int &wifiRSSI);
void killWiFi();
bool waitForSNTPSync(tm *timeInfo);
bool printLocalTime(tm *timeInfo);
#ifdef USE_HTTP
  int getOWMonecall(WiFiClient &client, owm_resp_onecall_t &r);
  int getOWMairpollution(WiFiClient &client, owm_resp_air_pollution_t &r);
#else
  int getOWMonecall(WiFiClientSecure &client, owm_resp_onecall_t &r);
  int getOWMairpollution(WiFiClientSecure &client, owm_resp_air_pollution_t &r);
#endif
int getQweatherairpollution(WiFiClientSecure &client, owm_resp_air_pollution_t &r);

int getWeatherNowcall(WiFiClientSecure &client, owm_resp_onecall_t &r);
int getWeatherHourcall(WiFiClientSecure &client, owm_resp_onecall_t &r);

#endif

