#include "driver/twai.h"
#include "BluetoothSerial.h"

// --- HARDWARE PINS ---
#define TX_PIN 21 
#define RX_PIN 22 
#define IDS_SWITCH_PIN 4   // Toggle switch (Connect to 3.3V to arm IDS)
#define INTERNAL_LED_PIN 2 // Built-in blue LED on most ESP32 Dev Boards
#define EXTERNAL_LED_PIN 23 // External Attack Indicator LED

// --- ANSI COLOR CODES FOR TERATERM ---
#define TERM_RESET  "\x1b[0m"
#define TERM_RED    "\x1b[31m" // For Critical DoS & Bus-Off
#define TERM_YELLOW "\x1b[33m" // For Spoofing
#define TERM_CYAN   "\x1b[36m" // For Flooding/Tempo

// --- BLUETOOTH SERIAL FOR ALERTS (TeraTerm) ---
BluetoothSerial SerialBT;

// --- TRIAD-CAN BASELINE PARAMETERS ---
const uint32_t NORMAL_ID = 0x123;
const uint8_t EXPECTED_DATA_0 = 0x4A;
const uint8_t EXPECTED_DATA_1 = 0xAE;
const uint8_t EXPECTED_DLC = 2;

// Layer 2 (TEMPO) Timing Thresholds
const unsigned long BASELINE_INTERVAL_MS = 100; 
// Beta = 0.44 (Flag if message arrives in < 44% of expected time)
const unsigned long INTERVAL_THRESHOLD = 44; 

// --- STATE TRACKING VARIABLES ---
unsigned long last_msg_time = 0;
unsigned long last_attack_detection = 0;
unsigned long last_ext_toggle = 0;
unsigned long last_int_toggle = 0;
bool attack_active = false;

void setup() {
  // 1. Start USB Serial (Raw Data)
  Serial.begin(115200);
  while (!Serial);
  
  // 2. Start Bluetooth Serial (IDS Alerts)
  SerialBT.begin("TRIAD_IDS_Alerts"); 
  
  Serial.println("\n=== ESP32 HYBRID RX + TRIAD-CAN NODE ===");
  Serial.println("USB Serial: Logging Raw CAN Traffic...");
  Serial.println("Bluetooth : Broadcasting as 'TRIAD_IDS_Alerts' (Connect via TeraTerm)...");

  // 3. Configure GPIO
  pinMode(IDS_SWITCH_PIN, INPUT_PULLDOWN);
  pinMode(INTERNAL_LED_PIN, OUTPUT);
  pinMode(EXTERNAL_LED_PIN, OUTPUT);
  
  // Ensure LEDs are off at boot
  digitalWrite(INTERNAL_LED_PIN, LOW);
  digitalWrite(EXTERNAL_LED_PIN, LOW);

  // 4. Configure TWAI hardware (Normal Mode, 500 kbps)
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_PIN, (gpio_num_t)RX_PIN, TWAI_MODE_NORMAL);
  // Increase RX queue length to handle aggressive flooding without crashing
  g_config.rx_queue_len = 20; 
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); 
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); 

  // 5. Install and Start the Driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("TWAI Driver installed.");
  } else {
    Serial.println("CRITICAL FAULT: Driver install failed.");
    while(1);
  }

  if (twai_start() == ESP_OK) {
    Serial.println("TWAI Driver started. Hardware armed.\n");
  } else {
    Serial.println("CRITICAL FAULT: Driver start failed.");
    while(1);
  }
}

