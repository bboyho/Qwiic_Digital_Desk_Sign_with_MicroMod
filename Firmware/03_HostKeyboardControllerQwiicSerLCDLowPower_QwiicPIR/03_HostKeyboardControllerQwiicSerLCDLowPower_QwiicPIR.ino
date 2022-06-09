/******************************************************************************
  Host Keyboard Controller with Qwiic Serial LCD and Qwiic PIR
  Date Modified: 5 Aug 2021
  Modified by
  Ho Yun "Bobby" Chan
  Keyboard Controller Code Originally created 8 Oct 2012
  by Cristian Maglie

  ========== DESCRIPTION==========

  This project code takes input from a USB keyboard with
  the SAMD51's USB host pins and outputs characters to
  the Qwiic RGB Serial Enabled LCD 4x20. Leave a message
  behind as you walk away from your desk!

  Note: Not all keyboards are compatible depending on the
  scan codes. This code was tested on the following
  keyboards.

      - Dell KB216t
      - Logitech K120

  This example also builds off the example from Arduino which
  originally showed the output of a USB Keyboard connected to
  the Native USB port on an Arduino Due (SAMD21) Board.

  http://arduino.cc/en/Tutorial/KeyboardController

  This code is part of the public domain.

******************************************************************************/

#include <KeyboardController.h> // Require keyboard control library
#include <SparkFun_Qwiic_PIR.h>
QwiicPIR pir;

//#include <Wire.h> //Was needed for I2C to SerLCD and Qwiic PIR but it is defined in the Qwiic PIR library so we comment it out

#define SerLCD_Address 0x72  //If using SerLCD with I2C
#define SERIAL_PORT_MONITOR Serial1 //debug via hardware UART pins

//assuming that we are using a SerLCD 20x4 screen
//these variables keep track of cursor location
int row = 0;
int remappedRow = row;
int column = 0;

//depending on what screen you are using, we are counting 0 as well
//int maxRow = 1;     // for 16x2
int maxRow = 3;        //for 20x4
//int maxColumn = 15; // for 16x2
int maxColumn = 19;    //for 20x4

boolean rgb_backlight = true; //used for function keys to immediately turn on/off backlight
boolean blink_box = true; //used to keep track of blink box as a "cursor"

//values to keep track of rgb backlight
int rVal = 157; //128 = Off, 157 = 100%
int gVal = 187; //158 = Off, 187 = 100%
int bVal = 217; //188 = Off, 217 = 100%

//values used to turn off rgb in Power Save Mode
int rVal_OFF = 128;
int gVal_OFF = 158;
int bVal_OFF = 188;

int lcdContrast = 40; //used to keep track of contast: Range is 255 to 0, 40 is default


boolean activity = true; //used to keep track of movement or keyboard presses
const int noActivityMillis = 10000; //used to compare amount of time when no activity to turn of screen
int currentMillis = 0;  //get time based on how long the Arduino has been running
int lastActivityMillis = 0;





// Initialize USB Controller
USBHost usb;

// Attach keyboard controller to USB
KeyboardController keyboard(usb);

//boolean capsLock = false; //used to keep track of capsLock, this is not used in this code
boolean numLock = false; //display numbers if numLock on, we'll assume that the keyboard resets every time so it's off by default




// This function intercepts key press
void keyPressed() {
  SERIAL_PORT_MONITOR.print("Pressed:  ");
  //printKey(); //disabled so we are not sending two key presses to the SerLCD
}




// This function intercepts key release
void keyReleased() {
  SERIAL_PORT_MONITOR.print("Released: ");
  printKey();
}




