/* API response deserialization for esp32-weather-epd.
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

#include "api_response.h"

#include <ArduinoJson.h>

#include <vector>

#include "config.h"

DeserializationError deserializenowQweather(String sjson,
                                            owm_resp_onecall_t &r) {
  int i;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, sjson);

  if (error) {
    return error;
  }

  JsonObject current = doc["now"];

  String obsTime = current["obsTime"].as<const char *>();
  struct tm tm;
  //"2024-07-10T15:00+08:00"
  sscanf(obsTime.c_str(), "%d-%d-%dT%d:%d", &tm.tm_year, &tm.tm_mon,
         &tm.tm_mday, &tm.tm_hour, &tm.tm_min);
  tm.tm_year = tm.tm_year - 1900;
  tm.tm_mon = tm.tm_mon - 1;
  tm.tm_isdst = -1;
  tm.tm_sec = 13;
  r.current.dt = mktime(&tm);

  // String stemp = daily["sunrise"].as<const char *>();

  // r.current.dt = current["dt"].as<int64_t>();
  // r.current.sunrise = current["sunrise"].as<int64_t>();
  // r.current.sunset = current["sunset"].as<int64_t>();
  String stemp = current["temp"].as<const char *>();
  r.current.temp = stemp.toInt() + 273.15;
  stemp = current["feelsLike"].as<const char *>();
  r.current.feels_like = (float)(stemp.toInt() + 273.15);

  stemp = current["pressure"].as<const char *>();
  r.current.pressure = stemp.toInt();

  stemp = current["humidity"].as<const char *>();
  r.current.humidity = stemp.toInt();

  stemp = current["cloud"].as<const char *>();
  r.current.clouds = stemp.toInt();

  stemp = current["vis"].as<const char *>();
  r.current.visibility = (float)(stemp.toInt() + 273.15);
  stemp = current["dew"].as<const char *>();
  r.current.dew_point = stemp.toInt();

  stemp = current["windSpeed"].as<const char *>();
  r.current.wind_speed = (float)(stemp.toInt() / 3.6);
  stemp = current["wind360"].as<const char *>();
  r.current.wind_deg = stemp.toInt();

  //  r.current.wind_gust = current["wind_gust"].as<float>();
  //  r.current.rain_1h = current["rain"]["1h"].as<float>();
  // r.current.snow_1h = current["snow"]["1h"].as<float>();
  // r.current.weather.main = current_weather["main"].as<const char *>();
  // r.current.weather.description = current["text"].as<const char *>();
  stemp = current["icon"].as<const char *>();
  r.current.weather.id = stemp.toInt();
  // r.current.weather.icon = current_weather["icon"].as<const char *>();

  return error;
}

DeserializationError deserializeHourQweather(String sjson,
                                             owm_resp_onecall_t &r) {
  int i;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, sjson);

  if (error) {
    return error;
  }

  Serial.println("deserialize hour ok");
  struct tm tm;
  String fxTime;
  String stemp;
  i = 0;
  for (JsonObject hourly : doc["hourly"].as<JsonArray>()) {
    fxTime = hourly["fxTime"].as<const char *>();
    //"2024-07-10T15:00+08:00"
    sscanf(fxTime.c_str(), "%d-%d-%dT%d:%d", &tm.tm_year, &tm.tm_mon,
           &tm.tm_mday, &tm.tm_hour, &tm.tm_min);
    tm.tm_year = tm.tm_year - 1900;
    tm.tm_mon = tm.tm_mon - 1;
    tm.tm_isdst = -1;
    tm.tm_sec = 13;
    r.hourly[i].dt = mktime(&tm);
    stemp = hourly["temp"].as<const char *>();
    r.hourly[i].temp = stemp.toInt() + 273.15;
    stemp = hourly["icon"].as<const char *>();
    r.hourly[i].weather.id = stemp.toInt();

    stemp = hourly["windSpeed"].as<const char *>();
    r.hourly[i].wind_speed = (float)(stemp.toInt() / 3.6);
    stemp = hourly["wind360"].as<const char *>();
    r.hourly[i].wind_deg = stemp.toInt();
    stemp = hourly["pop"].as<const char *>();
    r.hourly[i].pop = (float)(stemp.toInt() / 100.0);
#if 0
    stemp = hourly["humidity"].as<const char *>();
    r.hourly[i].humidity = stemp.toInt();
  
    stemp = hourly["pressure"].as<const char *>();
    r.hourly[i].pressure = stemp.toInt();
    stemp = hourly["cloud"].as<const char *>();
    r.hourly[i].clouds = stemp.toInt();
    stemp = hourly["dew"].as<const char *>();
    r.hourly[i].dew_point = stemp.toInt();
#endif
    if (i == OWM_NUM_HOURLY - 1) {
      break;
    }
    ++i;
  }

  // String stemp = daily["sunrise"].as<const char *>();

  // r.current.dt = current["dt"].as<int64_t>();
  // r.current.sunrise = current["sunrise"].as<int64_t>();
  // r.current.sunset = current["sunset"].as<int64_t>();

  // r.current.weather.icon = current_weather["icon"].as<const char *>();

  return error;
}  // end deserializeOneCall

DeserializationError deserializeQweather(String sjson, owm_resp_onecall_t &r) {
  int i;
  // DynamicJsonDocument doc(8192);
  // deserializeJson(doc, payload);
  // vector<GeoInfo> result;

  // JsonDocument filter;
  // filter["current"]  = false;
  // filter["minutely"] = false;
  // filter["hourly"]   = false;
  // filter["daily"]    = true;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, sjson);

  if (error) {
    return error;
  }
 
  i = 0;
  for (JsonObject daily : doc["daily"].as<JsonArray>()) {
    // r.daily[i].dt = daily["dt"].as<int64_t>();
    String fxDate = daily["fxDate"].as<const char *>();
    struct tm tm;
    sscanf(fxDate.c_str(), "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
    tm.tm_year = tm.tm_year - 1900;
    tm.tm_mon = tm.tm_mon - 1;
    tm.tm_isdst = -1;

    String stemp = daily["sunrise"].as<const char *>();
    sscanf(stemp.c_str(), "%d:%d", &tm.tm_hour, &tm.tm_min);
    tm.tm_sec = 13;
    r.daily[i].sunrise = mktime(&tm);

    stemp = daily["sunset"].as<const char *>();
    sscanf(stemp.c_str(), "%d:%d", &tm.tm_hour, &tm.tm_min);
    tm.tm_sec = 13;
    r.daily[i].sunset = mktime(&tm);
    /************ */
    stemp = daily["moonrise"].as<const char *>();
    sscanf(stemp.c_str(), "%d:%d", &tm.tm_hour, &tm.tm_min);
    tm.tm_sec = 13;
    r.daily[i].moonrise = mktime(&tm);
    /************ */
    stemp = daily["moonset"].as<const char *>();
    sscanf(stemp.c_str(), "%d:%d", &tm.tm_hour, &tm.tm_min);
    tm.tm_sec = 13;
    r.daily[i].moonset = mktime(&tm);
    /************ */
    stemp = daily["tempMax"].as<const char *>();
    r.daily[i].temp.max = stemp.toInt() + 273.15;
    stemp = daily["tempMin"].as<const char *>();
    r.daily[i].temp.min = stemp.toInt() + 273.15;

    stemp = daily["pressure"].as<const char *>();
    r.daily[i].pressure = stemp.toInt();
    stemp = daily["humidity"].as<const char *>();
    r.daily[i].humidity = stemp.toInt();
    stemp = daily["vis"].as<const char *>();
    r.daily[i].visibility = stemp.toInt() * 1000;
    stemp = daily["uvIndex"].as<const char *>();
    r.daily[i].uvi = stemp.toInt();
     stemp = daily["windSpeedDay"].as<const char *>();
    r.daily[i].wind_speed = (float)(stemp.toInt() / 3.6);
    r.daily[i].wind_gust = stemp.toInt();
    stemp = daily["wind360Day"].as<const char *>();
    r.daily[i].wind_deg = stemp.toInt();
    stemp = daily["iconDay"].as<const char *>();
    r.daily[i].weather.id = stemp.toInt();
    stemp = daily["cloud"].as<const char *>();
    Serial.println(String(r.daily[i].weather.id ));
    r.daily[i].clouds = stemp.toInt();

    // r.daily[i].wind_speed = daily["windSpeedDay"].as<float>();
    // r.daily[i].wind_gust = daily["wind_gust"].as<float>();
    // r.daily[i].wind_deg = daily["wind360Day"].as<int>();

    // // r.daily[i].dew_point  = daily["dew_point"] .as<float>();

    // r.daily[i].pop = daily["pop"].as<float>();
    // r.daily[i].rain = daily["rain"].as<float>();
    // r.daily[i].snow = daily["snow"].as<float>();
    // // JsonObject daily_weather = daily["weather"][0];

    // // r.daily[i].weather.main        = daily_weather["main"]       .as<const
    // // char *>(); r.daily[i].weather.description =
    // // daily_weather["description"].as<const char *>();
    // r.daily[i].weather.icon
    // // = daily_weather["icon"]       .as<const char *>();

    if (i == OWM_NUM_DAILY - 1) {
      break;
    }
    ++i;
  }

  //  r.current.dt = current["dt"].as<int64_t>();
  r.current.sunrise = r.daily[0].sunrise;
  r.current.sunset = r.daily[0].sunset;
  // r.current.dew_point = current["dew_point"].as<float>();
  r.current.uvi = r.daily[0].uvi;
  // r.current.wind_gust = current["wind_gust"].as<float>();
  // r.current.rain_1h = current["rain"]["1h"].as<float>();
  // r.current.snow_1h = current["snow"]["1h"].as<float>();
  // r.current.weather.main = current_weather["main"].as<const char *>();
  // r.current.weather.description = current_weather["description"].as<const
  // char *>(); r.current.weather.icon = current_weather["icon"].as<const char
  // *>();

  // JsonArray dailyArray = doc["daily"];
  //           for (JsonVariant v : dailyArray)
  //           {
  //               JsonObject daily = v.as<JsonObject>();
  //               DailyWeather dw;
  //               dw.fxDate = daily["fxDate"].as<String>();
  //               dw.sunrise = daily["sunrise"].as<String>();
  //               dw.sunset = daily["sunset"].as<String>();
  //               dw.moonrise = daily["moonrise"].as<String>();
  //               dw.moonset = daily["moonset"].as<String>();
  //               dw.moonPhase = daily["moonPhase"].as<String>();
  //               dw.tempMax = daily["tempMax"].as<String>();
  //               dw.tempMin = daily["tempMin"].as<String>();
  //               dw.Day.icon = daily["iconDay"].as<String>();
  //               dw.Day.text = daily["textDay"].as<String>();
  //               dw.Day.wind360 = daily["wind360Day"].as<String>();
  //               dw.Day.windDir = daily["windDirDay"].as<String>();
  //               dw.Day.windScale = daily["windScaleDay"].as<String>();
  //               dw.Day.windSpeed = daily["windSpeedDay"].as<String>();
  //               dw.Night.icon = daily["iconNight"].as<String>();
  //               dw.Night.text = daily["textNight"].as<String>();
  //               dw.Night.wind360 = daily["wind360Night"].as<String>();
  //               dw.Night.windDir = daily["windDirNight"].as<String>();
  //               dw.Night.windScale = daily["windScaleNight"].as<String>();
  //               dw.Night.windSpeed = daily["windSpeedNight"].as<String>();
  //               dw.humidity = daily["humidity"].as<String>();
  //               dw.precip = daily["precip"].as<String>();
  //               dw.pressure = daily["pressure"].as<String>();
  //               dw.vis = daily["vis"].as<String>();
  //               dw.cloud = daily["cloud"].as<String>();
  //               dw.uvIndex = daily["uvIndex"].as<String>();
  //               result.push_back(dw);
  //           }

  return error;
}  // end deserializeOneCall

