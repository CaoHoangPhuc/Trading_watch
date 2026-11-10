#include "CommandProcessor.h"

#include <ArduinoJson.h>

String CommandProcessor::handle(const String& rawCommand) const {
  String command = rawCommand;
  command.trim();
  command.toLowerCase();

  DynamicJsonDocument doc(1024);
  doc["type"] = "command_response";

  if (command == "help") {
    doc["ok"] = true;
    doc["command"] = "help";
    doc["docs_ref"] = "README.md#dashboard-command-channel";

    JsonArray commands = doc.createNestedArray("commands");
    JsonObject cmd = commands.createNestedObject();
    cmd["name"] = "help";
    cmd["description"] = "Show available dashboard commands";

    String out;
    serializeJson(doc, out);
    return out;
  }

  doc["ok"] = false;
  doc["command"] = rawCommand;
  doc["error"] = "Unknown command";
  doc["suggestion"] = "Run help";
  doc["docs_ref"] = "README.md#dashboard-command-channel";

  String out;
  serializeJson(doc, out);
  return out;
}
