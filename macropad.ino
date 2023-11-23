/*******************************************************************
 * RYAN UPDATE THIS (MAYBE LATER ITS DEC 24 AND I JUST GOT THE VIDEO DONE)
 * (c) 2020 Ryan Bates

  Last time I touched this: Dec 13 2020
  web: www.retrobuiltgames.com
  youtube: https://www.youtube.com/c/ryanbatesrbg
  twitter: @retrobuiltgames
  
Ryan's Pro Tips:
=============== Keyboard Control================================================================================
Keyboard.write();   Sends a keystroke to a connected computer. 
                    Can also accept ASCII code like this:
                    //Keyboard.write(32); // This is space bar (in decimal)
                    Helpful list of ASCII + decimal keys http://www.asciitable.com/
                    
Keyboard.press();   Best for holding down a key with multi-key commands; like copy/ paste
                    This example is [ctrl] + [shift] + [e];
                
Keyboard.print();   Sends a keystroke(s)
                    Keyboard.print("stop using aim bot"); // types this in as a char or int! (not a string)!

Keyboard.println(); Sends a keystroke followed by a newline (carriage return)
                     Very practical if you want to type a password and login in one button press!
                     
=============== Mouse Control================================================================================
Mouse.move(x, y, wheel);  Moves the mouse and or scroll wheel up/down left/right.
                          Range is -128 to +127. units are pixels 
                          -number = left or down
                          +number = right or up

Mouse.press(b);         Presses the mouse button (still need to call release). Replace "b" with:
                        MOUSE_LEFT   //Left Mouse button
                        MOUSE_RIGHT  //Right Mouse button
                        MOUSE_MIDDLE //Middle mouse button
                        MOUSE_ALL    //All three mouse buttons
                        
Mouse.release(b);       Releases the mouse button.

Mouse.click(b);         A quick press and release.

**********************************************************************************************************/

// --------------------------------------------------------------
// Standard Libraries
// --------------------------------------------------------------

//#include "Keyboard.h"
#include "HID-Project.h"
//#include <Mouse.h> //there are some mouse move functions for encoder_Mode 2 and 3  CONFLCTS WITH MOUSE
#include <Keypad.h>
// This library is for interfacing with the 3x4 Matrix
// Can be installed from the library manager, search for "keypad"
// and install the one by Mark Stanley and Alexander Brevig
// https://playground.arduino.cc/Code/Keypad/

const byte ROWS = 3; //four rows
const byte COLS = 4; //four columns

char previousPressedKey;
boolean hasReleasedKey = false;  //use for button Hold mode. Only works with one button at a time for now...

#include <Encoder.h>   
//Library for simple interfacing with encoders (up to two)
//low performance ender response, pins do not have interrupts
Encoder RotaryEncoderA(10, 16); //the LEFT encoder (encoder A)

// --------------------------------------------------------------
// Additional Libraries - each one of these will need to be installed to use the special features like i2c LCD and RGB LEDs.
// --------------------------------------------------------------
//#include <LiquidCrystal_I2C.h>
//LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address for a 40 chars and 4 line display
#include <Wire.h>
//#include <hd44780.h>
//#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header
//hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

const int LCD_NB_ROWS = 4 ;           //for the 4x20 LCD lcd.begin(), but i think this is kinda redundant 
const int LCD_NB_COLUMNS = 20 ;
unsigned long previousMillis = 0;     // values to compare last time interval was checked (For LCD refreshing)
unsigned long currentMillis = millis(); // values to compare last time interval was checked (For LCD refreshing) and DemoTimer

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int check_State = 0;                  // state to check trigger the demo interrupt
int updateLCD_flag = 0;               // LCD updater, this flag is used to only update the screen once between mode changes
                                      // Volume Up
                                      // and once every 3 second while in a mode. Saves cycles / resources

#include <Adafruit_NeoPixel.h>  //inclusion of Adafruit's NeoPixel (RBG addressable LED) library
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN            A2 // Which pin on the Arduino is connected to the NeoPixels?
#define NUMPIXELS      13 // How many NeoPixels are attached to the Arduino? 13 total, but they are address from 0,1,2,...12.

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int colorUpdate = 0;   //setting a flag to only update colors once when the mode is switched. 
const int b = 3;       // Brightness control variable. Used to divide the RBG vales set for the RGB LEDs. full range is 0-255. 255 is super bright
                       // In fact 255 is obnoxiously bright, so this use this variable to reduce the value. It also reduces the current draw on the USB


