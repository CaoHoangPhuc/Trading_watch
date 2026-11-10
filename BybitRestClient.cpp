#include "BybitRestClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "Config.h"

namespace {
const char* kInstrumentsEndpoint = "/v5/market/instruments-info?category=option&baseCoin=BTC&limit=200";
const char* kUnderlyingEndpoint = "/v5/market/tickers?category=linear&symbol=BTCUSDT";
}

bool BybitRestClient::fetchBtcOptionTickers(OptionContract* contracts, size_t maxContracts,
                                            size_t& outCount, float& outUnderlyingPrice,
                                            String& err) const {
  outCount = 0;
  outUnderlyingPrice = 0.0f;
  String cursor = "";
  while (outCount < maxContracts) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    https.setConnectTimeout(Config::HTTP_CONNECT_TIMEOUT_MS);
    https.setTimeout(Config::HTTP_TIMEOUT_MS);
    String url = String(Config::BYBIT_REST) + kInstrumentsEndpoint;
    if (cursor.length() > 0) {
      url += "&cursor=" + cursor;
    }
    if (!https.begin(client, url)) {
      err = "REST begin failed";
      return false;
    }

    int httpCode = https.GET();
    if (httpCode != HTTP_CODE_OK) {
      err = "REST HTTP error: " + String(httpCode);
      https.end();
      return false;
    }

    String payload = https.getString();
    https.end();

    StaticJsonDocument<256> filter;
    filter["retCode"] = true;
    filter["retMsg"] = true;
    filter["result"]["nextPageCursor"] = true;
    filter["result"]["list"][0]["symbol"] = true;

    DynamicJsonDocument doc(96 * 1024);
    DeserializationError parseErr = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (parseErr) {
      err = String("REST JSON parse failed: ") + parseErr.c_str();
      return false;
    }

    int retCode = doc["retCode"] | -1;
    if (retCode != 0) {
      err = String("REST retCode != 0: ") + String(retCode);
      return false;
    }

    JsonArray list = doc["result"]["list"].as<JsonArray>();
    if (list.isNull()) {
      err = "REST result.list missing";
      return false;
    }

    for (JsonVariant row : list) {
      if (outCount >= maxContracts) {
        break;
      }

      OptionContract c;
      if (!parseOptionSymbol(row["symbol"].as<String>(), c)) {
        continue;
      }
      contracts[outCount++] = c;
    }

    cursor = doc["result"]["nextPageCursor"] | "";
    if (cursor.length() == 0 || list.size() == 0) {
      break;
    }

    // Yield between pages so IDLE tasks and background networking can run.
    vTaskDelay(1);
  }

  fetchBtcUnderlyingPrice(outUnderlyingPrice);

  if (outCount == 0) {
    err = "No BTC option contracts parsed";
    return false;
  }

  return true;
}

bool BybitRestClient::fetchBtcUnderlyingPrice(float& outUnderlyingPrice) const {
  outUnderlyingPrice = 0.0f;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.setConnectTimeout(Config::HTTP_CONNECT_TIMEOUT_MS);
  https.setTimeout(Config::HTTP_TIMEOUT_MS);
  String url = String(Config::BYBIT_REST) + kUnderlyingEndpoint;
  if (!https.begin(client, url)) {
    return false;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    https.end();
    return false;
  }

  String payload = https.getString();
  https.end();

  StaticJsonDocument<256> filter;
  filter["retCode"] = true;
  filter["result"]["list"][0]["lastPrice"] = true;

  DynamicJsonDocument doc(4096);
  DeserializationError parseErr = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (parseErr) {
    return false;
  }

  if ((doc["retCode"] | -1) != 0) {
    return false;
  }

  String lastPrice = doc["result"]["list"][0]["lastPrice"] | "";
  if (lastPrice.length() == 0) {
    return false;
  }

  outUnderlyingPrice = lastPrice.toFloat();
  return outUnderlyingPrice > 0.0f;
}

bool BybitRestClient::parseOptionSymbol(const String& symbol, OptionContract& outContract) {
  int p1 = symbol.indexOf('-');
  if (p1 <= 0) return false;
  int p2 = symbol.indexOf('-', p1 + 1);
  if (p2 <= p1 + 1) return false;
  int p3 = symbol.indexOf('-', p2 + 1);
  if (p3 <= p2 + 1) return false;

  String expiry = symbol.substring(p1 + 1, p2);
  String strikeToken = symbol.substring(p2 + 1, p3);
  String sideToken = symbol.substring(p3 + 1);

  if (expiry.length() < 7 || strikeToken.length() == 0 || sideToken.length() == 0) {
    return false;
  }

  int day = expiry.substring(0, 2).toInt();
  String mon = expiry.substring(2, 5);
  mon.toUpperCase();
  int month = monthFromToken(mon);
  int year = 2000 + expiry.substring(5).toInt();
  if (month <= 0 || day <= 0 || year < 2020) {
    return false;
  }

  outContract.symbol = symbol;
  outContract.expiry = expiry;
  outContract.strike = strikeToken.toFloat();
  outContract.side = sideToken[0];
  outContract.day = day;
  outContract.month = month;
  outContract.year = year;

  return true;
}

int BybitRestClient::monthFromToken(const String& mon) {
  if (mon == "JAN") return 1;
  if (mon == "FEB") return 2;
  if (mon == "MAR") return 3;
  if (mon == "APR") return 4;
  if (mon == "MAY") return 5;
  if (mon == "JUN") return 6;
  if (mon == "JUL") return 7;
  if (mon == "AUG") return 8;
  if (mon == "SEP") return 9;
  if (mon == "OCT") return 10;
  if (mon == "NOV") return 11;
  if (mon == "DEC") return 12;
  return 0;
}
