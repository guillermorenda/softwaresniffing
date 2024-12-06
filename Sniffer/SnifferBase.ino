#include <WiFi.h>
#include "esp_wifi.h"
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Define the boot button pin (usually GPIO0 on many ESP32 boards)
#define BOOT_BUTTON_PIN 0

// SIM7000G GPS serial interface
SoftwareSerial ss(21, 22); // RX, TX (Use GPIO16 for RX and GPIO17 for TX)

// GPS object
TinyGPSPlus gps;

// Variable to track whether the program is running or paused
bool isPaused = false;
unsigned long lastDebounceTime = 0; // To handle debounce
unsigned long debounceDelay = 200;  // debounce delay in milliseconds
unsigned long lastChannelSwitchTime = 0;  // Time when the channel was last switched
unsigned long channelSwitchInterval = 5000;  // Time interval to switch channels (5 seconds)
int currentChannel = 1;  // Start from channel 1

void setup() {
  // Start the serial communication for debugging
  Serial.begin(115200);

  // Initialize the boot button pin as input (with pull-up)
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP); // Internal pull-up resistor

  // Start the WiFi module in promiscuous mode
  WiFi.mode(WIFI_STA);  // Set to Station mode, necessary for promiscuous mode
  esp_wifi_set_promiscuous(true);  // Enable promiscuous mode

  // Register the callback function that processes the captured packets
  esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);

  // Initialize SIM7000 GPS module
  ss.begin(9600); // SIM7000G baud rate for GPS is typically 9600

  Serial.println("ESP32 Sniffer started...");
}

void loop() {
  // Check if the button is pressed (LOW because of the pull-up resistor)
  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    // Check if enough time has passed to debounce the button press
    if (millis() - lastDebounceTime > debounceDelay) {
      // Toggle the pause state
      isPaused = !isPaused;
      
      // Update debounce time
      lastDebounceTime = millis();

      // Print to Serial Monitor to show the current state
      if (isPaused) {
        Serial.println("Execution Paused");
      } else {
        Serial.println("Execution Resumed");
      }
    }
  }

  // If the program is paused, do nothing and just keep checking for button press
  if (isPaused) {
    return;  // This will skip the rest of the loop
  }

  // Read GPS data from the SIM7000G module
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  // If a valid GPS fix is available, get the GPS data
  String gpsData = "";
  if (gps.location.isUpdated()) {
    gpsData = "Latitude=" + String(gps.location.lat(), 6) + ", Longitude=" + String(gps.location.lng(), 6);
  } else {
    gpsData = "No GPS fix";
  }

  // Iterate over channels every 'channelSwitchInterval' milliseconds
  if (millis() - lastChannelSwitchTime >= channelSwitchInterval) {
    // Switch to the next Wi-Fi channel (1-13)
    currentChannel = (currentChannel % 13) + 1;  // Cycle through channels 1 to 13
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);  // Set the Wi-Fi channel
    Serial.print("Switching to channel: ");
    Serial.println(currentChannel);
    lastChannelSwitchTime = millis();  // Update the last switch time
  }
}

// Callback function that will be triggered when packets are captured
void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;

  // Only handle management packets (this can be modified for other types)
  if (type == WIFI_PKT_MGMT) {
    // Extract the RSSI (signal strength) of the packet
    int rssi = packet->rx_ctrl.rssi;

    // Extract the sender MAC address (Source MAC)
    uint8_t *senderMac = packet->payload + 10;

    // Extract the receiver MAC address (Destination MAC)
    uint8_t *receiverMac = packet->payload + 4;

    // Extract the SSID (if available)
    String ssid = extractSSID(packet->payload, packet->rx_ctrl.sig_len); // Use sig_len instead of len

    // Get the timestamp in milliseconds
    unsigned long timestamp = millis();

    // Prepare the JSON object
    StaticJsonDocument<500> doc;

    // Add fields to the JSON object
    doc["timestamp"] = timestamp;
    doc["sender_mac"] = macToStr(senderMac);
    doc["receiver_mac"] = macToStr(receiverMac);
    doc["rssi"] = rssi;
    doc["ssid"] = ssid;  // Add the SSID to the JSON

    // Add the current channel to the JSON object
    doc["channel"] = currentChannel;

    // Add GPS data to the JSON
    String gpsData = "";
    if (gps.location.isUpdated()) {
      gpsData = "Latitude=" + String(gps.location.lat(), 6) + ", Longitude=" + String(gps.location.lng(), 6);
    } else {
      gpsData = "No GPS fix";
    }
    doc["gps"] = gpsData;

    // Convert the JSON object to a string and print it to the Serial Monitor
    String output;
    serializeJson(doc, output);
    Serial.println(output);
  }
}

// Function to convert MAC address to string format
String macToStr(uint8_t *mac) {
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

// Function to extract SSID from the payload of a management packet
String extractSSID(uint8_t *payload, size_t len) {
  // The SSID is located after the 24-byte header of a beacon frame
  // The SSID is in the variable-length field, starting at byte 25 (index 24)
  if (len > 24) {
    uint8_t ssidLength = payload[25];  // The length of the SSID
    String ssid = "";
    for (int i = 0; i < ssidLength; i++) {
      ssid += (char)payload[26 + i];  // Append the SSID bytes
    }
    return ssid;
  }
  return "";
}