char keys[ROWS][COLS] = {
  {'1', '2', '3', '4'},  //  the keyboard hardware is  a 3x4 grid... 
  {'5', '6', '7', '8'},
  {'9', '0', 'A', 'B'},  // these values need  to be single char, so...
};

//     |----------------------------|
//     |      [ 1] [ 2] [ 3] [ 4]   |     * Encoder A location = key[1]      
//     |      [ 5] [ 6] [ 7] [ 8]   |     * Encoder B location = Key[4]
//     |      [ 9] [ 0] [ A] [ B]   |      NOTE: The mode button is not row/column key, it's directly wired to A0!!
//     |----------------------------|

// Variables that will change:
int modePushCounter = 0;       // counter for the number of button presses
int buttonState = 0;           // current state of the button
int lastButtonState = 0;       // previous state of the button
int mouseMove;                 // variable that holds how many pixels to move the mouse cursor
long positionEncoderA  = -999; //encoderA LEFT position variable
const int ModeButton = A0;     // the pin that the Modebutton is attached to
const int pot = A1;           // pot for adjusting attract mode demoTime or mouseMouse pixel value
byte rowPins[ROWS] = {4, 5, A3 };    //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9 };  //connect to the column pinouts of the keypad
int powersave = 0;
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup() {
  Serial.begin(9600);
  //lcd.init();       //initialize the 4x20 lcd
  //lcd.backlight();  //turn on the backlight
  //lcd.noBacklight();
  //lcd.begin(LCD_NB_COLUMNS , LCD_NB_ROWS);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  randomSeed(analogRead(0));           // is this a bad idea? it's called before assigning the button as pull up... the switch is open at rest..
  pinMode(ModeButton, INPUT_PULLUP);  // initialize the button pin as a input:  
  //lcd.setCursor(0, 0);  lcd.print("Macro KB RC V2.0");
  //lcd.setCursor(0, 1);  lcd.print("(c) 2020 Ryan Bates");
  delay(800);
  //lcd.clear();

  Keyboard.begin();
  Consumer.begin();
  pixels.begin(); // This initializes the NeoPixel library.
}

