/* Password-based Door Lock System with Solenoid
 * Based on original code by Md. Riyad Hasan
 * Enhanced with buzzer fixes, LCD optimizations, and ESP32-CAM integration
 */

#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x21, 16, 2); // Change 0x27 if your LCD has a different I2C address

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = 
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {3, 4, 5, 6};
byte colPins[COLS] = {7, 8, 9, 10};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Pin setup
const int Solenoid_Pin = 13;
const int Buzzer_Pin = 11;
const int Led_Pin = 12;
const int ESP32_Signal_Pin = 2; // D2 for ESP32-CAM signal

// Password settings
const char Master_Password[] = "1122334455";  // Master password (10 digits)
String User_Password = "0000";    // Changeable user password (4 digits)
String Input_Password = "";

// System settings
const int MAX_WRONG_ATTEMPTS = 3;
const unsigned long LOCKOUT_TIME = 10000; // 10 seconds lockout
const unsigned long DOOR_OPEN_TIME = 3000; // 3 seconds door open
const unsigned long DEBOUNCE_DELAY = 50;   // 50ms debounce
const unsigned long LCD_UPDATE_INTERVAL = 250; // Update LCD every 250ms during lockout
const unsigned long ESP32_COOLDOWN_TIME = 5000; // 5 seconds cooldown after ESP32 signal
const unsigned long DOOR_CLOSE_DELAY = 1000; // 1 second delay before door can be opened again
const unsigned long DISPLAY_RESET_TIME = 2000; // 2 seconds to display ESP32 message

// Variables
int Wrong_Attempts = 0;
bool Change_Mode = false;
bool Waiting_For_New_Password = false;
unsigned long lockoutEndTime = 0;
unsigned long lastKeyPressTime = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastESP32SignalTime = 0;
unsigned long lastDoorOpenTime = 0;
unsigned long esp32MessageTime = 0;
bool inLockoutMode = false;
bool systemResetNeeded = false;
bool doorCurrentlyOpen = false;
bool esp32CooldownActive = false; // Cooldown after ESP32 signal
unsigned long esp32CooldownEndTime = 0;

void setup() 
{
  // Initialize pins
  pinMode(Solenoid_Pin, OUTPUT);
  pinMode(Buzzer_Pin, OUTPUT);
  pinMode(Led_Pin, OUTPUT);
  pinMode(ESP32_Signal_Pin, INPUT); // Regular INPUT mode
  
  // Ensure solenoid is locked initially
  digitalWrite(Solenoid_Pin, LOW);
  digitalWrite(Led_Pin, LOW);
  noTone(Buzzer_Pin);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Display welcome message
  lcd.setCursor(0, 0);
  lcd.print("Door Lock System");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  // Short beep to indicate startup
  tone(Buzzer_Pin, 1000, 100);
  delay(1500);
  
  lcd.clear();
  lcd.print("Enter Password:");
  lcd.setCursor(0, 1);
  lcd.print("____"); // Show 4 underscores initially
  lcd.setCursor(0, 1);
  
  Serial.begin(9600); // Optional: for debugging
  Serial.println("System Initialized");
  Serial.println("ESP32 Signal Pin: D2 (Expecting HIGH signal)");
  Serial.println("System will open door when pin goes HIGH");
  Serial.println("Cooldown: 5 seconds after ESP32 signal");
}

void loop() 
{
  // Check for ESP32-CAM signal (simple HIGH detection)
  checkESP32Signal();
  
  // Check ESP32 cooldown
  if (esp32CooldownActive && millis() > esp32CooldownEndTime)
  {
    esp32CooldownActive = false;
    Serial.println("ESP32 cooldown ended");
  }
  
  // Check if we need to reset ESP32 message display
  if (esp32MessageTime > 0 && millis() - esp32MessageTime > DISPLAY_RESET_TIME && !doorCurrentlyOpen)
  {
    esp32MessageTime = 0;
    resetDisplayToPassword();
  }
  
  // Check if system is locked out
  if (lockoutEndTime > 0 && millis() < lockoutEndTime) 
  {
    handleLockoutMode();
    return;
  }
  else if (lockoutEndTime > 0) 
  {
    // Lockout period ended
    exitLockoutMode();
  }
  
  // Reset system if needed after lockout
  if (systemResetNeeded)
  {
    resetSystemAfterLockout();
    systemResetNeeded = false;
  }
  
  // Get key with debouncing
  char key = keypad.getKey();
  
  if (key && (millis() - lastKeyPressTime > DEBOUNCE_DELAY)) 
  {
    lastKeyPressTime = millis();
    handleKeyPress(key);
  }
}