DeserializationError deserializeOneCall(WiFiClient &json,
                                        owm_resp_onecall_t &r) {
  int i;

  JsonDocument filter;
  filter["current"] = false;
  filter["minutely"] = false;
  filter["hourly"] = false;
  filter["daily"] = true;
#if !DISPLAY_ALERTS
  filter["alerts"] = false;
#else
  // description can be very long so they are filtered out to save on memory
  // along with sender_name
  for (int i = 0; i < OWM_NUM_ALERTS; ++i) {
    filter["alerts"][i]["sender_name"] = false;
    filter["alerts"][i]["event"] = true;
    filter["alerts"][i]["start"] = true;
    filter["alerts"][i]["end"] = true;
    filter["alerts"][i]["description"] = false;
    filter["alerts"][i]["tags"] = true;
  }
#endif

  JsonDocument doc;

  DeserializationError error =
      deserializeJson(doc, json, DeserializationOption::Filter(filter));
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] doc.overflowed() : " + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) {
    return error;
  }

  r.lat = doc["lat"].as<float>();
  r.lon = doc["lon"].as<float>();
  r.timezone = doc["timezone"].as<const char *>();
  r.timezone_offset = doc["timezone_offset"].as<int>();

  JsonObject current = doc["current"];
  r.current.dt = current["dt"].as<int64_t>();
  r.current.sunrise = current["sunrise"].as<int64_t>();
  r.current.sunset = current["sunset"].as<int64_t>();
  r.current.temp = current["temp"].as<float>();
  r.current.feels_like = current["feels_like"].as<float>();
  r.current.pressure = current["pressure"].as<int>();
  r.current.humidity = current["humidity"].as<int>();
  r.current.dew_point = current["dew_point"].as<float>();
  r.current.clouds = current["clouds"].as<int>();
  r.current.uvi = current["uvi"].as<float>();
  r.current.visibility = current["visibility"].as<int>();
  r.current.wind_speed = current["wind_speed"].as<float>();
  r.current.wind_gust = current["wind_gust"].as<float>();
  r.current.wind_deg = current["wind_deg"].as<int>();
  r.current.rain_1h = current["rain"]["1h"].as<float>();
  r.current.snow_1h = current["snow"]["1h"].as<float>();
  JsonObject current_weather = current["weather"][0];
  r.current.weather.id = current_weather["id"].as<int>();
  r.current.weather.main = current_weather["main"].as<const char *>();
  r.current.weather.description =
      current_weather["description"].as<const char *>();
  r.current.weather.icon = current_weather["icon"].as<const char *>();

  // minutely forecast is currently unused
  // i = 0;
  // for (JsonObject minutely : doc["minutely"].as<JsonArray>())
  // {
  //   r.minutely[i].dt            = minutely["dt"]           .as<int64_t>();
  //   r.minutely[i].precipitation = minutely["precipitation"].as<float>();

  //   if (i == OWM_NUM_MINUTELY - 1)
  //   {
  //     break;
  //   }
  //   ++i;
  // }

  i = 0;
  for (JsonObject hourly : doc["hourly"].as<JsonArray>()) {
    r.hourly[i].dt = hourly["dt"].as<int64_t>();
    r.hourly[i].temp = hourly["temp"].as<float>();
    r.hourly[i].feels_like = hourly["feels_like"].as<float>();
    r.hourly[i].pressure = hourly["pressure"].as<int>();
    r.hourly[i].humidity = hourly["humidity"].as<int>();
    r.hourly[i].dew_point = hourly["dew_point"].as<float>();
    r.hourly[i].clouds = hourly["clouds"].as<int>();
    r.hourly[i].uvi = hourly["uvi"].as<float>();
    r.hourly[i].visibility = hourly["visibility"].as<int>();
    r.hourly[i].wind_speed = hourly["wind_speed"].as<float>();
    r.hourly[i].wind_gust = hourly["wind_gust"].as<float>();
    r.hourly[i].wind_deg = hourly["wind_deg"].as<int>();
    r.hourly[i].pop = hourly["pop"].as<float>();
    r.hourly[i].rain_1h = hourly["rain"]["1h"].as<float>();
    r.hourly[i].snow_1h = hourly["snow"]["1h"].as<float>();
    // JsonObject hourly_weather = hourly["weather"][0];
    // r.hourly[i].weather.id          = hourly_weather["id"] .as<int>();
    // r.hourly[i].weather.main        = hourly_weather["main"]       .as<const
    // char *>(); r.hourly[i].weather.description =
    // hourly_weather["description"].as<const char *>();
    // r.hourly[i].weather.icon        = hourly_weather["icon"]       .as<const
    // char *>();

    if (i == OWM_NUM_HOURLY - 1) {
      break;
    }
    ++i;
  }

  i = 0;
  for (JsonObject daily : doc["daily"].as<JsonArray>()) {
    r.daily[i].dt = daily["dt"].as<int64_t>();
    r.daily[i].sunrise = daily["sunrise"].as<int64_t>();
    r.daily[i].sunset = daily["sunset"].as<int64_t>();
    r.daily[i].moonrise = daily["moonrise"].as<int64_t>();
    r.daily[i].moonset = daily["moonset"].as<int64_t>();
    r.daily[i].moon_phase = daily["moon_phase"].as<float>();
    JsonObject daily_temp = daily["temp"];
    // r.daily[i].temp.morn  = daily_temp["morn"] .as<float>();
    // r.daily[i].temp.day   = daily_temp["day"]  .as<float>();
    // r.daily[i].temp.eve   = daily_temp["eve"]  .as<float>();
    // r.daily[i].temp.night = daily_temp["night"].as<float>();
    r.daily[i].temp.min = daily_temp["min"].as<float>();
    r.daily[i].temp.max = daily_temp["max"].as<float>();
    // JsonObject daily_feels_like = daily["feels_like"];
    // r.daily[i].feels_like.morn  = daily_feels_like["morn"] .as<float>();
    // r.daily[i].feels_like.day   = daily_feels_like["day"]  .as<float>();
    // r.daily[i].feels_like.eve   = daily_feels_like["eve"]  .as<float>();
    // r.daily[i].feels_like.night = daily_feels_like["night"].as<float>();
    r.daily[i].pressure = daily["pressure"].as<int>();
    r.daily[i].humidity = daily["humidity"].as<int>();
    // r.daily[i].dew_point  = daily["dew_point"] .as<float>();
    r.daily[i].clouds = daily["cloud"].as<int>();
    r.daily[i].uvi = daily["uvIndex"].as<float>();
    r.daily[i].visibility = daily["vis"].as<int>();
    r.daily[i].wind_speed = daily["windSpeedDay"].as<float>();
    r.daily[i].wind_gust = daily["wind_gust"].as<float>();
    r.daily[i].wind_deg = daily["wind360Day"].as<int>();
    r.daily[i].pop = daily["pop"].as<float>();
    r.daily[i].rain = daily["rain"].as<float>();
    r.daily[i].snow = daily["snow"].as<float>();
    // JsonObject daily_weather = daily["weather"][0];
    // r.daily[i].weather.id          = daily_weather["id"]         .as<int>();
    // r.daily[i].weather.main        = daily_weather["main"]       .as<const
    // char *>(); r.daily[i].weather.description =
    // daily_weather["description"].as<const char *>(); r.daily[i].weather.icon
    // = daily_weather["icon"]       .as<const char *>();

    if (i == OWM_NUM_DAILY - 1) {
      break;
    }
    ++i;
  }