void loop() {
  char key = keypad.getKey();
  //mouseMove = (analogRead(pot)); //reading the analog input, pot = pin A1
  //mouseMove = map(mouseMove, 0,1023, 1,124); //remap the analog pot values fron 1 to 124
  checkModeButton();

  
  switch (modePushCounter) {                  // switch between keyboard configurations:
    //Linux General Setup (YELLOW!)
    //This one will support Linux Media player natively and support Linux VM by binding SHIFT+
    //SHIFT +(F2/F3) = Volume Down/Up
    //SHIFT +(F7/F8/F9) = Prev/Play/Pause/Next Track
    //Ctrl+Shift+K setup for shutdown.
    case 0:                                   // Application Alpha or MODE 0. Example = Every button ends your Zoom call
      LCD_update_0();                         // Mode 0 text for LCD
      setColorsMode0();                       // indicate what mode is loaded by changing the key colors
      encoderA_Mode0();                       // Ccustom function for encoder A
  
      if (key) {
        Serial.println(key);
        switch (key) {
          case '1': //Play/Pause
             //pixels.setPixelColor(1, pixels.Color(150,0,0)); // change the color to green when pressed, wait 100ms so the change can be observed
             Consumer.write(MEDIA_PLAY_PAUSE);
             Keyboard.press(KEY_LEFT_SHIFT); 
             Keyboard.press(KEY_F8); delay(150);//???
             Keyboard.releaseAll();         
             //pixels.setPixelColor(1, pixels.Color(0,150,0)); // change the color to green when pressed, wait 100ms so the change can be observed
             break;
          case '2': //Launch SonixD
             Keyboard.press(KEY_LEFT_GUI);delay(150);
             Keyboard.release(KEY_LEFT_GUI); 
             delay(300);
             Keyboard.println("sonixd");
             break;
          case '3': //Launch Terminator
             Keyboard.press(KEY_LEFT_GUI);delay(150);
             Keyboard.release(KEY_LEFT_GUI);
             delay(300); 
             Keyboard.println("terminator");
             break;
         case '4': //Launch VSCode (Untested)
             Keyboard.press(KEY_LEFT_GUI);delay(50);
             Keyboard.release(KEY_LEFT_GUI);
             delay(300);
             Keyboard.println("vscode");
             break;
         case '5': //Launch Firefox
             Keyboard.press(KEY_LEFT_GUI);delay(50);
             Keyboard.release(KEY_LEFT_GUI);
             delay(300);
             Keyboard.println("firefox");
             break;
        
        case '7': //Volume Up
             Consumer.write(MEDIA_VOLUME_UP);
             Keyboard.press(KEY_LEFT_SHIFT); 
             Keyboard.press(KEY_F3); delay(50);//???
             Keyboard.releaseAll();
             break;
        case '9': //Linux Shutdown (set keyboard-shortcut, ctrl+shift+k)
             Keyboard.press(KEY_LEFT_CTRL);
             Keyboard.press(KEY_LEFT_ALT);
             Keyboard.print('k');
             Keyboard.releaseAll();
             break;
        case '0': 
             Consumer.write(MEDIA_PREVIOUS);
             Keyboard.press(KEY_LEFT_SHIFT); 
             Keyboard.press(KEY_F7); delay(50);//???
             Keyboard.releaseAll();
             break;
        case 'A':
             Consumer.write(MEDIA_VOLUME_DOWN);
             Keyboard.press(KEY_LEFT_SHIFT); 
             Keyboard.press(KEY_F2);delay(150); //???
             Keyboard.releaseAll();
             break;
        case 'B':
             Consumer.write(MEDIA_NEXT);
             Keyboard.press(KEY_LEFT_SHIFT); 
             Keyboard.press(KEY_F9);delay(150); //???
             Keyboard.releaseAll();
             delay(50);
             break;     
        }     
    
        delay(100); Keyboard.releaseAll(); 
        colorUpdate = 0;                                    //call the color update to change the color back to Mode settings   // this releases the buttons 
    }
    break;
      
    // TEAMS SETUP (BLUE)  
    case 1:                     //  Application Beta or MODE 1 Rocket League Quick Chat (with light reactive keys)
      encoderA_Mode2();                         //move mouse
      setColorsMode1();
      LCD_update_1();                           //Mode 1 text for LCD
      if (key) {
      //Serial.println(key);
      switch (key) {
         case '1': 
                  Keyboard.press(KEY_LEFT_CTRL);
                  Keyboard.press(KEY_LEFT_SHIFT);
                  Keyboard.print('m');
                  Keyboard.releaseAll(); //(un)Mute Microphone
                  break;
        case '2': Serial.print('empty');  
           break;
        case '3': Keyboard.press(KEY_LEFT_CTRL);
                  Keyboard.press(KEY_LEFT_SHIFT);
                  Keyboard.print('o');                          //Turn on/off video
                  Keyboard.releaseAll();

                  break;   
        case '4': Keyboard.press(KEY_LEFT_GUI);                 //Screen shot.
                  Keyboard.press(KEY_LEFT_SHIFT);
                  Keyboard.print('s');
                  Keyboard.releaseAll();
                  delay(50); 
                  //pixels.setPixelColor(4, pixels.Color(0,150,0)); 
                  break;
           
        case '5': Keyboard.press(KEY_LEFT_CTRL);
                  Keyboard.press(KEY_LEFT_SHIFT);
                  Keyboard.print('k');                          //Hands Up
                  Keyboard.releaseAll();
                  break;
           
        case '6': Keyboard.press(KEY_LEFT_GUI);
                  Keyboard.print('.');                          //Emoji Search
                  Keyboard.releaseAll();
                  break;
          
        case '7': Keyboard.press(KEY_LEFT_CTRL);
                  Keyboard.press(KEY_UP_ARROW); delay(150);
           break;
           
        case '8': Serial.println(" "); 
           break;
           
        case '9': Keyboard.press(KEY_LEFT_CTRL);
                  Keyboard.press(KEY_LEFT_SHIFT);
                  Keyboard.print('x');
                  Keyboard.releaseAll();  //Full Edit Message
                  break;
           
        case '0': Keyboard.press(KEY_LEFT_ARROW); delay(50);                
           break;
           
        case 'A': Serial.println(" "); 
           break;
           
        case 'B': Keyboard.press(KEY_RIGHT_ARROW); delay(50);
           break;  
      }
      pixels.show();                                      //update the color after the button press
      delay(100); Keyboard.releaseAll();                   // this releases the buttons
      //delay(100);                                         //delay a bit to hold the color (optional)
      colorUpdate = 0;                                    //call the color update to change the color back to Mode settings   
    }
    break;
 //====================================================================================================================     
    case 2:    // General Use RED
      encoderA_Mode2();
      LCD_update_2();  
      setColorsMode2();                                  // set color layout for this mode

      if (key) {
       switch (key) {
        case '1': 
          Keyboard.press(KEY_LEFT_CTRL);    
          Keyboard.release(KEY_LEFT_CTRL); 
        break;
          
        case '2'://Definitely not a password
          Keyboard.press(KEY_LEFT_CTRL);    
          Keyboard.release(KEY_LEFT_CTRL);
          delay(250);  
          Keyboard.println("C4######");
          break;
    
        case '3':   
          //lcd.off(); //Turn off LCD
          colorUpdate=0;
          setColorsModeOFF(); 
          break;
    
        case '4':   
          //lcd.on();; //Turn back on LCD
          colorUpdate=0;
          setColorsMode2();
          break;
        
        case '5': Keyboard.press(KEY_F5);  Keyboard.release(KEY_F5);   
          break;
    
        case '6': Keyboard.press(KEY_LEFT_CTRL);    Keyboard.press('g');
          Keyboard.release(KEY_LEFT_CTRL);  Keyboard.release('g'); delay(250);
          Keyboard.println("Ms. Pac-Man (USA, Europe).gen");            
          break;
    
        case '7': Keyboard.press(KEY_LEFT_CTRL);    Keyboard.press('g');
          Keyboard.release(KEY_LEFT_CTRL);  Keyboard.release('g'); delay(250);
          Keyboard.println("Devilish - The Next Possession (USA).gen"); 
          break;
    
        case '8': Keyboard.press(KEY_F8); Keyboard.release(KEY_F8);     
          break;
    
       }
       delay(50); Keyboard.releaseAll(); // this releases the buttons
     }
    break;
      
//    case 3:                //Auto Attract (Demo) Mode for a Raspberry Pi running RetroPie
//     //LCD_update_3();                                     //Mode 3 text for LCD. not used, The AutoAttract mode has its own LCD code
//     setColorsMode3();
//     //encoderA_Mode3();                                  //  skip encoders to get better responce from keys
//      if (key) {
//        switch (key) {
//        case '1':   Keyboard.press('4');
//                    break;
//                    
//        case '2':   Keyboard.press('4');
//                  break;                         
//        case '3': Keyboard.press('5'); break;
//        case '4': Keyboard.press('6'); break;
//        case '5': previousMillis = currentMillis;     break; // works, most of the time
//        case '6': Keyboard.press(KEY_UP_ARROW); break;
//        case '7': Keyboard.press('1'); break;
//        case '8': Keyboard.press('2'); break;
//        case '9': Keyboard.press(KEY_LEFT_ARROW);  break;
//        case '0': Keyboard.press(KEY_DOWN_ARROW);  break;
//        case 'A': Keyboard.press(KEY_RIGHT_ARROW);  break;
//        case 'B': Keyboard.press(KEY_TAB);  break;
//          
//          break;                                
//               }
//       Keyboard.releaseAll(); // this releases the buttons
//               }
//     break;
  }
  delay(1);  // delay in between reads for stability

}
//---------------------Sub Routine Section--------------------------------------------------------------------------
void setColorsMode0(){
  if (colorUpdate == 0){                                           // have the neopixels been updated?
      for(int i=0;i<NUMPIXELS;i++){      //  Red,Green,Blue                      // pixels.Color takes RGB values; range is (0,0,0) to (255,255,255)
        pixels.setPixelColor(i, pixels.Color(150,   150,    0));      // Moderately bright red color.
        pixels.show();                                             // This pushes the updated pixel color to the hardware.
        delay(50); }                                               // Delay for a period of time (in milliseconds).                                         
      colorUpdate=1;   }                                           // Mark the color flag so neopixels are no longer updated in the loop
}

