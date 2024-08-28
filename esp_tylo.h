#include "esphome.h"

class CustomSerialSensor : public Component, public UARTDevice, public CustomAPIDevice {
 public:
  CustomSerialSensor(UARTComponent *parent, Switch *de_re_switch, sensor::Sensor *temperature_sensor, binary_sensor::BinarySensor *heater_status, binary_sensor::BinarySensor *light_status)
      : UARTDevice(parent), de_re_switch_(de_re_switch), temperature_sensor_(temperature_sensor), heater_status_(heater_status), light_status_(light_status) {}

  void setup() override {
    // Set DE/RE to LOW to enable receive mode
    de_re_switch_->turn_off();
    
    // Register custom services
    register_service(&CustomSerialSensor::heater_off, "heater_off");
    register_service(&CustomSerialSensor::heater_on, "heater_on");
    register_service(&CustomSerialSensor::light_off, "light_off");
    register_service(&CustomSerialSensor::light_on, "light_on");
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

      // Add the byte to the message buffer
      message_buffer_ += hex_byte;

      // Check for the start of a new message
      if (received_byte == 0x98) {
        message_buffer_ = "98";  // Start a new message buffer
      }

      // Check if message ends with 0x9C (end of message marker)
      if (received_byte == 0x9C) {
        // Check if the message is one of the common ones and skip logging if so
        if (message_buffer_ != "984007FDE39C" && message_buffer_ != "9840066D3A9C") {
          // Get the current time for precise logging
          auto time_now = millis();
          auto hours = (time_now / (1000 * 60 * 60)) % 24;
          auto minutes = (time_now / (1000 * 60)) % 60;
          auto seconds = (time_now / 1000) % 60;
          auto milliseconds = time_now % 1000;

          // Log the message with timestamp
          ESP_LOGI("custom_sensor", "[%02d:%02d:%02d.%03d] Received RS485 message: %s",
                   hours, minutes, seconds, milliseconds, message_buffer_.c_str());
        }

        // Process the complete message
        get_temperature(message_buffer_);
        get_heater_and_light_status(message_buffer_);

        // Clear the buffer after processing
        message_buffer_ = "";
      }
    }
  }

  void heater_off() {
    if (heater_status_->state) {
      de_re_switch_->turn_on(); // Set to transmit mode before sending all messages
      delay(15);
      send_message("984007700000000001820A9C");
      delay(10);
      de_re_switch_->turn_off(); // Set to receive mode after all messages
    }
  }

  void heater_on() {
    if (!heater_status_->state) {
      de_re_switch_->turn_on(); // Set to transmit mode before sending all messages
      delay(15);
      send_message("984007700000000001820A9C");
      delay(10);
      de_re_switch_->turn_off(); // Set to receive mode after all messages
    }
  }

  void light_on() {
    if (!light_status_->state) {
      de_re_switch_->turn_on(); // Set to transmit mode before sending all messages
      delay(15);
      send_message("984007700000000002A3B89C");
      delay(10);
      de_re_switch_->turn_off(); // Set to receive mode after all messages
    }
  }

  void light_off() {
    if (light_status_->state) {
      de_re_switch_->turn_on(); // Set to transmit mode before sending all messages
      delay(15);
      send_message("984007700000000002A3B89C");
      delay(10);
      de_re_switch_->turn_off(); // Set to receive mode after all messages
    }
  }

  void send_message(const char *msg) {
    // Convert hex string to byte array
    std::vector<uint8_t> byte_array = hex_string_to_bytes(msg);

    // Log the byte array being sent
    String log_message = "Sent byte array: ";
    for (uint8_t byte : byte_array) {
      if (byte < 0x10) {
        log_message += "0";  // Add leading zero for single digit
      }
      log_message += String(byte, HEX);
      log_message += " ";
    }
    ESP_LOGI("custom_sensor", "%s", log_message.c_str());

    // Send the byte array
    write_array(byte_array);
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

      if (aa_hex == "15") {
        temperature = ((bb - 24) * 256 + cc) / 9.0;
      } else if (aa_hex == "16") {
        temperature = ((bb - 128) * 256 + cc) / 9.0;
	  } else if (aa_hex == "17") {
        temperature = ((bb - 232) * 256 + cc) / 9.0;
      } else {
        // If other conditions for temperature are found, handle them appropriately
        return;
      }

      // Publish the temperature to Home Assistant
      temperature_sensor_->publish_state(temperature);
      ESP_LOGI("custom_sensor", "Temperature: %.2f°C", temperature);
    }
  }

  void get_heater_and_light_status(String message) {
    // Ensure the message starts with "9840083400000000" and ends with "9C"
    if (message.startsWith("9840083400000000") && message.endsWith("9C")) {
      // Extract the status byte after the zeros
      String status_hex = message.substring(16, 18);
      int status = strtol(status_hex.c_str(), NULL, 16);
      
      bool heater_on = false;
      bool light_on = false;
        
	  // Determine the heater and light status using bitwise operations
	  if (status & 16) {
		heater_on = true;
	  }

	  if (status & 8) {
		light_on = true;
	  }

      // Publish the heater and light status to Home Assistant
      heater_status_->publish_state(heater_on);
      light_status_->publish_state(light_on);

      ESP_LOGI("custom_sensor", "Heater status: %s, Light status: %s",
               heater_on ? "ON" : "OFF", light_on ? "ON" : "OFF");
    }
  }

 private:
  Switch *de_re_switch_;
  sensor::Sensor *temperature_sensor_;
  binary_sensor::BinarySensor *heater_status_;
  binary_sensor::BinarySensor *light_status_;
  String message_buffer_;
};
