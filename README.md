<div align="center">

# 🔥 Smart kibble dispenser

<p align="center">
  <img width="364" height="341" alt="image" src="https://github.com/user-attachments/assets/d4e8ff6c-9ebc-4495-95e6-1a24c7aaa75e" style="border-radius: 8px;" />
</p>

*An automated kibble dispenser, powered by an ESP32, infrared distance sensors, step motor and native Zigbee integration for Home Assistant.*

---

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![Author](https://img.shields.io/badge/author-Adrien%20Brune-orange.svg)
![Language](https://img.shields.io/badge/language-C%2F++-yellow.svg)
![Hardware](https://img.shields.io/badge/hardware-ESP32H2-red.svg)
![Protocol](https://img.shields.io/badge/protocol-Zigbee-blueviolet.svg)
![Platform](https://img.shields.io/badge/platform-Home%20Assistant%20%7C%20Z2M-lightgrey.svg)
![Status](https://img.shields.io/badge/status-active-success.svg)

</div>
## 💡 Introduction

Never run out of pet food again! The **Smart Pet Feeder Level Monitor** is a DIY IoT solution designed to monitor your pet food reserve in real-time. 

3D printed dispenser made to provide an entire week of food, the system uses precision infrared distance sensors to measure the height of the remaining food. Through a simple calibration process, the ESP32 delivers the exact amount for a configured portion of kibbles. Every features are connected to **Home Assistant** via the **Zigbee** protocol.

---

## ✨ Key Features

* **Infrared Distance Sensing:** Instantly know how much food is left in your pet's dispenser directly from your Home Assistant dashboard.
* **Step motor to ensure accuracy portions:** The step motor ensure a precise portion to deliver to the pet in grams configurable from Home Assistant.
* **Simple Calibration:** The kibble portion calibration can be configured from Home Assistant at any moment.
* **Native Zigbee Integration:** Seamless pairing with Home Assistant (Zigbee2MQTT) with low power consumption and reliable local communication.

---

## 🛠️ Technical Stack & Hardware

* **Microcontroller:** ESP32H2 (Wireless SoC)
* **Firmware Language:** C++
* **Communication Protocol:** Zigbee
* **Sensors:** Infrared distance sensor VL53LX
* **Mecanics:** Step motor NEMA with its TMC2209 controller
* **Smart Home Platform:** Home Assistant