void setColorsMode1(){
  if (colorUpdate == 0){                                           // have the neopixels been updated?
      for(int i=0;i<NUMPIXELS;i++){      //  Red,Green,Blue                      // pixels.Color takes RGB values; range is (0,0,0) to (255,255,255)
        pixels.setPixelColor(i, pixels.Color(0,   0,    150));      // Moderately bright red color.
        pixels.show();                                             // This pushes the updated pixel color to the hardware.
        delay(50); }                                               // Delay for a period of time (in milliseconds)                                             
      colorUpdate=1;   }                                           // Mark the color flag so neopixels are no longer updated in the loop
}

//void setColorsMode1(){
//  if (colorUpdate == 0){                                     // have the neopixels been updated?
//      pixels.setPixelColor(0,  pixels.Color( 80,  0,200));    //gradient mix
//      pixels.setPixelColor(1,  pixels.Color( 10,  0,200));    //gradient mix
//      pixels.setPixelColor(2,  pixels.Color( 20,  0,200));
//      pixels.setPixelColor(3,  pixels.Color( 40,  0,200));
//      pixels.setPixelColor(4,  pixels.Color( 60,  0,200));
//      pixels.setPixelColor(5,  pixels.Color( 80,  0,200));
//      pixels.setPixelColor(6,  pixels.Color(100,  0,200));
//      pixels.setPixelColor(7,  pixels.Color(120,  0,200));
//      pixels.setPixelColor(8,  pixels.Color(140,  0,200));
//      pixels.setPixelColor(9,  pixels.Color(160,  0,200));
//      pixels.setPixelColor(10, pixels.Color(180,  0,200));
//      pixels.setPixelColor(11, pixels.Color(200,  0,200));
//      pixels.setPixelColor(12, pixels.Color(220,  0,200));
//      pixels.show();
//      colorUpdate=1;              }                           // neoPixels have been updated. 
//                                                              // Set the flag to 1; so they are not updated until a Mode change
//}

