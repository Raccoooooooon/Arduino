#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HX711.h>
#include <Servo.h>
#include <Keypad.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Load Cell
#define DT 3
#define SCK 2
HX711 scale;
float calibration_factor = 102655.00; // <- Adjust after calibration

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {4, 5, 6, 7};
byte colPins[COLS] = {8, 9, 10, 11};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo, LEDs, Buzzer
Servo dispenserServo;
#define SERVO_PIN 12
#define GREEN_LED 13
#define RED_LED A0
#define BUZZER_PIN A1

// Variables
String inputWeight = "";
float targetWeight = 0.0;
bool dispensing = false;
bool transactionComplete = false;
float currentWeight = 0.0;
unsigned long lastUpdateTime = 0;

void setup() {
  Serial.begin(19200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Rice Dispenser");
  delay(2000);
  lcd.clear();
  lcd.print("Enter kilos:");

  dispenserServo.attach(SERVO_PIN);
  dispenserServo.write(90); // Initial position (stopped)
  
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
}

void loop() {
  char key = keypad.getKey();

  // Handle dispensing state
  if (dispensing) {
    monitorWeight();
    return;
  }

  if (key) {
    if (transactionComplete && key == '*') {
      // Reset system
      resetSystem();
      return;
    }

    if (!dispensing && !transactionComplete) {
      if (key >= '0' && key <= '9') {
        if (inputWeight.length() < 4) { // Limit input length
          inputWeight += key;
          lcd.setCursor(0, 1);
          lcd.print(inputWeight + " kg     ");
        }
      } else if (key == '#') {
        targetWeight = inputWeight.toFloat(); // Convert input to float
        
        if (targetWeight <= 0 || targetWeight > 10) { // Validate input
          lcd.clear();
          lcd.print("Invalid amount!");
          lcd.setCursor(0, 1);
          lcd.print("0.1-10 kg only");
          delay(2000);
          lcd.clear();
          lcd.print("Enter kilos:");
          lcd.setCursor(0, 1);
          lcd.print(inputWeight + " kg     ");
          return;
        }
        
        inputWeight = "";
        
        // Prepare for dispensing
        lcd.clear();
        lcd.print("Target: ");
        lcd.print(targetWeight, 3);
        lcd.print(" kg");
        delay(1000);
        
        startDispensing();
      } else if (key == '*') {
        // Clear input if no transaction yet
        inputWeight = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
        // Make sure servo doesn't move here
      }
    }
  }
}

void startDispensing() {
  dispensing = true;
  digitalWrite(RED_LED, HIGH); // Red light on during dispensing
  
  lcd.clear();
  lcd.print("Dispensing...");
  
  // Start servo motor
  dispenserServo.write(0); // Adjust this value based on your servo orientation
  
  lastUpdateTime = millis();
}

void monitorWeight() {
  // Read the weight from scale
  currentWeight = scale.get_units();
  
  // Update display every 200ms to avoid flicker
  if (millis() - lastUpdateTime > 200) {
    lcd.setCursor(0, 1);
    lcd.print(currentWeight, 3);
    lcd.print(" / ");
    lcd.print(targetWeight, 3);
    lcd.print("kg");
    lastUpdateTime = millis();
  }
  
  // Check if target weight is reached or exceeded
  if (currentWeight >= targetWeight) {
    dispensing = false;
    transactionComplete = true;
    
    // Stop the servo
    dispenserServo.write(90);
    
    // Visual and audio feedback
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    
    // Beep 3 times
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 1000);
      delay(200);
      noTone(BUZZER_PIN);
      delay(100);
    }
    
    lcd.clear();
    lcd.print("Complete!");
    lcd.setCursor(0, 1);
    lcd.print(currentWeight, 3);
    lcd.print(" kg");
    
    // Let the user know they can reset
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print("Press * to reset");
  }
}

void resetSystem() {
  // Make sure to set the servo to idle position first before doing anything else
  dispenserServo.write(90);
  
  lcd.clear();
  lcd.print("Enter kilos:");
  inputWeight = "";
  targetWeight = 0.0;
  dispensing = false;
  transactionComplete = false;
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  scale.tare(); // Reset scale to zero
}