#if DISPLAY_ALERTS
  i = 0;
  for (JsonObject alerts : doc["alerts"].as<JsonArray>()) {
    owm_alerts_t new_alert = {};
    // new_alert.sender_name = alerts["sender_name"].as<const char *>();
    new_alert.event = alerts["event"].as<const char *>();
    new_alert.start = alerts["start"].as<int64_t>();
    new_alert.end = alerts["end"].as<int64_t>();
    // new_alert.description = alerts["description"].as<const char *>();
    new_alert.tags = alerts["tags"][0].as<const char *>();
    r.alerts.push_back(new_alert);

    if (i == OWM_NUM_ALERTS - 1) {
      break;
    }
    ++i;
  }
#endif

  return error;
}  // end deserializeOneCall
DeserializationError deserializeAirQuality(WiFiClient &json,
                                           owm_resp_air_pollution_t &r) {
  int i = 0;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json);
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] doc.overflowed() : " + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) {
    return error;
  }

  r.coord.lat = doc["coord"]["lat"].as<float>();
  r.coord.lon = doc["coord"]["lon"].as<float>();

  for (JsonObject list : doc["list"].as<JsonArray>()) {
    r.main_aqi[i] = list["main"]["aqi"].as<int>();

    JsonObject list_components = list["components"];
    r.components.co[i] = list_components["co"].as<float>();
    r.components.no[i] = list_components["no"].as<float>();
    r.components.no2[i] = list_components["no2"].as<float>();
    r.components.o3[i] = list_components["o3"].as<float>();
    r.components.so2[i] = list_components["so2"].as<float>();
    r.components.pm2_5[i] = list_components["pm2_5"].as<float>();
    r.components.pm10[i] = list_components["pm10"].as<float>();
    r.components.nh3[i] = list_components["nh3"].as<float>();

    r.dt[i] = list["dt"].as<int64_t>();

    if (i == OWM_NUM_AIR_POLLUTION - 1) {
      break;
    }
    ++i;
  }

  return error;
}  // end deserializeAirQuality