void setColorsMode2(){
  if (colorUpdate == 0){                                           // have the neopixels been updated?
      for(int i=0;i<NUMPIXELS;i++){      //  Red,Green,Blue                      // pixels.Color takes RGB values; range is (0,0,0) to (255,255,255)
        pixels.setPixelColor(i, pixels.Color(150,   0,    0));      // Moderately bright red color.
        pixels.show();                                             // This pushes the updated pixel color to the hardware.
        delay(50); }                                               // Delay for a period of time (in milliseconds)                                             
      colorUpdate=1;   }                                           // Mark the color flag so neopixels are no longer updated in the loop
}
void setColorsMode3(){
  if (colorUpdate == 0){                                     // have the neopixels been updated?
      pixels.setPixelColor(0, pixels.Color(  0,150,150));   // cyan
      pixels.setPixelColor(1, pixels.Color(  0,150,  0));   // green
      pixels.setPixelColor(2, pixels.Color(  0,150,150));   // cyan 
      pixels.setPixelColor(3, pixels.Color(  0,150,  0));   // green
      pixels.setPixelColor(4, pixels.Color(  0,150,150));   // cyan
      pixels.setPixelColor(5, pixels.Color(  0,150,150));   // cyan
      pixels.setPixelColor(6, pixels.Color(  0,150,  0));   // green
      pixels.setPixelColor(7, pixels.Color(  0,150,150));   // cyan
      pixels.setPixelColor(8, pixels.Color(  0,150,  0));   // green
      pixels.setPixelColor(9, pixels.Color(  0,150,  0));   // green
      pixels.setPixelColor(10, pixels.Color(  0,150,150));  // cyan 
      pixels.setPixelColor(11, pixels.Color(  0,150,  0));  // green
      pixels.setPixelColor(12, pixels.Color(  0,150,150));  // cyan 
      pixels.show(); colorUpdate=1;                 }       // neoPixels have been updated. 
                                                            // Set the flag to 1; so they are not updated until a Mode change
}
void setColorsModeOFF(){
  pixels.clear();                                             // This pushes the updated pixel color to the hardware.
  if (colorUpdate == 0){                                           // have the neopixels been updated?
      for(int i=0;i<NUMPIXELS;i++){      //  Red,Green,Blue                      // pixels.Color takes RGB values; range is (0,0,0) to (255,255,255)
        pixels.setPixelColor(i, pixels.Color(1,   1,  1));      // Moderately bright red color.
        pixels.show();                                             // This pushes the updated pixel color to the hardware.
        delay(50); }// Delay for a period of time (in milliseconds)                                             
        colorUpdate=1;    }   
  }                                           // Mark the color flag so neopixels are no longer updated in the loop

