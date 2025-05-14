#include <HX711.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 3;  // DT pin
const int LOADCELL_SCK_PIN = 2;   // SCK pin

HX711 scale;

// Calibration factor - this will be adjusted during calibration
float calibration_factor = -90000000;  // This is a starting value, will be adjusted during calibration
                                    // The negative/positive value depends on your scale orientation

void setup() {
  Serial.begin(9600);
  Serial.println("HX711 Calibration for Kilograms");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place a known weight on the scale");
  Serial.println("Press + or - to adjust the calibration factor");
  Serial.println("Press t to tare (reset to zero) the scale");
  Serial.println("Press s to save the calibration factor");
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();  // Initialize with default calibration factor
  scale.tare();       // Reset the scale to 0
  
  long zero_factor = scale.read_average();  // Get a baseline reading
  Serial.print("Zero factor: ");  // This can be useful for debugging
  Serial.println(zero_factor);
}

void loop() {
  scale.set_scale(calibration_factor);  // Adjust to this calibration factor

  Serial.print("Reading: ");
  Serial.print(scale.get_units(5), 3);  // Get average of 5 readings
  Serial.print(" kg");
  Serial.print("\t Calibration Factor: ");
  Serial.println(calibration_factor);

  if (Serial.available()) {
    char temp = Serial.read();
    
    // Increase calibration factor
    if (temp == '+') {
      calibration_factor += 10;
    }
    // Further increase calibration factor
    else if (temp == 'a') {
      calibration_factor += 100;
    }
    // Decrease calibration factor
    else if (temp == '-') {
      calibration_factor -= 10;
    }
    // Further decrease calibration factor
    else if (temp == 'z') {
      calibration_factor -= 100;
    }
    // Tare the scale
    else if (temp == 't') {
      scale.tare();
      Serial.println();
      Serial.println("---------------------");
      Serial.println("Scale tared to zero!");
      Serial.println("Current reading: 0.000 kg");
      Serial.println("---------------------");
      Serial.println();
    }
    // Save the calibration factor
    else if (temp == 's') {
      Serial.println();
      Serial.println("---------------------");
      Serial.print("Calibration factor saved: ");
      Serial.println(calibration_factor);
      Serial.println("Use this value in your final code");
      Serial.println("---------------------");
      Serial.println();
    }
  }
  delay(500);  // Short delay for readability
}