void checkESP32Signal()
{
  static bool lastSignalState = LOW; // Track previous state
  static bool signalProcessed = false; // Track if we've processed current HIGH signal
  bool currentSignalState = digitalRead(ESP32_Signal_Pin);
  
  // Detect when pin goes HIGH (ESP32 activates signal)
  if (lastSignalState == LOW && currentSignalState == HIGH)
  {
    Serial.println("ESP32: Detected HIGH signal (rising edge)");
    
    // Check if we can process this signal
    if (!esp32CooldownActive && lockoutEndTime == 0 && 
        millis() - lastDoorOpenTime > DOOR_CLOSE_DELAY && !signalProcessed)
    {
      signalProcessed = true;
      openDoorViaESP32();
    }
    else if (esp32CooldownActive)
    {
      Serial.println("ESP32: Signal ignored (in cooldown)");
    }
    else if (lockoutEndTime > 0)
    {
      Serial.println("ESP32: Signal ignored (system locked)");
      showLockoutMessage();
    }
  }
  
  // Detect when pin goes LOW (ESP32 releases signal)
  if (lastSignalState == HIGH && currentSignalState == LOW)
  {
    Serial.println("ESP32: Detected LOW signal (falling edge) - Signal released");
    signalProcessed = false; // Reset for next signal
  }
  
  lastSignalState = currentSignalState;
}

void openDoorViaESP32()
{
  if (doorCurrentlyOpen) 
  {
    Serial.println("ESP32: Door already open");
    return;
  }
  
  Serial.println("ESP32: Opening door via ESP32 signal");
  
  // Start cooldown period
  esp32CooldownActive = true;
  esp32CooldownEndTime = millis() + ESP32_COOLDOWN_TIME;
  
  lcd.clear();
  lcd.print("ESP32 Access");
  lcd.setCursor(0, 1);
  lcd.print("Opening Door...");
  esp32MessageTime = millis(); // Start timer for message display
  
  // Beep twice for ESP32 access
  tone(Buzzer_Pin, 1200, 200);
  delay(300);
  tone(Buzzer_Pin, 1200, 200);
  
  // Open door (activate solenoid)
  digitalWrite(Solenoid_Pin, HIGH);
  digitalWrite(Led_Pin, HIGH);
  doorCurrentlyOpen = true;
  lastDoorOpenTime = millis();
  
  // Display status
  delay(500);
  lcd.clear();
  lcd.print("Door Open");
  lcd.setCursor(0, 1);
  lcd.print("Via ESP32-CAM");
  esp32MessageTime = millis(); // Reset timer
  
  // Keep door open for specified time
  unsigned long doorOpenStart = millis();
  while (millis() - doorOpenStart < DOOR_OPEN_TIME)
  {
    // Continue to monitor ESP32 signal but ignore during opening
    // This prevents re-triggering during the open period
    if (digitalRead(ESP32_Signal_Pin) == LOW)
    {
      Serial.println("ESP32: Signal released during door open - ignoring");
    }
    delay(100); // Small delay to prevent CPU overuse
  }
  
  // Close door
  closeDoor();
  
  // Update display after door closes
  lcd.clear();
  lcd.print("Door Closed");
  lcd.setCursor(0, 1);
  lcd.print("ESP32 Complete");
  esp32MessageTime = millis(); // Reset timer again
  
  Serial.println("ESP32: Door operation complete");
  Serial.print("Cooldown active for ");
  Serial.print(ESP32_COOLDOWN_TIME / 1000);
  Serial.println(" seconds");
}

void showLockoutMessage()
{
  lcd.clear();
  lcd.print("System Locked");
  lcd.setCursor(0, 1);
  lcd.print("ESP32 Signal Ignored");
  tone(Buzzer_Pin, 300, 500);
  esp32MessageTime = millis(); // Start timer to reset display
}

void openDoorViaPassword()
{
  if (doorCurrentlyOpen) 
  {
    Serial.println("Password: Door already open");
    return;
  }
  
  Serial.println("Password: Opening door via password");
  
  lcd.clear();
  lcd.print("Access Granted!");
  tone(Buzzer_Pin, 1500, 300);
  digitalWrite(Led_Pin, HIGH);
  
  // Open door (activate solenoid)
  digitalWrite(Solenoid_Pin, HIGH);
  doorCurrentlyOpen = true;
  lastDoorOpenTime = millis();
  
  lcd.setCursor(0, 1);
  lcd.print("Door Opening...");
  delay(DOOR_OPEN_TIME);
  
  // Close door
  closeDoor();
  
  lcd.clear();
  lcd.print("Door Locked");
  delay(1000);
}