void checkModeButton(){
  buttonState = digitalRead(ModeButton);
  if (buttonState != lastButtonState) { // compare the buttonState to its previous state
    if (buttonState == LOW) { // if the state has changed, increment the counter
      // if the current state is LOW then the button cycled:
      modePushCounter++;
      //Serial.println("pressed");
      //Serial.print("number of button pushes: ");
      //Serial.println(modePushCounter);
      updateLCD_flag = 0;   // forces a screen refresh 
      colorUpdate = 0;      // set the color change flag ONLY when we know the mode button has been pressed. 
                            // Saves processor resources from updating the neoPixel colors all the time
    } 
    delay(50); // Delay a little bit to avoid bouncing
  }
  lastButtonState = buttonState;  // save the current state as the last state, for next time through the loop
   if (modePushCounter >2){       //reset the counter after 4 presses CHANGE THIS FOR MORE MODES
      modePushCounter = 0;}
}

void encoderA(){
  long newPos = RotaryEncoderA.read()/4; //When the encoder lands on a valley, this is an increment of 4.
  
  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(positionEncoderA);
    Keyboard.press(KEY_LEFT_ARROW);
    Keyboard.release(KEY_LEFT_ARROW);                      
    }

  if (newPos != positionEncoderA && newPos < positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(positionEncoderA);
    Keyboard.press(KEY_RIGHT_ARROW);
    Keyboard.release(KEY_RIGHT_ARROW);                      
    }
}

//=============== encoder definitions/assignments ===========================================
//this section allows a unique encoder function for each mode (profile). Four total in this case or modes 0 through 3.

//=============Encoder A & B Function ====== Set 0 =========================================================
void encoderA_Mode0(){
  long newPos = RotaryEncoderA.read()/4; //When the encoder lands on a valley, this is an increment of 4.
                                          // your encoder might be different (divide by 2) i dunno. 
  if (newPos != positionEncoderA) {
    Serial.print("Value = ");
    Serial.print(newPos);
    if (newPos > positionEncoderA){
      Serial.print(" LEFT!!! ");
      Serial.println();
      Consumer.write(MEDIA_VOLUME_DOWN);
    }
    if (newPos < positionEncoderA){
      Serial.print(" RIGHT!!! ");
      Serial.println();
      Consumer.write(MEDIA_VOLUME_UP);
    }
    positionEncoderA = newPos;
  }
}


//=============Encoder A & B Function ====== Set 1 =========================================================
void encoderA_Mode1(){
  long newPos = RotaryEncoderA.read()/2; 
  if (newPos != positionEncoderA && newPos < positionEncoderA) {
    positionEncoderA = newPos;
     //tab increase
    Keyboard.write(65); //tab key
     }

  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    //tab decrease
    Keyboard.press(KEY_LEFT_SHIFT); 
    Keyboard.write(97); //tab key
    Keyboard.release(KEY_LEFT_SHIFT);                      }

}

