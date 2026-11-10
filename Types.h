#pragma once

#include <Arduino.h>

struct OptionContract {
  String symbol;
  String expiry;
  float strike;
  char side;
  int year;
  int month;
  int day;
};

struct TickerState {
  String symbol;
  String lastPrice;
  String bid1Price;
  String ask1Price;
  String markPrice;
  String indexPrice;
  String underlyingPrice;
  String delta;
  String gamma;
  String vega;
  String theta;
  unsigned long updatedAtMs;
  bool valid;
};