void closeDoor()
{
  // Close door (deactivate solenoid)
  digitalWrite(Solenoid_Pin, LOW);
  digitalWrite(Led_Pin, LOW);
  doorCurrentlyOpen = false;
  
  // Update last door open time
  lastDoorOpenTime = millis();
  
  Serial.println("Door closed");
}

void handleLockoutMode()
{
  static unsigned long lastBlinkTime = 0;
  static bool ledState = false;
  static bool displayInitialized = false;
  
  // Initialize lockout display only once
  if (!displayInitialized)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SYSTEM LOCKED");
    displayInitialized = true;
    inLockoutMode = true;
  }
  
  // Update countdown only every LCD_UPDATE_INTERVAL (reduces LCD updates)
  if (millis() - lastLCDUpdate > LCD_UPDATE_INTERVAL)
  {
    lastLCDUpdate = millis();
    unsigned long remainingTime = (lockoutEndTime - millis()) / 1000;
    
    lcd.setCursor(0, 1);
    lcd.print("Wait: ");
    if (remainingTime >= 10) 
    {
      lcd.print(remainingTime);
      lcd.print("s ");
    }
    else 
    {
      lcd.print("0");
      lcd.print(remainingTime);
      lcd.print("s ");
    }
    lcd.print("    "); // Clear any leftover characters
  }
  
  // Blink LED every 500ms without blocking
  if (millis() - lastBlinkTime > 500)
  {
    lastBlinkTime = millis();
    ledState = !ledState;
    digitalWrite(Led_Pin, ledState);
  }
}

void exitLockoutMode()
{
  lockoutEndTime = 0;
  inLockoutMode = false;
  digitalWrite(Led_Pin, LOW);
  noTone(Buzzer_Pin);
  
  lcd.clear();
  lcd.print("Lockout Ended");
  tone(Buzzer_Pin, 800, 200);
  delay(1000);
  
  // Set flag to reset system properly
  systemResetNeeded = true;
}

void resetSystemAfterLockout()
{
  // Reset all system variables to default state
  Input_Password = "";
  Wrong_Attempts = 0;
  Change_Mode = false;
  Waiting_For_New_Password = false;
  doorCurrentlyOpen = false;
  esp32CooldownActive = false;
  
  // Reset LCD display to normal state
  resetDisplayToPassword();
  
  // Ensure all outputs are off
  digitalWrite(Solenoid_Pin, LOW);
  digitalWrite(Led_Pin, LOW);
  noTone(Buzzer_Pin);
  
  Serial.println("System reset after lockout");
}

void resetDisplayToPassword()
{
  lcd.clear();
  lcd.print("Enter Password:");
  lcd.setCursor(0, 1);
  lcd.print("____");
  lcd.setCursor(0, 1);
}

void handleKeyPress(char key) 
{
  // Give feedback for key press
  tone(Buzzer_Pin, 800, 20);
  
  if (key >= '0' && key <= '9') 
  {
    if (Change_Mode && !Waiting_For_New_Password) 
    {
      // Master password input (max 10 digits)
      if (Input_Password.length() < 10) 
      {
        Input_Password += key;
        displayPasswordDots(Input_Password.length(), 10);
      }
    }
    else if (Waiting_For_New_Password) 
    {
      // New user password input (max 4 digits)
      if (Input_Password.length() < 4) 
      {
        Input_Password += key;
        displayPasswordDots(Input_Password.length(), 4);
      }
    }
    else 
    {
      // Normal user password input (max 4 digits)
      if (Input_Password.length() < 4) 
      {
        Input_Password += key;
        displayPasswordDots(Input_Password.length(), 4);
      }
    }
  }
  else if (key == 'C') 
  {
    // Clear input
    Input_Password = "";
    lcd.setCursor(0, 1);
    if (Change_Mode && !Waiting_For_New_Password) 
    {
      lcd.print("__________"); // 10 underscores for master
    }
    else if (Waiting_For_New_Password || (!Change_Mode && !Waiting_For_New_Password))
    {
      lcd.print("____"); // 4 underscores for user
    }
    lcd.setCursor(0, 1);
  }
  else if (key == '#') 
  { 
    // OK/Enter button
    Check_Password();
  }
  else if (key == '*') 
  { 
    // Change password mode
    Change_Mode = true;
    Waiting_For_New_Password = false;
    Input_Password = "";
    lcd.clear();
    lcd.print("Master Password:");
    lcd.setCursor(0, 1);
    lcd.print("__________"); // 10 underscores for master password
    lcd.setCursor(0, 1);
  }
}