//=============Encoder A & B Function ====== Set 2 =========================================================
void encoderA_Mode2(){ //testing some encoder wheel play control for arcade games; centede, tempest...

  long newPos = RotaryEncoderA.read()/2; 
  if (newPos != positionEncoderA) {
    if (newPos > positionEncoderA) {
      positionEncoderA = newPos;
      Serial.println(mouseMove);
      //Keyboard.println("Volume Up");
      Mouse.move(-mouseMove,0,0); //moves mouse right... Mouse.move(x, y, wheel) range is -128 to +127
    }

    if (newPos < positionEncoderA) {
      positionEncoderA = newPos;
      //Keyboard.println("Volume Down");
      Mouse.move(mouseMove,0,0); //moves mouse left... Mouse.move(x, y, wheel) range is -128 to +127
    }
  }                  
}

//=============Encoder A & B Function ====== Set 3 =========================================================
void encoderA_Mode3(){
  long newPos = RotaryEncoderA.read()/2; 
  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    Mouse.press(MOUSE_LEFT); //holds down the mouse left click
    Mouse.move(0,4,0); //moves mouse down... Mouse.move(x, y, wheel) range is -128 to +127
    Mouse.release(MOUSE_LEFT); //releases mouse left click
                                                               }

  if (newPos != positionEncoderA && newPos < positionEncoderA) { 
    positionEncoderA = newPos;
    Mouse.press(MOUSE_LEFT); //holds down the mouse left click
    Mouse.move(0,-4,0); //moves mouse up... Mouse.move(x, y, wheel) range is -128 to +127
    Mouse.release(MOUSE_LEFT); //releases mouse left click                   
                                                              }
}

//========================     LCD Update Routines    ===================================
//Linux Usage (Yellow)
void LCD_update_0() { 
                      //This method is less heavy on tying up the arduino cycles to update the LCD; instead 
                      //this updates the LCD every 3 seconds. If you put the LCD.write commands
                      //in the key function loops, this breaks the 'feel' and responsiveness of the keys. 
                      //This subroutine that runs infrequently helps the keypad function with decent response.
                      
  unsigned long currentMillis = millis();

  //====== SETUP THE SCREEN OPTIONS =======
  //================= a note about this cycle, this follows the example sketch "Blink without Delay"===============
  if (currentMillis - previousMillis >= 3000) {                       // if the elasped time greater than 3 seconds
    previousMillis = currentMillis;                                   // save the last time you checked the interval
    switch (updateLCD_flag) {
//    case 0:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print("1: Play / Pause / Vol");
//      lcd.setCursor(0, 1); lcd.print("2: SonixD");
//      lcd.setCursor(0, 2); lcd.print("3: Terminator");
//      lcd.setCursor(0, 3); lcd.print("4: VSCode");
//      updateLCD_flag = 1;
//      break;
//    case 1:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print("5: Firefox");
//      lcd.setCursor(0, 1); lcd.print("6: ");
//      lcd.setCursor(0, 2); lcd.print("7: Volume Up");
//      lcd.setCursor(0, 3); lcd.print("8: ");
//      updateLCD_flag = 2;
//      break;
//    case 2:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print(" 9: SHUTDOWN LINUX");
//      lcd.setCursor(0, 1); lcd.print("10: Previous Track");
//      lcd.setCursor(0, 2); lcd.print("11: Volume Down");
//      lcd.setCursor(0, 3); lcd.print("12: Next Track");
//      updateLCD_flag = 0;
//      break; 
    case 0: 
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("1 - Hello, world!");
      display.setCursor(0, 8);
      display.println("2 - Hello, world!");
      display.setCursor(0, 16);
      display.println("3 - Hello, world!");
      display.setCursor(0, 24);
      display.println("4 - Hello, world!");
      display.display();
      updateLCD_flag = 1;
      break;
    case 1: 
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("5 - Hello, world!");
      display.setCursor(0, 8);
      display.println("6 - Hello, world!");
      display.setCursor(0, 16);
      display.println("7 - Hello, world!");
      display.setCursor(0, 24);
      display.println("8 - Hello, world!");
      display.display();
      updateLCD_flag = 2;
      break;
    case 2: 
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("9 - Hello, world!");
      display.setCursor(0, 8);
      display.println("10 - Hello, world!");
      display.setCursor(0, 16);
      display.println("11 - Hello, world!");
      display.setCursor(0, 24);
      display.println("12 - Hello, world!");
      display.display();
      updateLCD_flag = 0;
      break;
      }
  }
}

