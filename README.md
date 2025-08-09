# Password_Based_Door_Lock_System

This is a simple Arduino-based door lock system using a keypad and an I2C LCD. The system supports a 10-digit master password for administrative access and a 4-digit user password for normal door unlocking. The user password can be changed securely by entering the master password first.


Features:
4x4 Matrix Keypad input
I2C LCD display for user prompts and feedback
Master password (10 digits) to enter password change mode
User password (4 digits), changeable after master password authentication
Solenoid lock control
Buzzer and LED alarm triggered after 3 wrong attempts


Hardware Required:
Arduino board (Uno, Mega, etc.)
4x4 Matrix Keypad
LCD Display (16x02) With I2C Module
Solenoid lock (controlled via a relay or MOSFET)
Buzzer
LED
Connecting wires and power supply


Wiring:
Keypad connected to digital pins 2 to 9
LCD connected via I2C (SDA, SCL pins, typically A4, A5 on Uno)
Solenoid lock control pin: 13
Buzzer pin: 11
LED pin: 12


Usage:
Upload the code to your Arduino board.
On startup, the LCD prompts "Enter Password:".
Enter the 4-digit user password to unlock the door.
To change the user password, press *, then enter the 10-digit master password (1122334455).
Enter a new 4-digit user password to update it.
After 3 consecutive wrong attempts, the alarm will sound.


Contact:
For any questions or support:
WhatsApp: +8801730288553
Email: riyadhasan24a@gmail.com
Github: github.com/riyadhasan24

