/* The source Code from : https://github.com/riyadhasan24
 * By Md. Riyad Hasan
 */

 
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Change 0x27 if your LCD has a different I2C address

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Pin setup
const int Solenoid_Pin = 13;
const int Buzzer_Pin = 11;
const int Led_Pin = 12;

// Passwords
const char Master_Password[] = "1122334455";  // Master password (10 digits)
String User_Password = "0000";    // Changeable user password (4 digits)
String Input_Password = "";

// Variables
int Wrong_Attempts = 0;
bool Change_Mode = false; // To know if we are in password change mode
bool Waiting_For_New_Password = false; // To check if waiting for new user password input

void setup() 
{
  pinMode(Solenoid_Pin, OUTPUT);
  pinMode(Buzzer_Pin, OUTPUT);
  pinMode(Led_Pin, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Door Lock System");
  delay(1500);
  lcd.clear();
  lcd.print("Enter Password:");
}

void loop() 
{
  char key = keypad.getKey();

  if (key) 
  {
    if (key >= '0' && key <= '9') 
    {
      if (Change_Mode && !Waiting_For_New_Password) 
      {
        // Master password input (max 10 digits)
        if (Input_Password.length() < 10) 
        {
          Input_Password += key;
          lcd.setCursor(Input_Password.length() - 1, 1);
          lcd.print("*");
        }
      }
      
      else if (Waiting_For_New_Password) 
      {
        // New user password input (max 4 digits)
        if (Input_Password.length() < 4) 
        {
          Input_Password += key;
          lcd.setCursor(Input_Password.length() - 1, 1);
          lcd.print("*");
        }
      }
      
      else 
      {
        // Normal user password input (max 4 digits)
        if (Input_Password.length() < 4) 
        {
          Input_Password += key;
          lcd.setCursor(Input_Password.length() - 1, 1);
          lcd.print("*");
        }
      }
    }
    
    else if (key == 'C') 
    {
      Input_Password = "";
      lcd.setCursor(0, 1);
      lcd.print("                "); // Clear display line fully
      lcd.setCursor(0, 1);
    }
    
    else if (key == '#') 
    { // OK button
      Check_Password();
    }
    
    else if (key == '*') 
    { // Change mode
      Change_Mode = true;
      Waiting_For_New_Password = false;
      lcd.clear();
      lcd.print("Master Password:");
      Input_Password = "";
    }
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
      Waiting_For_New_Password = true; // Now wait for new user password input
    }
    
    else 
    {
      lcd.clear();
      lcd.print("Wrong Master!");
      delay(1000);
      lcd.clear();
      lcd.print("Enter Password:");
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
      delay(1000);
      lcd.clear();
      lcd.print("Enter Password:");
      Change_Mode = false;
      Waiting_For_New_Password = false;
      Input_Password = "";
    }
    
    else 
    {
      lcd.clear();
      lcd.print("4 digits only!");
      delay(1000);
      lcd.clear();
      lcd.print("New Password:");
      Input_Password = "";
    }
  }
  
  else 
  {
    // Normal door open check
    if (Input_Password == User_Password || Input_Password == Master_Password) {
      lcd.clear();
      lcd.print("Access Granted");
      digitalWrite(Solenoid_Pin, HIGH);
      delay(3000);
      digitalWrite(Solenoid_Pin, LOW);
      Wrong_Attempts = 0;
    }
    
    else 
    {
      Wrong_Attempts++;
      lcd.clear();
      lcd.print("Wrong Password!");
      if (Wrong_Attempts >= 3) 
      {
        Warning_Mode();
      }
      delay(1000);
    }
    
    lcd.clear();
    lcd.print("Enter Password:");
    Input_Password = "";
  }
}

void Warning_Mode() 
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alram Activated ");
  lcd.setCursor(0, 1);
  lcd.print("Wrong Password ");
  digitalWrite(Led_Pin, HIGH);
  
  for (int i = 0; i < 5; i++) 
  {
    tone(Buzzer_Pin, 1000);
    delay(300);
    tone(Buzzer_Pin, 500);
    delay(300);
  }
  
  noTone(Buzzer_Pin);
  digitalWrite(Led_Pin, LOW);
  Wrong_Attempts = 0;
}
