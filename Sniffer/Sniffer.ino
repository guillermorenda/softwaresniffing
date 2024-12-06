#include <WiFi.h>
#include "esp_wifi.h"
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <HTTPClient.h>

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

// LTE and HTTP variables
SoftwareSerial simSerial(27, 26); // SIM7000G interface for AT commands (TX, RX)
HTTPClient http;
String serverUrl = "http://your.mobius.server.url";  // Replace with your Mobius server URL
String apn = "your_apn";  // Replace with your APN (e.g., "internet" or whatever your provider requires)


String senderMac = "";
String receiverMac = "";
int rssi = 0;
String ssid = "";


void setup() {
  // Start the serial communication for debugging
  Serial.begin(115200);

  // Initialize the boot button pin as input (with pull-up)
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP); // Internal pull-up resistor

  // Start the SIM7000G serial interface
  simSerial.begin(9600); // SIM7000G baud rate for AT commands is typically 9600

  // Start the GPS module
  ss.begin(9600); // SIM7000G baud rate for GPS is typically 9600

  // Initialize the HTTP client
  http.begin(serverUrl); // Connect to the Mobius server

  // Initialize LTE connection
  setupLTE();

  // Start WiFi in promiscuous mode
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);  // Enable promiscuous mode
  esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);  // Register promiscuous callback

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

  // Send data to the Mobius server
  sendDataToMobius(gpsData);
}

void setupLTE() {
  // Start the LTE connection
  Serial.println("Initializing SIM7000G LTE connection...");

  // Power on the SIM7000G module (send AT commands to initialize the LTE connection)
  sendATCommand("AT");  // Check communication
  delay(2000);
  sendATCommand("AT+CSQ");  // Check signal quality
  delay(2000);
  sendATCommand("AT+CGATT=1");  // Attach to the GPRS network
  delay(2000);
  sendATCommand("AT+CGDCONT=1,\"IP\",\"" + apn + "\"");  // Set APN
  delay(2000);
  sendATCommand("AT+CGACT=1,1");  // Activate the PDP context (connect to the internet)
  delay(5000);
}

void sendDataToMobius(String gpsData) {
  // Create JSON data to send to the Mobius server
  StaticJsonDocument<1024> doc;
  
  // Add fields to the JSON object
  doc["timestamp"] = millis();
  doc["channel"] = currentChannel;
  doc["gps"] = gpsData;
  
  // Extract Wi-Fi packet details (these values will be updated in the promiscuous callback)
  doc["sender_mac"] = senderMac;
  doc["receiver_mac"] = receiverMac;
  doc["rssi"] = rssi;
  doc["ssid"] = ssid;

  // Serialize JSON to string
  String jsonData;
  serializeJson(doc, jsonData);

  // Send data to Mobius server
  http.addHeader("Content-Type", "application/json");  // Set content type as JSON
  int httpResponseCode = http.POST(jsonData);  // Send HTTP POST request with JSON data

  // Check response
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error sending POST request. Code: ");
    Serial.println(httpResponseCode);
  }

  // End the HTTP request
  http.end();
}

void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;

  // Only handle management packets (this can be modified for other types)
  if (type == WIFI_PKT_MGMT) {
    // Extract the RSSI (signal strength) of the packet
    rssi = packet->rx_ctrl.rssi;

    // Extract the sender MAC address (Source MAC)
    uint8_t *senderMacArray = packet->payload + 10;
    senderMac = macToStr(senderMacArray);

    // Extract the receiver MAC address (Destination MAC)
    uint8_t *receiverMacArray = packet->payload + 4;
    receiverMac = macToStr(receiverMacArray);

    // Extract the SSID (if available)
    ssid = extractSSID(packet->payload, packet->rx_ctrl.sig_len);

    // Get the timestamp in milliseconds
    unsigned long timestamp = millis();

    // Prepare the JSON object
    StaticJsonDocument<500> doc;

    // Add fields to the JSON object
    doc["timestamp"] = timestamp;
    doc["channel"] = currentChannel;
    doc["sender_mac"] = senderMac;
    doc["receiver_mac"] = receiverMac;
    doc["rssi"] = rssi;
    doc["ssid"] = ssid;

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
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

// Function to extract SSID from the Wi-Fi packet
String extractSSID(uint8_t *payload, uint16_t len) {
  // Look for the SSID field in the management packet
  for (int i = 0; i < len; i++) {
    if (payload[i] == 0x00) {
      return String((char *)(payload + i + 1));
    }
  }
  return String("");
}

void sendATCommand(String command) {
  // Send an AT command to the SIM7000G module and print the response
  simSerial.println(command);
  delay(1000);
  while (simSerial.available()) {
    Serial.write(simSerial.read());  // Print the response to Serial Monitor
  }
}
