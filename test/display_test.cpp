#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

// Create an instance of the TFT_eSPI class
TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
        delay(1);
    }
    Serial.println("\n--- TFT Display Test ---");

    // Initialize the TFT display
    tft.init();
    Serial.println("TFT initialized.");

    // Set the rotation (0=portrait, 1=landscape, 2=inv portrait, 3=inv landscape)
    tft.setRotation(1);

    // Fill the screen with a black color
    tft.fillScreen(TFT_BLACK);
    Serial.println("Screen filled black.");

    // Set text color and size
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // White text with a black background
    tft.setTextSize(2);
    tft.setCursor(10, 10); // Set cursor to top-left corner

    // Print a message
    tft.println("Hello, World!");
    tft.println("Display is working!");
    Serial.println("Test message written to screen.");

    Serial.println("Test complete. The display should be showing the message.");
}

void loop() {
    // Nothing to do here for this test
    delay(1000);
}

