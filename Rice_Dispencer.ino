#include <LiquidCrystal_I2C.h>
#include <HX711.h>
#include <Keypad.h>
#include <Servo.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Load Cell
#define DT 3
#define SCK 2
HX711 scale;
float calibration_factor = 102655.00; // Adjust this to your calibrated value

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
  Serial.begin(9600);

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

  // Check for * button press first, for complete system reset
  if (key == '*') {
    // Always reset the system when * is pressed, regardless of state
    resetSystem();
    return;
  }

  // Handle dispensing state
  if (dispensing) {
    monitorWeight();
    return;
  }

  // Handle transaction complete state
  if (transactionComplete) {
    // We're just waiting for the auto-reset to occur
    // No need for additional code here
    return;
  }

  if (key) {
    if (!dispensing && !transactionComplete) {
      if (key >= '0' && key <= '9') {
        if (inputWeight.length() < 4) { // Limit input length
          inputWeight += key;
          lcd.setCursor(0, 1);
          lcd.print(inputWeight + " kg     ");
        }
      } else if (key == '#') {  // Changed to '#' to start dispensing
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
        
        // Start dispensing
        startDispensing();
      }
    }
  }
}

void startDispensing() {
  dispensing = true;
  lcd.clear();
  lcd.print("Dispensing...");
  
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  
  // Open the dispenser
  dispenserServo.write(180); // Adjust as needed for your servo

  // Initial beep
  tone(BUZZER_PIN, 1000, 200);
  
  lastUpdateTime = millis();
}

void monitorWeight() {
  // Only update the display every 250ms to avoid flicker
  if (millis() - lastUpdateTime > 250) {
    lastUpdateTime = millis();
    
    currentWeight = scale.get_units(3); // Get average of 3 readings
    if (currentWeight < 0) currentWeight = 0;
    
    lcd.setCursor(0, 1);
    lcd.print(currentWeight, 3);
    lcd.print(" / ");
    lcd.print(targetWeight, 1);
    lcd.print(" kg ");
  }

  // Check if target weight is reached
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

    // Send sale data to ESP32
    sendSaleData(currentWeight);

    // Auto-reset after a brief delay
    delay(2000);
    resetSystem();
  }
}

void resetSystem() {
  // Stop dispensing if active
  if (dispensing) {
    dispenserServo.write(90); // Stop the servo
  }
  
  // Reset all states
  inputWeight = "";
  targetWeight = 0.0;
  dispensing = false;
  transactionComplete = false;
  
  // Reset LEDs
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  
  // Reset display
  lcd.clear();
  lcd.print("Enter kilos:");
  lcd.setCursor(0, 1);
  
  // Reset scale
  scale.tare();
  
  // Feedback for reset
  tone(BUZZER_PIN, 2000, 100);
}

void sendSaleData(float weight) {
  // Send weight data to ESP32
  Serial.print(weight, 3); // Send with 3 decimal places
  Serial.println(); // End with newline
}