//Teams - Blue    
void LCD_update_1() {
unsigned long currentMillis = millis();
if (currentMillis - previousMillis >= 3000) {                         // if the elasped time greater than 3 seconds
    previousMillis = currentMillis;                                   // save the last time you checked the interval
    switch (updateLCD_flag) {
        case 0:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print("1 Vol / Mute");
//      lcd.setCursor(0, 1); lcd.print("2:");
//      lcd.setCursor(0, 2); lcd.print("3: Turn Off Video");
//      lcd.setCursor(0, 3); lcd.print("4: Screenshot");
      updateLCD_flag = 1;
      break;
    case 1:    
//    lcd.clear();
//      lcd.setCursor(0, 0); lcd.print("5: Hand Up");
//      lcd.setCursor(0, 1); lcd.print("6: Emoji");
//      lcd.setCursor(0, 2); lcd.print("7: Edit Last Message");
//      lcd.setCursor(0, 3); lcd.print("8: Call");
      updateLCD_flag = 2;
      break;
    case 2:    
//    lcd.clear();
//      lcd.setCursor(0, 0); lcd.print(" 9: Full Edit");
//      lcd.setCursor(0, 1); lcd.print("10: Left Arrow");
//      lcd.setCursor(0, 2); lcd.print("11: Set to Busy");
//      lcd.setCursor(0, 3); lcd.print("12: Right Arrow");
      updateLCD_flag = 0;
      break; 
      }}}

//Windows S/C (RED)
void LCD_update_2() { 
unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 3000) {                       //if the elasped time greater than 3 seconds
    previousMillis = currentMillis;                                   // save the last time you checked the interval
     switch (updateLCD_flag) {
    case 0:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print("1: New & Improved!");
//      lcd.setCursor(0, 1); lcd.print("2: Arduino ProMicro");
//      lcd.setCursor(0, 2); lcd.print("3: Macro Keyboard");
//      lcd.setCursor(0, 3); lcd.print("4: --Version 2.0--");
      updateLCD_flag = 1;
      break;
    case 1:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print("5: Undo");
//      lcd.setCursor(0, 1); lcd.print("6: Redo");
//      lcd.setCursor(0, 2); lcd.print("7: Find Previous");
//      lcd.setCursor(0, 3); lcd.print("8: Find Next");
      updateLCD_flag = 2;
      break;
    case 2:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print(" 9: Copy");
//      lcd.setCursor(0, 1); lcd.print("10: Paste");
//      lcd.setCursor(0, 2); lcd.print("11: Comment/ UnComm");
//      lcd.setCursor(0, 3); lcd.print("12: Find");
      updateLCD_flag = 0;
      break; 
      }}}

void LCD_update_3() { 
unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 3000) {                       //if the elasped time greater than 3 seconds
    previousMillis = currentMillis; 
     switch (updateLCD_flag) {
    case 0:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print("1: MS Paint & Mouse L/R");
//      lcd.setCursor(0, 1); lcd.print("2: window snap <-");
//      lcd.setCursor(0, 2); lcd.print("3: window snap ->");
//      lcd.setCursor(0, 3); lcd.print("4: Alt+F4");
      updateLCD_flag = 1;
      break;
    case 1:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print("5: Calculator");
//      lcd.setCursor(0, 1); lcd.print("6: Excel");
//      lcd.setCursor(0, 2); lcd.print("7: Word");
//      lcd.setCursor(0, 3); lcd.print("8: Random Wiki");
      updateLCD_flag = 2;
      break;
    case 2:    
//      lcd.clear();
//      lcd.setCursor(0, 0); lcd.print(" 9: lolz");
//      lcd.setCursor(0, 1); lcd.print("10: Ryan's Youtube");
//      lcd.setCursor(0, 2); lcd.print("11: Minimize all");
//      lcd.setCursor(0, 3); lcd.print("12: Snip-it");
      updateLCD_flag = 0;
      break; 
      }}}
