#include <ArduinoJson.hpp>
#include <ArduinoJson.h>

void SerializeObject()
{
    String json;
    StaticJsonDocument<300> doc;
    doc["SSID"] = "L"; //Puede no hay
    doc["receiver address"] = "%02x:%02x:%02x:%02x:%02x:%02x"; //Simepre hay
    doc["sender address"] = "%02x:%02x:%02x:%02x:%02x:%02x"; // Siempre hay
    doc["filtering address"] = "%02x:%02x:%02x:%02x:%02x:%02x"; //Puede qye no hay
    doc["Hash"] = "qwerty"; //Siempre hay
    doc["RSSI"] = 0; //Puede que no hay
    doc["SN"] = 123456;
    doc["TimeStamp"] =1234:
    doc["channel"] = 0; // Simepre hay1-27
    doc["latitude"] = 0; // Simepre hay
    doc["longitud"] = 0; //Siempre hay

    serializeJson(doc, json);
    Serial.println(json);
}


void setup()
{
    Serial.begin(115200);

    Serial.println("===== Object Example =====");
    Serial.println("-- Serialize --");
    SerializeObject();
}

void loop()
{
}