DeserializationError deserializeAirQualityQweather(
    String sjson, owm_resp_air_pollution_t &r) {
 
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, sjson);

  if (error) {
    return error;
  }
  JsonObject now_air = doc["now"];

  struct tm tm;
  String stemp;
  // i = 0;
  //   //"2024-07-10T15:00+08:00"

  String pubTime = now_air["pubTime"].as<const char *>();
  sscanf(pubTime.c_str(), "%d-%d-%dT%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
         &tm.tm_hour, &tm.tm_min);
  tm.tm_year = tm.tm_year - 1900;
  tm.tm_mon = tm.tm_mon - 1;
  tm.tm_isdst = -1;
  tm.tm_sec = 13;
  r.dt[0] = mktime(&tm);

  stemp = now_air["aqi"].as<const char *>();
  r.main_aqi[0] = stemp.toInt();

  stemp = now_air["pm10"].as<const char *>();
  r.components.pm10[0] = stemp.toFloat();
  stemp = now_air["pm2p5"].as<const char *>();
  r.components.pm2_5[0] = stemp.toFloat();

  stemp = now_air["no2"].as<const char *>();
  r.components.no2[0] = stemp.toFloat();

  stemp = now_air["so2"].as<const char *>();
  r.components.so2[0] = stemp.toFloat();

  stemp = now_air["co"].as<const char *>();
  r.components.co[0] = stemp.toFloat();

  stemp = now_air["o3"].as<const char *>();
  r.components.o3[0] = stemp.toFloat();

  // r.coord.lat = doc["coord"]["lat"].as<float>();
  // r.coord.lon = doc["coord"]["lon"].as<float>();

  // for (JsonObject list : doc["list"].as<JsonArray>()) {
  //   r.main_aqi[i] = list["main"]["aqi"].as<int>();

  //   JsonObject list_components = list["components"];
  //   r.components.co[i] = list_components["co"].as<float>();
  //   r.components.no[i] = list_components["no"].as<float>();
  //   r.components.no2[i] = list_components["no2"].as<float>();
  //   r.components.o3[i] = list_components["o3"].as<float>();
  //   r.components.so2[i] = list_components["so2"].as<float>();
  //   r.components.pm2_5[i] = list_components["pm2_5"].as<float>();
  //   r.components.pm10[i] = list_components["pm10"].as<float>();
  //   r.components.nh3[i] = list_components["nh3"].as<float>();

  //   r.dt[i] = list["dt"].as<int64_t>();

  //   if (i == OWM_NUM_AIR_POLLUTION - 1) {
  //     break;
  //   }
  //   ++i;
  // }

  return error;
}  // end deserializeAirQuality