void Check_Password() 
{
  if (Change_Mode && !Waiting_For_New_Password) 
  {
    // Step 1: Check master password input
    if (Input_Password == Master_Password) 
    {
      lcd.clear();
      lcd.print("New Password:");
      Input_Password = "";
      Waiting_For_New_Password = true;
      lcd.setCursor(0, 1);
      lcd.print("____"); // 4 underscores for new password
      lcd.setCursor(0, 1);
      tone(Buzzer_Pin, 1500, 200);
    }
    else 
    {
      lcd.clear();
      lcd.print("Wrong Master!");
      tone(Buzzer_Pin, 300, 500);
      delay(1000);
      resetDisplayToPassword();
      Change_Mode = false;
      Input_Password = "";
    }
  }
  else if (Change_Mode && Waiting_For_New_Password) 
  {
    // Step 2: Set new user password (must be 4 digits)
    if (Input_Password.length() == 4) 
    {
      User_Password = Input_Password;
      lcd.clear();
      lcd.print("Password Changed");
      tone(Buzzer_Pin, 1500, 300);
      delay(1000);
      resetDisplayToPassword();
      Change_Mode = false;
      Waiting_For_New_Password = false;
      Input_Password = "";
    }
    else 
    {
      lcd.clear();
      lcd.print("4 digits only!");
      tone(Buzzer_Pin, 300, 500);
      delay(1000);
      lcd.clear();
      lcd.print("New Password:");
      Input_Password = "";
      lcd.setCursor(0, 1);
      lcd.print("____");
      lcd.setCursor(0, 1);
    }
  }
  else 
  {
    // Normal door open check
    if (Input_Password == User_Password || Input_Password == Master_Password) 
    {
      // Open door via password
      openDoorViaPassword();
      
      Wrong_Attempts = 0;
      resetDisplayToPassword();
      Input_Password = "";
    }
    else 
    {
      Wrong_Attempts++;
      lcd.clear();
      lcd.print("Wrong Password!");
      lcd.setCursor(0, 1);
      lcd.print("Try: ");
      lcd.print(Wrong_Attempts);
      lcd.print("/");
      lcd.print(MAX_WRONG_ATTEMPTS);
      
      tone(Buzzer_Pin, 300, 500);
      delay(2000);
      
      if (Wrong_Attempts >= MAX_WRONG_ATTEMPTS) 
      {
        activateAlarm();
      }
      else
      {
        resetDisplayToPassword();
        Input_Password = "";
      }
    }
  }
}

void activateAlarm()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ALARM ACTIVATED!");
  lcd.setCursor(0, 1);
  lcd.print("System Locked!");
  
  // Play alarm sound with non-blocking pattern
  unsigned long alarmStartTime = millis();
  unsigned long lastBeepTime = 0;
  bool beepHigh = true;
  
  while (millis() - alarmStartTime < 3000) // 3 seconds alarm
  {
    // Alternate between high and low tones every 200ms
    if (millis() - lastBeepTime > 200)
    {
      lastBeepTime = millis();
      beepHigh = !beepHigh;
      
      if (beepHigh)
      {
        tone(Buzzer_Pin, 1200, 180);
        digitalWrite(Led_Pin, HIGH);
      }
      else
      {
        tone(Buzzer_Pin, 600, 180);
        digitalWrite(Led_Pin, LOW);
      }
    }
  }
  
  // Cleanup
  noTone(Buzzer_Pin);
  digitalWrite(Led_Pin, LOW);
  
  // Reset input password
  Input_Password = "";
  
  // Start lockout
  lockoutEndTime = millis() + LOCKOUT_TIME;
  lastLCDUpdate = 0; // Reset for lockout display
  
  // Reset wrong attempts counter
  Wrong_Attempts = 0;
}

void displayPasswordDots(int length, int maxLength) 
{
  lcd.setCursor(0, 1);
  
  // Display asterisks for entered digits
  for (int i = 0; i < maxLength; i++) 
  {
    if (i < length) 
    {
      lcd.print("*"); // Show asterisk for entered digit
    }
    else 
    {
      lcd.print("_"); // Show underscore for remaining positions
    }
  }
  
  // Clear any leftover characters if maxLength < 16
  for (int i = maxLength; i < 16; i++)
  {
    lcd.print(" ");
  }
  
  // Position cursor at next underscore
  lcd.setCursor(length, 1);
}
