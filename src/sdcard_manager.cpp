#include "sdcard_manager.h"

// Global file object (optional — mostly for reuse in functions)
File32 file;

bool writeFile(const char* filepath, const char* data) {
  file = sd.open(filepath, O_RDWR | O_CREAT);
  if (!file) {
    Serial.println("Failed to open file for writing.");
    return false;
  }

  file.print(data);  // You can also use file.write(data, strlen(data));
  file.close();
  return true;
}

String readFile(const char* filepath) {
  File32 file = sd.open(filepath, O_RDONLY);
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return "";
  }

  String content = "";
  while (file.available()) {
    content += (char)file.read();
  }

  file.close();
  return content;
}


