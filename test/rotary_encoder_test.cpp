#include <Arduino.h>
#include "rot_encoder.h" // Include your RotaryEncoderManager class

// Define the pins for the rotary encoder (from main.cpp)
#define ENCODER_CLK_PIN 32
#define ENCODER_DT_PIN  33
#define ENCODER_SW_PIN  15

RotaryEncoderManager encoder(ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN);

// --- Button Callback Functions ---
void handleButtonClick() {
    Serial.println("Rotary Button: CLICK!");
}

void handleButtonDoubleClick() {
    Serial.println("Rotary Button: DOUBLE CLICK!");
}

void handleButtonLongPress() {
    Serial.println("Rotary Button: LONG PRESS START!");
    encoder.resetPosition(); // Example: Reset position on long press
    Serial.println("Encoder position reset.");
}

// --- Setup Function ---
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) { // Wait for Serial to initialize for up to 2 seconds
        delay(1);
    }
    Serial.println("\n--- Rotary Encoder Test ---");
    Serial.println("Rotate the encoder to see position and delta changes.");
    Serial.println("Press the button for click, double click, and long press events.");

    encoder.begin();
    // Attach button callbacks
    encoder.setClickHandler(handleButtonClick);
    encoder.setDoubleClickHandler(handleButtonDoubleClick);
    encoder.setLongPressHandler(handleButtonLongPress);

    Serial.println("Rotary Encoder Manager initialized.");
    Serial.printf("Initial encoder position: %ld\n", encoder.getPosition());
}

// --- Loop Function ---
void loop() {
    encoder.update(); // IMPORTANT: Must be called frequently for OneButton to work

    int32_t delta = encoder.getDelta();
    if (delta != 0) {
        Serial.printf("Encoder: Position = %ld, Delta = %ld\n", encoder.getPosition(), delta);
    }

    delay(1); // Small delay to prevent watchdog timer issues and excessive serial output
}
