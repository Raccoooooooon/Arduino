#include "HX711.h"

#define DT 3
#define SCK 2

HX711 scale;

void setup() {
  Serial.begin(9600);
  scale.begin(DT, SCK);
  scale.set_scale();  // Start with no calibration factor
  scale.tare();       // Reset the scale to 0

  Serial.println("\nHX711 Calibration Mode");
  Serial.println("1. Remove all weight.");
  Serial.println("2. Send 'r' to reset to 0 (tare).");
  Serial.println("3. Place exactly 1kg of rice.");
  Serial.println("4. Read the number shown (this is your calibration factor).");
  Serial.println("5. Use that number in your main sketch: scale.set_scale(<your number>);");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'r' || cmd == 'R') {
      scale.tare();
      Serial.println("Tare done. Scale reset to 0.");
    }
  }

  Serial.print("Reading: ");
  Serial.print(scale.get_units(10)); // average of 10 readings
  Serial.println(" units");

  delay(1000);
}