void loop() {
  twai_message_t rx_msg;
  unsigned long now = millis();

  // --- 1. RECEIVE MESSAGES ---
  // Using a 100ms timeout. It will wait efficiently without thrashing the CPU.
  if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) == ESP_OK) {
    
    // --> LOG RAW DATA TO ARDUINO USB SERIAL
    Serial.print("[RX] ID: 0x");
    Serial.print(rx_msg.identifier, HEX);
    Serial.print(" | DLC: ");
    Serial.print(rx_msg.data_length_code);
    Serial.print(" | DATA: ");
    for (int i = 0; i < rx_msg.data_length_code; i++) {
      if (rx_msg.data[i] < 0x10) Serial.print("0"); 
      Serial.print(rx_msg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // --- 2. TRIAD-CAN INTRUSION DETECTION SYSTEM ---
    if (digitalRead(IDS_SWITCH_PIN) == HIGH) {
      
      unsigned long interval = now - last_msg_time;
      bool anomaly_flagged_this_frame = false; // Tracks if current frame is an attack

      // LAYER 1: RAPID (Whitelist / ID Validation)
      if (rx_msg.identifier != NORMAL_ID) {
        anomaly_flagged_this_frame = true;
        if (rx_msg.identifier == 0x000) {
          SerialBT.println(TERM_RED "[CRITICAL] LAYER 1 RAPID: Aggressive DoS Attack (ID 0x000) Detected!" TERM_RESET);
        } else {
          SerialBT.print(TERM_CYAN "[WARNING] LAYER 1 RAPID: Fuzzing/Flooding - Unknown ID 0x");
          SerialBT.print(rx_msg.identifier, HEX);
          SerialBT.println(TERM_RESET);
        }
      } 
      else {
        // If the ID is valid (0x123), we must check BOTH Timing and Payload
        bool payload_anomaly = (rx_msg.data_length_code != EXPECTED_DLC || 
                                rx_msg.data[0] != EXPECTED_DATA_0 || 
                                rx_msg.data[1] != EXPECTED_DATA_1);
                                
        bool timing_anomaly = (interval < INTERVAL_THRESHOLD);

        // Evaluate SIGMA (Payload) first, as it is the definitive proof of Spoofing
        if (payload_anomaly) {
          anomaly_flagged_this_frame = true;
          SerialBT.println(TERM_YELLOW "[ALERT] LAYER 3 SIGMA: Spoofing! Valid ID but maliciously altered payload." TERM_RESET);
        } 
        // Only log pure TEMPO (Timing) alerts if the payload is perfectly legitimate
        else if (timing_anomaly) {
          anomaly_flagged_this_frame = true;
          SerialBT.print(TERM_CYAN "[WARNING] LAYER 2 TEMPO: Frequency Spike! Frame arrived in ");
          SerialBT.print(interval);
          SerialBT.println("ms. Bus Saturation imminent." TERM_RESET);
        }
      }

      // If an attack was flagged, update the attack timestamp
      if (anomaly_flagged_this_frame) {
        last_attack_detection = now;
        attack_active = true;
      }

      // Update timing baseline if it was our target ID
      if (rx_msg.identifier == NORMAL_ID) {
        last_msg_time = now;
      }
    }
    
  } else {
    // Check for physical bus crashes
    twai_status_info_t status;
    twai_get_status_info(&status);
    if (status.state == TWAI_STATE_BUS_OFF) {
      Serial.println("[ERROR] Bus-Off detected! Hardware crash.");
      SerialBT.println(TERM_RED "[CRITICAL] Bus-Off condition! Attack was successful in killing the hardware." TERM_RESET);
      
      twai_initiate_recovery();
      delay(250);
      twai_start();
    }
  }

  // --- 3. NON-BLOCKING LED VISUALIZER ---
  // If no attacks have been detected in the last 500ms, clear the attack state
  if (now - last_attack_detection > 500) {
    attack_active = false;
  }

  if (attack_active) {
    // SYSTEM UNDER ATTACK: Turn OFF internal LED, rapidly blink External LED
    digitalWrite(INTERNAL_LED_PIN, LOW); 
    if (now - last_ext_toggle >= 50) {   // 50ms = very fast aggressive blink
      last_ext_toggle = now;
      digitalWrite(EXTERNAL_LED_PIN, !digitalRead(EXTERNAL_LED_PIN));
    }
  } else {
    // SYSTEM SECURE: Turn OFF external LED
    digitalWrite(EXTERNAL_LED_PIN, LOW); 
    
    // Only blink the internal LED if we are actively receiving healthy frames
    if (now - last_msg_time < 500) {     
      if (now - last_int_toggle >= 200) { // 200ms = steady healthy heartbeat blink
        last_int_toggle = now;
        digitalWrite(INTERNAL_LED_PIN, !digitalRead(INTERNAL_LED_PIN));
      }
    } else {
      // If the bus is completely silent (no normal frames), turn the LED off
      digitalWrite(INTERNAL_LED_PIN, LOW);
    }
  }
}

//RX+IDS node firmware
