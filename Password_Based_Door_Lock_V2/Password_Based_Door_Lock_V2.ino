/* The source Code from : https://github.com/riyadhasan24
 * By Md. Riyad Hasan
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

// Variables
int Wrong_Attempts = 0;
bool Change_Mode = false;
bool Waiting_For_New_Password = false;
unsigned long lockoutEndTime = 0;
unsigned long lastKeyPressTime = 0;
unsigned long lastLCDUpdate = 0;
bool inLockoutMode = false;
bool systemResetNeeded = false; // Flag to indicate system needs reset after lockout

void setup() 
{
  // Initialize pins
  pinMode(Solenoid_Pin, OUTPUT);
  pinMode(Buzzer_Pin, OUTPUT);
  pinMode(Led_Pin, OUTPUT);
  
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
}

void loop() 
{
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
  
  // Reset LCD display to normal state
  lcd.clear();
  lcd.print("Enter Password:");
  lcd.setCursor(0, 1);
  lcd.print("____"); // Show 4 underscores
  lcd.setCursor(0, 1);
  
  // Ensure all outputs are off
  digitalWrite(Solenoid_Pin, LOW);
  digitalWrite(Led_Pin, LOW);
  noTone(Buzzer_Pin);
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
      lcd.clear();
      lcd.print("Enter Password:");
      Change_Mode = false;
      Input_Password = "";
      lcd.setCursor(0, 1);
      lcd.print("____");
      lcd.setCursor(0, 1);
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
      lcd.clear();
      lcd.print("Enter Password:");
      Change_Mode = false;
      Waiting_For_New_Password = false;
      Input_Password = "";
      lcd.setCursor(0, 1);
      lcd.print("____");
      lcd.setCursor(0, 1);
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
      lcd.clear();
      lcd.print("Access Granted!");
      tone(Buzzer_Pin, 1500, 300);
      digitalWrite(Led_Pin, HIGH);
      
      // Open door (activate solenoid)
      digitalWrite(Solenoid_Pin, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("Door Opening...");
      delay(DOOR_OPEN_TIME);
      
      // Close door (deactivate solenoid)
      digitalWrite(Solenoid_Pin, LOW);
      digitalWrite(Led_Pin, LOW);
      
      lcd.clear();
      lcd.print("Door Locked");
      delay(1000);
      
      Wrong_Attempts = 0;
      lcd.clear();
      lcd.print("Enter Password:");
      lcd.setCursor(0, 1);
      lcd.print("____");
      lcd.setCursor(0, 1);
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
        lcd.clear();
        lcd.print("Enter Password:");
        lcd.setCursor(0, 1);
        lcd.print("____");
        lcd.setCursor(0, 1);
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
