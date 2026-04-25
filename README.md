TRIAD-CAN: Real-Time CAN Bus Attack Detection

A lightweight, real-time Intrusion Detection System (IDS) for automotive CAN networks. TRIAD-CAN detects DoS, spoofing, and fuzzing attacks using a multi-layer approach on embedded hardware (STM32 + ESP32), without machine learning or external processing.


 Features

* Real-time CAN attack detection (<10µs per frame)
* Detects DoS, Spoofing, and Fuzzing attacks
* Three-layer detection:
    * L1 (RAPID): ID validation
    * L2 (TEMPO): Timing analysis
    * L3 (SIGMA): Payload integrity
* Training-free (no datasets required)
* Live web dashboard for attack control & monitoring
* Bluetooth alerts + LED indicators
* Fully embedded (runs on ESP32)


Hardware Used

* STM32F103C8T6 (Blue Pill) – Transmitter
* ESP32 (Receiver + IDS)
* ESP32 (Attacker Node)
* SN65HVD230 CAN Transceivers (×3)
* Breadboard, 120Ω termination resistors


Software Used

* STM32CubeIDE (STM32 programming)
* Arduino IDE (ESP32 programming)
* Node.js + HTML/CSS/JS (Web dashboard)


 How It Works

1. Initialization: CAN network setup at 500 kbps
2. Calibration: IDS learns normal traffic patterns
3. Attack Injection: Attacker ESP32 triggers attacks via web UI
4. Detection: Incoming frames analyzed across 3 layers
5. Classification: Multi-evidence fusion identifies attack type
6. Alerts: Output via dashboard, Bluetooth, and LEDs


Attacks Simulated

* DoS: Bus flooding with high-priority IDs
* Spoofing: Fake messages with valid IDs
* Fuzzing: Random IDs and payloads


 Results

* Accurate real-time detection of all attack types
* Stable operation under high traffic
* Lightweight and suitable for embedded deployment


 Future Improvements

* Add attack prevention (blocking/filtering)
* Support more attacks (MITM, replay, bus-off)
* Scale to larger CAN networks
* CAN-FD support
* Optional ML-based enhancements


Applications

* Automotive cybersecurity
* Electric & autonomous vehicles
* Industrial CAN systems
* Research & testing platforms
