#include "esphome.h"

class CustomSerialSensor : public Component, public UARTDevice, public CustomAPIDevice {
 public:
  CustomSerialSensor(UARTComponent *parent, Switch *de_re_switch, sensor::Sensor *temperature_sensor)
      : UARTDevice(parent), de_re_switch_(de_re_switch), temperature_sensor_(temperature_sensor) {}

  void setup() override {
    // Set DE/RE to LOW to enable receive mode
    de_re_switch_->turn_off();
    
    // Register custom services
    register_service(&CustomSerialSensor::on_heater_off_service, "heater_off");
    register_service(&CustomSerialSensor::on_heater_on_service, "heater_on");
  }

  void loop() override {
    // Check if data is available to read
    while (available()) {
        // Read the message byte by byte
        uint8_t received_byte = read();
        String hex_byte = String(received_byte, HEX);

        // Convert the hex byte to uppercase
        hex_byte.toUpperCase();

        // Add leading zero if necessary to make it two characters
        if (hex_byte.length() < 2) {
            hex_byte = "0" + hex_byte;
        }

        message_buffer_ += hex_byte;

        // Check for the start of a new message
        if (received_byte == 0x98) {
            message_buffer_ = "98";  // Start a new message buffer
        }

        // Check if message ends with 0x9C (end of message marker)
        if (received_byte == 0x9C) {
            // Check if the message is one of the common ones and skip logging if so
            if (message_buffer_ != "984007FDE39C" && message_buffer_ != "9840066D3A9C") {
                ESP_LOGI("custom_sensor", "Received RS485 message: %s", message_buffer_.c_str());
            }

            // Process the complete message to get temperature
            get_temperature(message_buffer_);

            // Clear the buffer after processing
            message_buffer_ = "";
        }
    }
  }

  void on_heater_off_service() {
    send_message("984008340000000009E1799C");
    delay(10);
    send_message("984008718000020000687F9C");
    delay(10);
  }

  void on_heater_on_service() {
    send_message("984007700000000001820A9C");
    delay(10);
    send_message("98400834000000001908F79C");
    delay(10);
    send_message("9840087000000000009B0C9C");
    delay(10);
    send_message("98400871800003C0008A449C");
    delay(10);
  }

  void send_message(const char *msg) {
    de_re_switch_->turn_on();  // Set to transmit mode
    
    // Convert hex string to byte array
    std::vector<uint8_t> byte_array = hex_string_to_bytes(msg);
    
    // Send byte array
    write_array(byte_array);
    delay(20); // Allow some time to transmit the message
    
    de_re_switch_->turn_off(); // Set to receive mode
    ESP_LOGI("custom_sensor", "Sent RS485 message: %s", msg);
  }

  // Utility function to convert hex string to byte array
  std::vector<uint8_t> hex_string_to_bytes(const char *hex_str) {
    std::vector<uint8_t> byte_array;
    size_t len = strlen(hex_str);
    for (size_t i = 0; i < len; i += 2) {
      // Take two characters from the string
      char hex_byte[3] = {hex_str[i], hex_str[i + 1], '\0'};
      // Convert the characters to a byte
      uint8_t byte = strtol(hex_byte, NULL, 16);
      byte_array.push_back(byte);
    }
    return byte_array;
  }

  void get_temperature(String message) {
    // Ensure the message starts with "984008600000" and ends with "9C"
    if (message.startsWith("984008600000") && message.endsWith("9C")) {
      // Extract relevant data from the message
      String aa_hex = message.substring(12, 14);
      String bb_hex = message.substring(14, 16);
      String cc_hex = message.substring(16, 18);

      int aa = strtol(aa_hex.c_str(), NULL, 16);
      int bb = strtol(bb_hex.c_str(), NULL, 16);
      int cc = strtol(cc_hex.c_str(), NULL, 16);

      float temperature = 0.0;

      // Corrected condition using aa_hex as a string
      if (aa_hex == "15") {
        temperature = ((bb - 24) * 256 + cc) / 9.0;
      } else if (aa_hex == "16") {
        temperature = ((bb - 128) * 256 + cc) / 9.0;
      } else {
        // If other conditions for temperature are found, handle them appropriately
        return;
      }

      // Publish the temperature to Home Assistant
      temperature_sensor_->publish_state(temperature);
      ESP_LOGI("custom_sensor", "Temperature: %.2f°C", temperature);
    }
  }

 private:
  Switch *de_re_switch_;
  sensor::Sensor *temperature_sensor_;
  String message_buffer_;
};
