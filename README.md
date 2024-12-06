TEAM: Token Rings

### Problem:
In this time and age, more and more devices are using the electromagnetic spectrum, in the form of Wi-Fi technology. This causes an increase in latencies, pollution in the electromagnetic spectrum, and an overall decrease in efficiency of IoT devices that are being used by public services.
Another possible problem that we intend to solve is illegal renting. Only in Barcelona, in the last year, more than 15,000 apartments were occupied illegally.

### The Presentation of a Solution
Our solution consists of a mechanism that will capture packets exchanged between devices that operate on the 2.4 GHz range. Afterward, it will send these packets to the Mobius platform via IoT communication protocols.
The next step is data management on the server side. There, the system will extract all necessary information, including the MAC address of the sender, MAC address of the receiver, RSSI, and GPS coordinates. As is known, all this information is contained within the packet and can be obtained freely.
Once collected, this information can be processed via a series of algorithms with varying results. For example:
If the goal is to increase the efficiency of stationary public IoT devices in areas with high Wi-Fi pollution, the algorithm may calculate the most efficient channel range to use at a specific time.
On the other hand, if the goal is to pinpoint the location of illegal renters, the algorithm will compare MAC addresses of devices over time. If a certain area shows a high frequency of device changes, it could indicate that the area should be inspected.

### Scenario
This project implements a system for collecting and transmitting information. The system uses an ESP32 as the main device for data acquisition, a SIM7000G for obtaining GPS data and sending information via LTE, and a Raspberry Pi as a central server. The entire setup is powered by a battery, ensuring portability and autonomy for the devices. 

Selected Hardware
**ESP32**: Main microcontroller for data acquisition and processing. 

**SIM7000G**: Communication module responsible for:
Obtaining location data using GPS.
Transmitting the collected data over the LTE network.

**Battery**: Power source that supplies energy to both the ESP32 and the SIM7000G module.

**Raspberry Pi**: Central server responsible for receiving, storing, and processing the data sent by the SIM7000G module.

### Device Connection Description
The physical connections between devices are set up as follows:

**ESP32 ↔ SIM7000G**:

**Data Transmission (TX/RX)**:
The TX pin of the ESP32 is connected to the RX pin of the SIM7000G.
The RX pin of the ESP32 is connected to the TX pin of the SIM7000G.

**Power Supply**:
The 3.3V pin of the SIM7000G is connected to the VCC pin of the ESP32.

**Ground (GND)**:
The GND pin of the SIM7000G is connected to the GND pin of the ESP32.

**Battery**:
The battery simultaneously powers both the ESP32 and the SIM7000G module.

**Raspberry Pi**:
The Raspberry Pi connects to the system remotely (e.g., via HTTP, MQTT, or any chosen protocol) to receive the data sent by the SIM7000G.

### Factors for Improvement
A potential improvement to this project is expanding the system to analyze the 5GHz WiFi spectrum. This enhancement would require replacing the ESP32 with a more capable microcontroller or development board that supports 5GHz WiFi spectrum analysis. Such an upgrade would enable the system to perform additional tasks, such as advanced wireless network diagnostics or interference detection.
