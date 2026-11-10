#pragma once

#include <Arduino.h>

class CommandProcessor {
 public:
  String handle(const String& rawCommand) const;
};