void printKey() {
  activity = true;

  // getOemKey() returns the OEM-code associated with the key
  int tempKey = keyboard.getOemKey();
  SERIAL_PORT_MONITOR.print(" key:");
  SERIAL_PORT_MONITOR.print(tempKey);

  // getModifiers() returns a bits field with the modifiers-keys
  int mod = keyboard.getModifiers();
  SERIAL_PORT_MONITOR.print(" mod:");
  SERIAL_PORT_MONITOR.print(mod);

  SERIAL_PORT_MONITOR.print(" => ");

  if (mod & LeftCtrl)
    SERIAL_PORT_MONITOR.print("L-Ctrl ");
  if (mod & LeftShift)
    SERIAL_PORT_MONITOR.print("L-Shift ");
  if (mod & Alt)
    SERIAL_PORT_MONITOR.print("Alt ");
  if (mod & LeftCmd)
    SERIAL_PORT_MONITOR.print("L-Cmd ");
  if (mod & RightCtrl)
    SERIAL_PORT_MONITOR.print("R-Ctrl ");
  if (mod & RightShift)
    SERIAL_PORT_MONITOR.print("R-Shift ");
  if (mod & AltGr)
    SERIAL_PORT_MONITOR.print("AltGr ");
  if (mod & RightCmd)
    SERIAL_PORT_MONITOR.print("R-Cmd ");

  // getKey() returns the ASCII translation of OEM key
  // combined with modifiers.
  SERIAL_PORT_MONITOR.write(keyboard.getKey());
  SERIAL_PORT_MONITOR.println();

  /*
    USB SCAN CODES TO KEYCAP (US LAYOUT)
    https://www.win.tue.nl/~aeb/linux/kbd/scancodes-10.html

      CHARACTER KEYS
      USB SCAN CODE = KEYCAP

      4 to 29 = 'a' to 'z' and 'A' to 'Z' with Shift/CapsLock
      30 to 39 = '1' to '9', '0', and '!@#$%^&*()' with Shift
      40 = 'Enter'
      42 = 'Backspace'
      44 = 'Space'
      45 = '-' or '_' with Shift
      46 = '=' or '+' with Shift
      47 = '[' or '{' with Shift
      48 = ']' or '}' with Shift
      49 = '\' or '|' with Shift
      51 = ';' or ':' with Shift
      52 = `'` or '"' with Shift
      53 = '`' or '~' with Shift
      54 = ',' or '<' with Shift
      55 = '.' or '>' with Shift
      56 = '/' or '?' with Shift

      CURSOR CONTROL D-PAD (US LAYOUT)
      79 = `→` (e.g. right)
      80 = `←` (e.g. left)
      81 = `↓` (e.g. down)
      82 = `↑` (e.g. up)

      NUMERIC KEYPAD (US LAYOUT), NUMLOCK MUST BE ENABLED
      83 = 'NumLock'
      84 = '/'
      85 = '*'
      86 = '-'
      87 = '+'
      88 = 'Enter'
      89 = 'End'             or '1' w/ NumLock
      90 = '↓' (e.g. down)   or '2' w/ NumLock
      91 = 'Page Down'       or '3' w/ NumLock
      92 = '←' (e.g. left)   or '4' w/ NumLock
      93 =                      '5' w/ NumLock
      94 = '→' (e.g. right)  or '6' w/ NumLock
      95 = 'Home'             or '7' w/ NumLock
      96 = '↑' (e.g. up)      or '8' w/ NumLock
      97 = 'Page Up'          or '9' w/ NumLock
      98 = 'Insert'           or '0' w/ NumLock
      99 = 'Delete'           or '.' w/ NumLock

      OTHER
      41 = 'Escape'
      73 = 'Insert'
      74 = 'Home'
      75 = 'Page Up'
      76 = 'Delete'
      77 = 'End'
      78 = 'Page Down'

      FUNCTION KEYS
      58 = `F1`
      59 = `F2`
      60 = `F3`
      61 = `F4`
      62 = `F5`
      63 = `F6`
      64 = `F7`
      65 = `F8`
      66 = `F9`
      67 = `F10`
      68 = `F11`
      69 = `F12`



  */

  //----------TYPEWRITER AND KEYPAD KEYS (QWERTY, US LAYOUT) ----------
  if ((tempKey >= 4 && tempKey <= 39) ||
      (tempKey >= 44 && tempKey <= 49) ||
      (tempKey >= 51 && tempKey <= 56) ||
      (tempKey >= 84 && tempKey <= 87) ||
      ((tempKey >= 89 && tempKey <= 97) && numLock == true) ||
      tempKey == 73 ||
      tempKey == 98 ||
      (tempKey == 99 && numLock == true)) {

    Wire.beginTransmission(SerLCD_Address);

    if ( (tempKey == 49) && ((mod == 2) || (mod == 32)) ) {
      //Note: When sending the pipeline character(`|`),
      //we'll need to send 2x since the character
      //is also used as a setting character.
      Wire.write(keyboard.getKey());
      delay(50);//short delay before sending next line
      Wire.write(keyboard.getKey());
    }
    else if ((tempKey == 49) && (mod == 0)) {
      //Note: When sending back slash (`\`),
      //we will load the custom character using
      //printCustomChar() through I2C
      printCustomChar(0);
    }
    else if ( (tempKey == 53) && ((mod == 2) || (mod == 32)) ) {
      //Note: When sending tilde (`~`),
      //we will load the custom character using
      //printCustomChar() through I2C
      printCustomChar(1);
    }
    else if (tempKey == 73) {
      //Note: When sending 'insert',
      //we will load the custom character '♥' using
      //printCustomChar() through I2C
      printCustomChar(2);
    }
    else if (tempKey == 98 && numLock == false) {
      //Note: When sending keypad insert,
      //we will load the custom character '♡' using
      //printCustomChar() through I2C
      printCustomChar(3);
    }
    else {
      Wire.write(keyboard.getKey());
    }


    //after typing the cursor will move automatically to
    //next position so let's keep track of it
    if (column < maxColumn) {
      //if we are not at the end of the row, move cursor to next position
      column = column + 1;
    }
    else {
      //if we are at the end of the row, reset cursor to the beginning of the row
      column = 0;

      if (row < maxRow) {
        //if we are not on the last line, move cursor to the next line
        row = row + 1;
      }
      else {
        //if we are on the last line, move cursor to the first line
        row = 0;
      }
    }

    //remap according to SerLCD's line number
    if (row == 0) {
      remappedRow = 0;
    }
    else if (row == 1) {
      remappedRow = 64;
    }
    else if (row == 2) {
      remappedRow = 20;
    }
    else if (row == 3) {
      remappedRow = 84;
    }

    Wire.write(254); //Send command character
    Wire.write(128 + remappedRow + column); //update cursor position
    Wire.endTransmission(); //Stop I2C transmission

  }





  /*NOTE: These keys move the cursor so it is kept
    as separate condition statements. We will
    also include most of the numLock keys when 'NUMLOCK'
    is disabled.*/
  else if ((tempKey >= 79 && tempKey <= 82) ||
           tempKey == 40 ||
           tempKey == 74 ||
           tempKey == 77 ||
           tempKey == 75 ||
           tempKey == 78 ||
           tempKey == 83 ||
           tempKey == 88 ||
           ((tempKey >= 89 && tempKey <= 97) && numLock == false) ) {


    //move cursor based on d-pad or Enter key

    //----------NUMLOCK KEY----------
    if (tempKey == 83) {
      numLock = !numLock;// change state of numLock if pressed by inverting it
    }




    //----------LEFT KEY----------
    else if ((tempKey == 80) || (tempKey == 92 && numLock == false)) {

      if (column > 0) {
        //if we are at the beginning of the row
        column = column - 1;
      }
      else {
        //if we are at the end of the row
        column = maxColumn;

        if (row > 0) {
          //if we are not on the last line move up a line
          row = row - 1;
        }
        else {
          //if we are on the last line
          row = maxRow;
        }
      }
    }



    //----------RIGHT KEY----------
    else if ( (tempKey == 79) || (tempKey == 94 && numLock == false) ) {

      if (column < maxColumn) {
        //if we are at the before the end of the row
        column = column + 1;
      }
      else {
        //if we are at the end of the row, reset cursor position
        column = 0;

        if (row < maxRow) {
          //move to next line if we are before the last line
          row = row + 1;
        }
        else {
          //move to the first line if we have reached the end
          row = 0;
        }
      }
    }



    //----------UP KEY----------
    else if ( (tempKey == 82) || (tempKey == 96 && numLock == false) ) {
      if (row > 0) {
        //if we are not on the first line
        row = row - 1;
      }
      else {
        //if we are on the first line, move cursor to the last line
        row = maxRow;
      }
    }



    //----------DOWN KEY----------
    else if ( (tempKey == 81) || (tempKey == 90 && numLock == false) ) {
      if (row < maxRow) {
        //if we are not on the first line
        row = row + 1;
      }
      else {
        //if we are on the last line, move cursor to the first line
        row = 0;
      }
    }



    //----------ENTER KEY----------
    else if (tempKey == 40 || tempKey == 88) {
      //move to the first position of the next line
      column = 0;
      if (row < maxRow) {
        //if we are not on the first line, move cursor to the next line
        row = row + 1;
      }
      else {
        //if we are on the last line, move cursor to the first line
        row = 0;
      }
    }

    //----------HOME KEY----------
    else if (tempKey == 74 || (tempKey == 95 && numLock == false) ) {
      //move to the first position of the next line
      column = 0;
    }

    //----------END KEY----------
    else if (tempKey == 77 || (tempKey == 89 && numLock == false)) {
      //move to the first position of the next line
      column = maxColumn;
    }

    //----------PAGE UP KEY----------
    else if (tempKey == 75 || (tempKey == 97 && numLock == false)) {
      //move to the top row
      row = 0;
    }

    //----------PAGE DOWN KEY----------
    else if (tempKey == 78 || (tempKey == 91 && numLock == false)) {
      //move to the bottom row
      row = maxRow;
    }

    else if (tempKey == 93 && numLock == false ) {
      blink_box = !blink_box;// change state of blink_box if pressed by inverting it

      Wire.beginTransmission(SerLCD_Address);
      Wire.write(254); //Send command character

      if (blink_box == false) {
        Wire.write( (1 << 3) | (1 << 2) ); //Cursor off, blinking box off
      }
      else {
        //if blink_box == true

        Wire.write( (1 << 3) | (1 << 2) | (1 << 0) ); //Cursor off, blinking box on
      }

      Wire.endTransmission(); //Stop I2C transmission

    }


    //remap according to SerLCD's line number
    if (row == 0) {
      remappedRow = 0;
    }
    else if (row == 1) {
      remappedRow = 64;
    }
    else if (row == 2) {
      remappedRow = 20;
    }
    else if (row == 3) {
      remappedRow = 84;
    }

    Wire.beginTransmission(SerLCD_Address);
    Wire.write(254); //Send command character
    Wire.write(128 + remappedRow + column); //update cursor position
    Wire.endTransmission(); //Stop I2C transmission

  }




  //----------ESCAPE KEY----------
  else if (tempKey == 41) {
    //Make the escape key clear screen and reset cursor position.
    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write('-'); //Send clear display command
    //Control the cursor
    Wire.write(254); //Send command character
    Wire.write( (1 << 3) | (1 << 2) | (1 << 0) ); //Cursor on, blinking box on
    blink_box = true;
    // reset cursor position to (0,0) after using clear display command
    row = 0;
    remappedRow = row;
    column = 0;

    Wire.write(254); //Send command character
    Wire.write(128 + remappedRow + column); //update cursor position
    Wire.endTransmission(); //Stop I2C transmission

  }



  //----------BACKSPACE KEY----------
  else if (tempKey == 42) {
    //Backspace
    //- 'Delete' by replacing with 'Space',
    //- move backward,
    //- then move backward again since cursor moves after sending character


    for (int i = 0 ; i < 2; i++) {

      if (i == 0) {
        //"Delete character" by replacing character with a space
        //and then moving cursor backwards after this condition statement.
        //We do this only once.

        Wire.beginTransmission(SerLCD_Address);
        Wire.write(32); //Send 'Space' Character
        Wire.endTransmission(); //Stop I2C transmission

        //cursor moved automatically to next position so let's keep track of it
        if (column < maxColumn) {
          column = column + 1;
        }
        else {
          column = 0;

          if (row < maxRow) {
            row = row + 1;
          }
          else {
            row = 0;
          }
        }
      }

      //For backspace, we are replacing the current position
      //with a space. This moves the cursor forward once space
      //To correct this, we are going to move backward twice
      //with the help of the for() loop.
      if (column > 0) {
        column = column - 1;
      }
      else {
        column = maxColumn;

        if (row > 0) {
          row = row - 1;
        }
        else {
          row = maxRow;
        }
      }


      //remap according to SerLCD's line number
      if (row == 0) {
        remappedRow = 0;
      }
      else if (row == 1) {
        remappedRow = 64;
      }
      else if (row == 2) {
        remappedRow = 20;
      }
      else if (row == 3) {
        remappedRow = 84;
      }

      Wire.beginTransmission(SerLCD_Address);
      Wire.write(254); //Send command character
      Wire.write(128 + remappedRow + column); //update cursor position
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);




    }//end for() loop for controlling cursor with 'Backspace'
  }//end condition statement for 'Backspace'



  //----------DELETE KEY----------
  else if (tempKey == 76 || (tempKey == 99 && numLock == false) ) {
    //'Delete'
    //- 'Delete' by replacing with 'Space',
    //- move backward again since cursor moves after sending character
    //Note: Delete for the SerLCD is not like the traditional
    //forward delete key. For the SerLCD, we are simply
    //going to delete the character at the cursor position.
    //The characters ahead of the cursor will not be shifted
    //to the left.

    Wire.beginTransmission(SerLCD_Address);
    Wire.write(32); //Send 'Space' Character

    //cursor moved automatically to next position so let's keep track of it
    if (column < maxColumn) {
      column = column + 1;
    }
    else {
      column = 0;

      if (row < maxRow) {
        row = row + 1;
      }
      else {
        row = 0;
      }
    }

    //move cursor back to where it was
    if (column > 0) {
      column = column - 1;
    }
    else {
      column = maxColumn;

      if (row > 0) {
        row = row - 1;
      }
      else {
        row = maxRow;
      }
    }


    //remap according to SerLCD's line number
    if (row == 0) {
      remappedRow = 0;
    }
    else if (row == 1) {
      remappedRow = 64;
    }
    else if (row == 2) {
      remappedRow = 20;
    }
    else if (row == 3) {
      remappedRow = 84;
    }

    Wire.write(254); //Send command character
    Wire.write(128 + remappedRow + column); //update cursor position
    Wire.endTransmission(); //Stop I2C transmission


  }//end condition statement for 'Delete'



  else if (tempKey == 58) {
    //`F1`
    //rVal-

    if (rVal > 128) {
      rVal = rVal - 1;
    }
    else {
      rVal = 128;
    }

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write(rVal); //Update red value
    Wire.endTransmission(); //Stop I2C transmission

  }


  else if (tempKey == 59) {
    //`F2`
    //rVal+

    if (rVal < 157 ) {
      rVal = rVal + 1;
    }
    else {
      rVal = 157;
    }

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write(rVal); //Update red value
    Wire.endTransmission(); //Stop I2C transmission
  }


  else if (tempKey == 60) {
    //`F3`
    //gVal-

    if (gVal > 158 ) {
      gVal = gVal - 1;
    }
    else {
      gVal = 158;
    }

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write(gVal); //Update green value
    Wire.endTransmission(); //Stop I2C transmission

  }


  else if (tempKey == 61) {
    //`F4`
    //gVal+

    if (gVal < 187 ) {
      gVal = gVal + 1;
    }
    else {
      gVal = 187;
    }

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write(gVal); //Update green value
    Wire.endTransmission(); //Stop I2C transmission

  }

  else if (tempKey == 62) {
    //`F5`
    //bVal-

    if (bVal > 188 ) {
      bVal = bVal - 1;
    }
    else {
      bVal = 188;
    }

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write(bVal); //Update blue value
    Wire.endTransmission(); //Stop I2C transmission
  }

  else if (tempKey == 63) {
    //`F6`
    //bVal+

    if (bVal < 217 ) {
      bVal = bVal + 1;
    }
    else {
      bVal = 217;
    }

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write(bVal); //Update blue value
    Wire.endTransmission(); //Stop I2C transmission

  }

  else if (tempKey == 64) {
    //`F7`
    //backlight ON/OFF

    if (rVal == 128 && gVal == 158 &&  bVal == 188)
    { //if all values are off, we will turn it all ON in the next condition statement
      rgb_backlight = 0;
    }
    else {
      //if any of the LEDs is partially on, we will turn it all OFF in the next condition statement
      rgb_backlight = 1;
    }


    if (rgb_backlight == 0)
    { // rgb_backlight == false //OFF, so turn ON

      rgb_backlight = 1;// set it ON
      rVal = 157;
      gVal = 187;
      bVal = 217;


      Wire.beginTransmission(SerLCD_Address);

      Wire.write('|'); //Put LCD into setting mode
      Wire.write(rVal); //Set red backlight

      Wire.write('|'); //Put LCD into setting mode
      Wire.write(gVal); //Set green backlight

      Wire.write('|'); //Put LCD into setting mode
      Wire.write(bVal); //Set blue backlight

      Wire.endTransmission(); //Stop I2C transmission

    }
    else
    { // rgb_backlight == true //ON, so turn OFF

      rgb_backlight = 0;// set it OFF
      rVal = 128;
      gVal = 158;
      bVal = 188;

      Wire.beginTransmission(SerLCD_Address);

      Wire.write('|'); //Put LCD into setting mode
      Wire.write(rVal); //Set red backlight

      Wire.write('|'); //Put LCD into setting mode
      Wire.write(gVal); //Set green backlight

      Wire.write('|'); //Put LCD into setting mode
      Wire.write(bVal); //Set blue backlight

      Wire.endTransmission(); //Stop I2C transmission

    }


  }


  else if (tempKey == 65) {
    //`F8`
    //set custom color, let's set it to cyan (0%, 100%, 100%)

    rVal = 128;
    gVal = 187;
    bVal = 217;

    Wire.beginTransmission(SerLCD_Address);

    Wire.write('|'); //Put LCD into setting mode
    Wire.write(rVal); //Set red backlight

    Wire.write('|'); //Put LCD into setting mode
    Wire.write(gVal); //Set green backlight

    Wire.write('|'); //Put LCD into setting mode
    Wire.write(bVal); //Set blue backlight
    Wire.endTransmission(); //Stop I2C transmission

  }




  else if (tempKey == 66) {
    //`F9`
    //Custom Message 1

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write('-'); //Send clear display command

    // reset cursor position to (0,0) after using clear display command
    row = 0;
    remappedRow = row;
    column = 0;


    if (maxRow == 3 && maxColumn == 19) {
      //Note: We'll send the lines in separate I2C transmissions
      //instead of all at once. Sending several lines freezes the SerLCD.

      Wire.print("Gone dancing! I'll  ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("be back at 12:30pm! ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("         =)         ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("SparkFun Electronics");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line


      //move cursor position so it's out of the way of the text;
      row = 2;
      remappedRow = 20;
      column = 19;
    }
    else {
      //Note: We'll send the lines in separate I2C transmissions
      //instead of all at once. Sending several lines freezes the SerLCD.

      Wire.print("Gone dancing!   ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("BRB at 12:30pm! ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      //move cursor position so it's out of the way of the text;
      row = 1;
      remappedRow = 64;
      column = 15;
    }

    Wire.beginTransmission(SerLCD_Address);

    Wire.write(254); //Send command character
    Wire.write(128 + remappedRow + column); //update cursor position
    Wire.endTransmission(); //Stop I2C transmission
    delay(50);//short delay before sending next line

  }




  else if (tempKey == 67) {
    //`F10`
    //Custom Message 2

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write('-'); //Send clear display command

    // reset cursor position to (0,0) after using clear display command
    row = 0;
    remappedRow = row;
    column = 0;

    if (maxRow == 3 && maxColumn == 19) {

      //Note: We'll send the lines in separate I2C transmissions
      //instead of all at once. Sending several lines freezes the SerLCD.

      Wire.print("Out for lunch! I'll ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("be back at  2:00pm! ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("        ^_^         ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("SparkFun Electronics");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line


      //move cursor position so it's out of the way of the text;
      row = 2;
      remappedRow = 20;
      column = 19;
    }
    else {
      //Note: We'll send the lines in separate I2C transmissions
      //instead of all at once. Sending several lines freezes the SerLCD.

      Wire.print("Out for lunch!  ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("BRB at  2:00pm! ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      //move cursor position so it's out of the way of the text;
      row = 1;
      remappedRow = 64;
      column = 15;
    }

    Wire.beginTransmission(SerLCD_Address);

    Wire.write(254); //Send command character
    Wire.write(128 + remappedRow + column); //update cursor position
    Wire.endTransmission(); //Stop I2C transmission
    delay(50);//short delay before sending next line

  }




  else if (tempKey == 68) {
    //`F11`
    //Custom Message 3

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write('-'); //Send clear display command

    // reset cursor position to (0,0) after using clear display command
    row = 0;
    remappedRow = row;
    column = 0;


    if (maxRow == 3 && maxColumn == 19) {

      //Note: We'll send the lines in separate I2C transmissions
      //instead of all at once. Sending several lines freezes the SerLCD.

      Wire.print("In a meeting! I'll  ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("be back at  2:00pm! ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("                    ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("SparkFun Electronics");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line


      //move cursor position so it's out of the way of the text;
      row = 2;
      remappedRow = 20;
      column = 19;
    }
    else {
      //Note: We'll send the lines in separate I2C transmissions
      //instead of all at once. Sending several lines freezes the SerLCD.

      Wire.print("In a meeting!   ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("BRB at  2:00pm! ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      //move cursor position so it's out of the way of the text;
      row = 1;
      remappedRow = 64;
      column = 15;
    }

    Wire.beginTransmission(SerLCD_Address);

    Wire.write(254); //Send command character
    Wire.write(128 + remappedRow + column); //update cursor position
    Wire.endTransmission(); //Stop I2C transmission
    delay(50);//short delay before sending next line

  }



  else if (tempKey == 69) {
    //`F12`
    //Custom Message 4

    Wire.beginTransmission(SerLCD_Address);
    Wire.write('|'); //Put LCD into setting mode
    Wire.write('-'); //Send clear display command

    // reset cursor position to (0,0) after using clear display command
    row = 0;
    remappedRow = row;
    column = 0;

    if (maxRow == 3 && maxColumn == 19) {

      //Note: We'll send the lines in separate I2C transmissions
      //instead of all at once. Sending several lines freezes the SerLCD.

      Wire.print("Bobby's desk is here");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("   Working remote!  ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print(" See you virtually! ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("SparkFun Electronics");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line


      //move cursor position so it's out of the way of the text;
      row = 2;
      remappedRow = 20;
      column = 19;
    }
    else {
      //Note: We'll send the lines in separate I2C transmissions
      //instead of all at once. Sending several lines freezes the SerLCD.

      Wire.print("Bobby's desk is ");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      Wire.beginTransmission(SerLCD_Address);
      Wire.print("here!GoneVirtual");
      Wire.endTransmission(); //Stop I2C transmission
      delay(50);//short delay before sending next line

      //move cursor position so it's out of the way of the text;
      row = 1;
      remappedRow = 64;
      column = 15;
    }

    Wire.beginTransmission(SerLCD_Address);

    Wire.write(254); //Send command character
    Wire.write(128 + remappedRow + column); //update cursor position
    Wire.endTransmission(); //Stop I2C transmission
    delay(50);//short delay before sending next line

  }



  delay(50); //The maximum update rate of OpenLCD is about 100Hz (10ms). A smaller delay will cause flicker

}





//Set Custom Characters for 5x8 Character Position
//'\'
//0x0,0x10,0x8,0x4,0x2,0x1,0x0,0x0
//%0,%10000,%1000,%100,%10,%1,%0,%0
byte back_slash[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00001,
  0b00000,
  0b00000
};





//'~'
//0x0,0x0,0x0,0x8,0x15,0x2,0x0,0x0
//%0,%0,%1000,%10101,%10,%0,%0,%0
byte tilde[8] = {
  0b00000,
  0b00000,
  0b01000,
  0b10101,
  0b00010,
  0b00000,
  0b00000,
  0b00000
};





//'♥'
//0x0,0x0,0xa,0x1f,0x1f,0xe,0x4,0x0
//%0,%0,%1010,%11111,%11111,%1110,%100,%0
byte heart[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};





//'♡'
//0x0,0x0,0xa,0x15,0x11,0xa,0x4,0x0
//%0,%0,%1010,%10101,%10001,%1010,%100,%0
byte empty_heart[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b10101,
  0b10001,
  0b01010,
  0b00100,
  0b00000
};





//Given a character number (0 to 7 is valid)
//Given an 8 byte array
//Record this data as a custom character to CGRAM
void loadCustomCharacter(byte charNumber, byte charData[])
{
  if (charNumber > 7) charNumber = 7; //Error correction

  Wire.write('|'); //Send setting character
  Wire.write(27 + charNumber); //27 is the first custom character spot

  for (byte x = 0 ; x < 8 ; x++) //There are 8 bytes of data we need to load
    Wire.write(charData[x]); //Write 8 bytes of graphic data to display
}





//Display a given custom character that was previously loaded into CGRAM
void printCustomChar(byte charNumber)
{
  if (charNumber > 7) charNumber = 7; //Error correction

  Wire.write('|'); //Send setting character
  Wire.write(35 + charNumber); //Tell LCD to display custom char # 0-7
}





void setup() {
  SERIAL_PORT_MONITOR.begin(115200);
  //while (!SERIAL_PORT_MONITOR); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  SERIAL_PORT_MONITOR.println("Keyboard Controller Program started");

  if (usb.Init() == -1)
    SERIAL_PORT_MONITOR.println("OSC did not start.");

  delay(3000);//wait a second for the SerLCD to initialize before setting it up

  Wire.begin(); //Join the I2C bus
  Wire.setClock(400000);   // Set clock speed to be the fastest for better communication (fast mode)

  Wire.beginTransmission(SerLCD_Address);
  //Send custom characters to display
  //These are recorded to SerLCD and are remembered even after power is lost
  //There is a maximum of 8 custom characters that can be recorded
  loadCustomCharacter(0, back_slash);
  delay(50);
  loadCustomCharacter(1, tilde);
  delay(50);
  Wire.endTransmission(); //Stop I2C transmission

  Wire.beginTransmission(SerLCD_Address);
  loadCustomCharacter(2, heart);
  delay(50);
  loadCustomCharacter(3, empty_heart);
  delay(50);

  Wire.write('|'); //Put LCD into setting mode
  Wire.write(rVal); //Set red backlight

  Wire.write('|'); //Put LCD into setting mode
  Wire.write(gVal); //Set green backlight

  Wire.write('|'); //Put LCD into setting mode
  Wire.write(bVal); //Set blue backlight

  Wire.endTransmission(); //Stop I2C transmission

  delay(50);//short delay before sending next line

  Wire.beginTransmission(SerLCD_Address);
  Wire.write('|'); //Put LCD into setting mode
  Wire.write('-'); //Send clear display command
  //Control the cursor
  Wire.write(254); //Send command character
  Wire.write( (1 << 3) | (1 << 2) | (1 << 0) ); //Cursor on, blinking box on
  blink_box = true;
  Wire.endTransmission(); //Stop I2C transmission

  delay(50); //short delay before sending next line

  //check if pir will acknowledge over I2C
  if (pir.begin() == false) {
    SERIAL_PORT_MONITOR.println("Device did not acknowledge! Freezing.");
    Wire.beginTransmission(SerLCD_Address);
    Wire.print("PIR !Connected");
    Wire.endTransmission(); //Stop I2C transmission
    while (1);
  }

  SERIAL_PORT_MONITOR.println("PIR acknowledged. Waiting 30 Seconds while PIR warms up");
  Wire.beginTransmission(SerLCD_Address);
  Wire.print("Waiting 30 secs for PIR up");
  Wire.endTransmission(); //Stop I2C transmission
  for (uint8_t seconds = 0; seconds < 30; seconds++)
  {
    SERIAL_PORT_MONITOR.println(seconds);

    delay(1000);
  }

  SERIAL_PORT_MONITOR.println("PIR warm!");

  //Use this function call to change the debounce time of the PIR sensor
  //The parameter is the debounce time in milliseconds
  pir.setDebounceTime(500);

  Wire.beginTransmission(SerLCD_Address);
  Wire.write('|'); //Put LCD into setting mode
  Wire.write('-'); //Send clear display command
  Wire.endTransmission(); //Stop I2C transmission
  delay(50); //short delay before sending next line

  Wire.beginTransmission(SerLCD_Address);
  Wire.write('|'); //Send command character
  Wire.write('/'); //disable system messages since current buffer on the SerLCD does not save what is on screen or custom characters correctly
  Wire.endTransmission(); //Stop I2C transmission
  delay(50); //short delay before sending next line




}





void loop() {
  if (activity == true) {

    lastActivityMillis = millis(); //save time when event occurred

    if (lcdContrast != 5) {

      //turn on screen once
      Wire.beginTransmission(SerLCD_Address);
      //Control the cursor and display
      Wire.write('|'); //Put LCD into setting mode
      Wire.write(24); //Value to change contrast
      Wire.write(5); //Contrast from 255 to 0; 5 is default

      //turn on backlight to previous value
      Wire.write('|'); //Put LCD into setting mode
      Wire.write(rVal); //Set red backlight

      Wire.write('|'); //Put LCD into setting mode
      Wire.write(gVal); //Set green backlight

      Wire.write('|'); //Put LCD into setting mode
      Wire.write(bVal); //Set blue backlight

      Wire.endTransmission(); //Stop I2C transmission

      lcdContrast = 5;
    }
    activity = false; //reset flag
  }
  else {
    //check time to see if we are over 10s
    currentMillis = millis();

    if ( ( currentMillis - lastActivityMillis ) < noActivityMillis ) {
      //do nothing since t < 10s
    }
    else {
      //it's been over 10s then let's turn off screen

      if (lcdContrast != 255) {
        //turn off screen once
        Wire.beginTransmission(SerLCD_Address);
        //display off by turning contrast off
        Wire.write('|'); //Put LCD into setting mode
        Wire.write(24); //Value to change contrast
        Wire.write(255);//Contrast from 255 to 0; 5 is default

        delay(50);

        Wire.write('|'); //Put LCD into setting mode
        Wire.write(rVal_OFF); //Set red backlight

        Wire.write('|'); //Put LCD into setting mode
        Wire.write(gVal_OFF); //Set green backlight

        Wire.write('|'); //Put LCD into setting mode
        Wire.write(bVal_OFF); //Set blue backlight

        Wire.endTransmission(); //Stop I2C transmission
        lcdContrast = 255;
      }
    }
  }

  delay(50);




  // Process USB tasks
  usb.Task();

  //check if there is an available PIR event, and tell us what it is!
  if (pir.available()) {
    if (pir.objectDetected()) {
      SERIAL_PORT_MONITOR.println("Object Detected");
      activity = true;
    }
    if (pir.objectRemoved()) {
      SERIAL_PORT_MONITOR.println("Object Removed");
    }
    pir.clearEventBits();
  }

}





//Given a number, i2cSendValue chops up an integer into four values and sends them out over I2C
void i2cSendValue(int value)
{
  Wire.beginTransmission(SerLCD_Address); // transmit to device #1

  Wire.write('|'); //Put LCD into setting mode
  Wire.write('-'); //Send clear display command

  Wire.endTransmission(); //Stop I2C transmission
}
