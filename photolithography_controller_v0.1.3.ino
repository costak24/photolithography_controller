
#include <I2CIO.h>
#include <LCD.h>
#include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>

#include <Wire.h>

#define I2C_ADDR    0x27                                                                                                           // Defines I2C Address where the PCF8574A is
                                                                                                                                   // Address can be changed by soldering A0, A1, or A2
                                                                                                                                   // Default is 0x27

                                                                                                                                   // maps the pin configuration of LCD backpack for the LiquidCristal class
#define BACKLIGHT_PIN 3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

LiquidCrystal_I2C lcd(I2C_ADDR,
                      En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin,
                      BACKLIGHT_PIN, POSITIVE);
                         
float pNow1;                                                                                                                        //main diode current dose
float pNow2;                                                                                                                        //secondary (calibrating) diode current dose
float pRatio = 1.0;                                                                                                                 //calibration ratio between diodes, 1:1 until calibrated

                                                                                                                                    //boot diagnostics block
int startTest = 0;
int calTest = 0;
int doseSet = 0;
int sysStat = 0;
                                                                                                                                    //loads some system defaults
int dispTime = 0;
int defaultHeight = 27.0;

long previousMicros = 0;
                                                                                                                                    //firmware version info
String firmVersion = String("  Firmware V0.1.2U  ");
String firmDate = String("     08/20/2016     ");
String firmAuthor = String("    by Ken Costa    ");
                                                                                                                                    //initializes system UI buttons
const int enterButton = 3;
const int cancelButton = 5;
const int leftArrow = 6;
const int rightArrow = 8;
const int upArrow = 7;
const int downArrow = 9;
                                                                                                                                    //initializes dose control variables
long userInput1 = 0;
long userInput2 = 0;
long userInput3 = 0;
long userInput4 = 0;
long userInput5 = 0;
long userInput;
int arrowCursor;
                                                                                                                                    //initializes variables to store data from diodes
int analogSensor1;
int analogSensor2;
                                                                                                                                    //initializes global algorithm variables
float voltIn1;                      
float ampIn1;                               
float pWatts1;        
float pNext1;      

float voltIn2;    
float ampIn2;              
float pWatts2;                                   
float pNext2;  


void setup(){

lcd.begin(20,4);                                                                                                                    // 20 columns by 4 rows on display
lcd.setBacklight(HIGH);                                                                                                             // Turn on backlight, LOW for off
lcd.setCursor ( 0, 0 );                                                                                                             // go to the top left corner

pinMode(4, OUTPUT);                                   
digitalWrite(4, LOW);                                                                                                               //output to lamp
pinMode(2, OUTPUT);
digitalWrite(2, LOW);                                                                                                               //calibration algorithm diagnostic pin
pinMode(A0, INPUT);                                                                                                                 //photodiode 1, main diode
pinMode(A2, INPUT);                                                                                                                 //photodiode 2, calibrated reference
pinMode(3, INPUT_PULLUP);                                                                                                           //enter button
pinMode(5, INPUT_PULLUP);                                                                                                           //cancel button
pinMode(6, INPUT_PULLUP);                                                                                                           //left arrow
pinMode(7, INPUT_PULLUP);                                                                                                           //up arrow
pinMode(8, INPUT_PULLUP);                                                                                                           //right arrow
pinMode(9, INPUT_PULLUP);                                                                                                           //down arrow
pNow1 = 0.0;
pNow2 = 0.0;
}

void firmData()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(firmVersion);
  lcd.setCursor(0,1);
  lcd.print(firmDate);
  lcd.setCursor(0,2);
  lcd.print(firmAuthor);
  lcd.setCursor(0,3);
  lcd.print("  ent to continue   ");
  while(digitalRead(enterButton) == HIGH)
  {
    delayMicroseconds(1000);
  }
}
void sysMenu()                                                                                                                        //where all settings can be changed
{
 lcd.clear();
 lcd.setCursor(0,0);
 lcd.print("Run tests = left    ");
 lcd.setCursor(0,1);
 lcd.print("calibrate = right   ");
 lcd.setCursor(0,2);
 lcd.print("Set dose = up       ");
 lcd.setCursor(0,3);
 lcd.print("System info = down  ");
 while(digitalRead(3) && digitalRead(5) && digitalRead(6) && digitalRead(7) &&  digitalRead(8) && digitalRead(9) == HIGH)
 {
  delayMicroseconds(1000);
 }
}

void starttest()                                                                                                                      //test to verify the integrity of the lamp/system
{
  startTest = 0;                                                                                                                      //resets variable value
  lcd.setCursor ( 0, 1 );
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("   Run lamp test?   ");
  while(digitalRead(enterButton) == HIGH && digitalRead(cancelButton) == HIGH)                                                        //digitalRead 3 and 5 are enter and cancel respectively
  {
   delayMicroseconds(1000);
  }
  if(digitalRead(enterButton) == LOW && digitalRead(cancelButton) == HIGH)                                                            //enter pressed
   {
    delay(500);
     lcd.setCursor(0,2);
     lcd.print("   confirm test?    ");
     lcd.setCursor(0,3);
     lcd.print("  enter or cancel    ");
      while(digitalRead(enterButton) == HIGH && digitalRead(cancelButton) == HIGH)
      {
        delayMicroseconds(1000);
      }
      if(digitalRead(enterButton) == LOW && digitalRead(cancelButton) == HIGH){                                                       //enter pressed
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("default height:");                                                                                                   //check height for lamp test
      lcd.setCursor(16,0);
      lcd.print(defaultHeight);
      lcd.setCursor(0,1);
      lcd.print("    up to adjust    ");
      lcd.setCursor(0,3);
      lcd.print("ent to continue... ");
      delay(1000);
      while(digitalRead(enterButton) && digitalRead(upArrow) == HIGH){
        delayMicroseconds(1000);
      }
      if(digitalRead(upArrow) == LOW) {                                                                                              //up arrow pressed, adjust default height
        delay(1000);
        int lastHeight = defaultHeight;
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("  lamp height (cm)  ");
          lcd.setCursor(10,1);
          lcd.print(defaultHeight);
        while(digitalRead(enterButton) && digitalRead(cancelButton) == HIGH){

          if(digitalRead(upArrow) == LOW && digitalRead(downArrow) == HIGH && defaultHeight < 100){                                  //adds height, up arrow pressed
            defaultHeight = defaultHeight + 1;
            lcd.setCursor(10,1);
            lcd.print(defaultHeight);
            delay(500);
          }
          else if(digitalRead(upArrow) == HIGH && digitalRead(downArrow) == LOW && defaultHeight > 0){                //subtracts height, down arrow pressed
            defaultHeight = defaultHeight - 1;
            lcd.setCursor(10,1);
            lcd.print(defaultHeight);
            delay(500);
          }
        }
        if(digitalRead(enterButton) == HIGH && digitalRead(cancelButton) == LOW){                                            //cancels adjustment, proceeds with test, cancel button pressed
          defaultHeight = lastHeight;
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("     Cancelled!     ");
          lcd.setCursor(0,2);
          lcd.print(" EYE PROTECTION ON  ");
          lcd.setCursor(0,3);
          lcd.print("BEFORE PROCEEDING! ");
          delay(1000);
          while(digitalRead(enterButton) == HIGH){
            delayMicroseconds(1000);
        }

        }
        else if(digitalRead(enterButton) == LOW && digitalRead(cancelButton) == HIGH){                                        //enter pressed, height saved
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("   New Height set   ");
          lcd.setCursor(0,2);
          lcd.print(" EYE PROTECTION ON  ");
          lcd.setCursor(0,3);
          lcd.print("BEFORE PROCEEDING! ");
          delay(1000);
          while(digitalRead(enterButton) == HIGH){
            delayMicroseconds(1000);
        }
        }
      }
        if(digitalRead(enterButton) == LOW && digitalRead(cancelButton) == HIGH)                                               //enter pressed, test begins
       {
         digitalWrite(4, HIGH);                                                                                                //lamp on

         lcd.clear();
         lcd.setCursor(0,0);
         lcd.print("diagnostics running ");
         delay(10000);                                                                                                         //diagnostics to be written here, continuity, intensity
         digitalWrite(4, LOW);
         lcd.clear();
         lcd.setCursor(0,0);
         startTest = startTest + 1;                                                                                            //adds additional one. 0 = not prompted 1 = skipped by user 2 = run
         lcd.print("diagnostics complete");
           // if
         lcd.setCursor(0,3);
         lcd.print("ent to continue...");
         while(digitalRead(enterButton) == HIGH){                                                                              //waits for enter, displays diagnostic info
          delayMicroseconds(1000);
         }
       }
  }
  else if(digitalRead(enterButton) == HIGH && digitalRead(cancelButton) == LOW)                                                //cancel button pressed, cancels test
  {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("     Cancelled!     ");
          delay(1000);
  }
   }
}

void setDose() {                                                                                                              //initializes dose control function
  doseSet = 0;
  arrowCursor = 0;
  lcd.setCursor(0,2);
  lcd.print("   set dose now?    ");
  while(digitalRead(enterButton) == HIGH && digitalRead(cancelButton) == HIGH) { 
    delayMicroseconds(1000);
  }
  if(digitalRead(cancelButton) == LOW) {                                                                                      //cancel button pushed, exits function
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("     Cancelled!     ");
       delay(2000);
}
   else if(digitalRead(enterButton) == LOW && digitalRead(cancelButton)== HIGH) {                                            //enter button pressed
    delay(1000);
    lcd.clear();                                                                                                             //initializes dose GUI interface
    lcd.setCursor(0,0);                                                                                                      //each variable is displayed in order of significance
    lcd.print("   Dose control:   ");                                                                                        //will be calculated to appropriate degree and added
    lcd.setCursor(7,1);
    lcd.print(userInput1);
    lcd.setCursor(8,1);
    lcd.print(userInput2);
    lcd.setCursor(9,1);
    lcd.print(".");
    lcd.setCursor(10,1);
    lcd.print(userInput3);
    lcd.setCursor(11,1);
    lcd.print(userInput4);
    lcd.setCursor(12,1);
    lcd.print(userInput5);
    lcd.setCursor(0,3);
    lcd.print("   enter to save   ");
    lcd.setCursor(7,1);
    arrowCursor = 0;
  while(digitalRead(enterButton) == HIGH && digitalRead(cancelButton) == HIGH) {
    lcd.blink();                                                                                                            //turns on cursor
  if(digitalRead(leftArrow) == LOW) {                                                                                       //left arrow press
       arrowCursor = arrowCursor -1;
        if(arrowCursor == -1){
          arrowCursor = 4;
        }
          delay(250);
    }
   else if(digitalRead(rightArrow) == LOW) {                                                                               //right arrow press
       arrowCursor = arrowCursor +1;
        if(arrowCursor == 5){
          arrowCursor = 0;
        }
          delay(250);
    }
   else if(digitalRead(upArrow) == LOW) {                                                                        //up arrow press
       if(arrowCursor == 0) {                                                                              //set up for use in all 5 decimal places
          userInput1 = userInput1 +1;
            if(userInput1 == 10) {
            userInput1 = 0;
       }
            lcd.setCursor(7,1);
            lcd.print(userInput1);
            lcd.setCursor(7,1);
            delay(250);
    }
       if(arrowCursor == 1) {
          userInput2 = userInput2 +1;
            if(userInput2 == 10) {
            userInput2 = 0;
       }
            lcd.setCursor(8,1);
            lcd.print(userInput2);
            lcd.setCursor(8,1);
            delay(250);
    }
       if(arrowCursor == 2) {
          userInput3 = userInput3 +1;
            if(userInput3 == 10) {
            userInput3 = 0;
       }
            lcd.setCursor(10,1);
            lcd.print(userInput3);
            lcd.setCursor(10,1);
            delay(250);
    }
       if(arrowCursor == 3) {
          userInput4 = userInput4 +1;
            if(userInput4 == 10) {
            userInput4 = 0;
       }
            lcd.setCursor(11,1);
            lcd.print(userInput4);
            lcd.setCursor(11,1);
            delay(250);
    }
       if(arrowCursor == 4) {
          userInput5 = userInput5 +1;
            if(userInput5 == 10) {
            userInput5 = 0;
       }
            lcd.setCursor(12,1);
            lcd.print(userInput5);
            lcd.setCursor(12,1);
            delay(250);
    }
}
   else if(digitalRead(downArrow) == LOW) {                                                                          //down arrow pressed
       if(arrowCursor == 0) {                                                                                //set up for use in all 5 decimal places
          userInput1 = userInput1 -1;
            if(userInput1 == -1) {
            userInput1 = 9;
       }
            lcd.setCursor(7,1);
            lcd.print(userInput1);
            lcd.setCursor(7,1);
            delay(250);
    }
       if(arrowCursor == 1) {
          userInput2 = userInput2 -1;
            if(userInput2 == -1) {
            userInput2 = 9;
       }
            lcd.setCursor(8,1);
            lcd.print(userInput2);
            lcd.setCursor(8,1);
            delay(250);
    }
       if(arrowCursor == 2) {
          userInput3 = userInput3 -1;
            if(userInput3 == -1) {
            userInput3 = 9;
       }
            lcd.setCursor(10,1);
            lcd.print(userInput3);
            lcd.setCursor(10,1);
            delay(250);
    }
       if(arrowCursor == 3) {
          userInput4 = userInput4 -1;
            if(userInput4 == -1) {
            userInput4 = 9;
       }
            lcd.setCursor(11,1);
            lcd.print(userInput4);
            lcd.setCursor(11,1);
            delay(250);
    }
       if(arrowCursor == 4) {
          userInput5 = userInput5 -1;
            if(userInput5 == -1) {
            userInput5 = 9;
       }
            lcd.setCursor(12,1);
            lcd.print(userInput5);
            lcd.setCursor(12,1);
            delay(250);
    }
}
}
  if(digitalRead(cancelButton) == LOW) {                                                                                     //cancel button pressed 
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.noBlink();
       lcd.print("     Cancelled!     ");
       delay(2000);
}
   else if(digitalRead(enterButton) == LOW) {                                                                               //enter button pressed, dose saved 
       userInput1 = userInput1 * 10000;                                                                           //converting to appropriate decimal place
       userInput2 = userInput2 * 1000;
       userInput3 = userInput3 * 100;
       userInput4 = userInput4 * 10;
       userInput5 = userInput5 * 1;
       userInput = userInput1 + userInput2 + userInput3 + userInput4 + userInput5;                                //adding for one data value, userInput
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.noBlink();
       lcd.print("   New dose set!  ");
       lcd.setCursor(7,1);
       lcd.print(userInput);
       delay(2000);
       doseSet =+ 1;                                                                                              //dose test passed
}
}
}

void sysStatus() {                                                                                                //system status function
    if(calTest == 0) {
    lcd.setCursor(7,0);
    lcd.print(" err_cal     ");                                                                                   //checks if system has been calibrated
}
    else if(calTest == 1 && startTest == 0){
      lcd.setCursor(7,0);
      lcd.print(" err_strttst ");                                                                                 //checks if lamp has been tested
    }
    else if (calTest == 1 && startTest == 1 && doseSet == 0){
      lcd.setCursor(7,0);
      lcd.print(" err_doseset ");                                                                                 //checks if dose is set
    }
    else{
      sysStat = 1;
      lcd.setCursor(7,0);
      lcd.print(" Ready");
    }
}

void loop() {                                                                                                     //starttest only runs after system reset
if(startTest == 0)
{
  lcd.setCursor ( 0, 0 );
  lcd.write("  Firmware V0.1.2U  ");
  lcd.setCursor ( 0, 1 );
  lcd.write(" System Starting Up ");
 
  delay(5000);
  starttest();
  startTest = startTest + 1;                                                                                      //starttest has been requested by system, user can opt out
  delay(1000);
}

analogSensor1 = analogRead(A0);
analogSensor2 = analogRead(A2);
                             
voltIn1 = analogSensor1 * (5/1023.0);                                                                             //defines pNext with power and resolution ---- diode 1

voltIn1 = voltIn1 / 4.9365;

ampIn1 = voltIn1 / 10000.0;                                                                                       //uses ohm's law to determine current
pWatts1 = ampIn1 / .055;                                                                                          //uses power-current relationship
pNext1 = pWatts1 / .1706;                                                                                         //devided by the area of the photodiode

voltIn2 = analogSensor2 * (5/1023.0);                                                                             //defines pNext with power and resolution ---- diode 2

voltIn2 = voltIn2 / 4.986;

ampIn2 = voltIn2 / 10000.0;                                                                                       //uses ohm's law to determine current
pWatts2 = ampIn2 / .055;                                                                                          //uses power-current relationship
pNext2 = pWatts2 / .1706;                                                                                         //devided by the area of the photodiode

dispTime = 0;
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Status:");
sysStatus();
if(calTest == 0){                                                                                              //while system is uncalibrated user may not proceed
caltest();
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Status:");
sysStatus();
}
if(doseSet == 0){                                                                                              //while dose is not set user cannot dose
setDose();
}
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Status:");
sysStatus();
lcd.setCursor(0,1);
lcd.print("up for main menu   ");
lcd.setCursor(0,2);
lcd.print("ent to begin dosing ");

while(digitalRead(enterButton) == HIGH)                                                                                     //wait for enter
{
  delayMicroseconds(1000);
}
  lcd.clear();
  lcd.print("Dosing:");
  lcd.setCursor(0,1);
  pNow1 = 0;
  dispTime = 0;

   digitalWrite(4, HIGH);                                                                                         //lamp on
   digitalWrite(2, HIGH);
   
  while(pNow1 < userInput && pNow2 < userInput)                                                                                        //lamp will only dose while the energy is less than the user preset
  {

      unsigned long currentMicros = micros();                                                                     //micros call loop, calls function every 1000 microseconds
      if(currentMicros - previousMicros > 1000)
      {
        
      analogSensor1 = analogRead(A0);
      analogSensor2 = analogRead(A2);

    if(pNow1 < userInput){

      voltIn1 = analogSensor1 * pRatio *(5/1023.0);                                                                         //defines pNext with power and resolution    code is added in while loop to allow updating
     
      voltIn1 = voltIn1 / 4.9365;
      
      ampIn1 = voltIn1 / 10000.0;                                                                                 //uses ohm's law to determine current
      pWatts1 = ampIn1 / .055;                                                                                    //uses power-current relationship
      pNext1 = pWatts1 / .1706;                                                                                   //devided by the area of the photodiode                                                                                 //pRatio is taken from the calibration step

      pNow1 = pNow1 + pNext1;                                                                                     //dose is refreshed
    }
    
      dispTime = dispTime + 1;                                                                                    //display time is updated
    if(pNow2 < userInput){
      
      voltIn2 = analogSensor2 * (5/1023.0);                                                                         //defines pNext with power and resolution    code is added in while loop to allow updating

      voltIn2 = voltIn2 / 4.986;
      
      ampIn2 = voltIn2 / 10000.0;                                                                                 //uses ohm's law to determine current
      pWatts2 = ampIn2 / .055;                                                                                    //uses power-current relationship
      pNext2 = pWatts2 / .1706;                                                                                   //devided by the area of the photodiode                                                                                 //pRatio is taken from the calibration step

      pNow2 = pNow2 + pNext2;                                                                                     //dose is refreshed
    }
    if(pNow1 >= userInput){
      digitalWrite(4, LOW);
    }

    if(pNow2 >= userInput){
      digitalWrite(2, LOW);
    }

        if(dispTime == 100)                                                                                             //display time is only updated every x cycles to avoid overloading display
        {
         lcd.setCursor(8, 0);
         lcd.print(pNow1);
         lcd.setCursor(0, 3);
         lcd.print(pNow2);
         
         dispTime = 0; 
        }
      previousMicros = currentMicros;
      }

  }
  
digitalWrite(2, LOW);
digitalWrite(4, LOW);                                                                                             //lamp off

  lcd.setCursor(0,2);
  lcd.print("  Dose  Complete");
  while(digitalRead(enterButton) == HIGH)
  {
    delayMicroseconds(1000);
  }
pNow1 = 0.0;
delay(1000);
}

void caltest() {                                                                                                  //calibration function
  if(calTest == 0) {

    lcd.setCursor(0,2);
    lcd.print("   Calibrate now?   ");
    while(digitalRead(enterButton) && digitalRead(cancelButton) == HIGH) {
      delayMicroseconds(1000);
    }
    if(digitalRead(enterButton) == LOW && digitalRead(cancelButton) == HIGH) {
      lcd.setCursor(0,2);
      lcd.print("   confirm test?    ");
      delay(1000);
      while(digitalRead(enterButton) && digitalRead(cancelButton) == HIGH) {
      delayMicroseconds(1000);
    }

      if(digitalRead(enterButton) == LOW && digitalRead(cancelButton) == HIGH){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("   Move diode in    ");
        lcd.setCursor(0,1);
        lcd.print("   ent when done    ");
        lcd.setCursor(0,2);
        lcd.print(" EYE PROTECTION ON  ");
        lcd.setCursor(0,3);
        lcd.print(" BEFORE PROCEEDING! ");
        delay(1000);
        while(digitalRead(enterButton) == HIGH){
          delayMicroseconds(1000);
        }
        
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("   calibrating...   ");

      float avVoltIn1 = 0;
      float avVoltIn2 = 0;
      float avDenominator = 0;
      
      digitalWrite(4, HIGH);
    for(unsigned long samples = 0; samples <= 2500000; samples += 1){
      unsigned long currentMicros = micros();
      if(currentMicros - previousMicros > 1000)
      {
        
      analogSensor1 = analogRead(A0);
      analogSensor2 = analogRead(A2);

      voltIn1 = analogSensor1*(5/1023.0);                         

      voltIn1 = voltIn1 / 4.9365;
      
      avVoltIn1 = avVoltIn1 + voltIn1;

      voltIn2 = analogSensor2*(5/1023.0);                         

      voltIn2 = voltIn2 / 4.986;
      
      avVoltIn2 = avVoltIn2 + voltIn2;
 
      dispTime = dispTime + 1;
      avDenominator = avDenominator + 1;

      if(dispTime == 100)
      {
        lcd.setCursor(0,1);
        lcd.print("Reference:");
        lcd.setCursor(10,1);
        lcd.print(voltIn2);
        lcd.setCursor(0,2);
        lcd.print("Main/user:");
        lcd.setCursor(10,2);
        lcd.print(voltIn1);
        lcd.setCursor(0,3);
        lcd.print(samples);
        dispTime = 0;
      }

      previousMicros = currentMicros;
      } 
  }
  digitalWrite(4, LOW);
  calTest = calTest + 1;

  avVoltIn1 = avVoltIn1 / avDenominator;
  avVoltIn2 = avVoltIn2 / avDenominator;
  pRatio = avVoltIn2 / avVoltIn1;

  delay(5000);
  lcd.clear();
  lcd.print("    calibration     ");
  lcd.setCursor(0,1);
  lcd.print("      complete      ");
  lcd.setCursor(0,2);
  lcd.print("Ratio:");
  lcd.setCursor(6,2);
  lcd.print(pRatio);
  previousMicros = 0;
  while(digitalRead(enterButton) == HIGH){
    delayMicroseconds(1000);
  }

  lcd.clear();
  lcd.print("Status:");
      }
       if(digitalRead(cancelButton) == LOW && digitalRead(enterButton) == HIGH) {
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("     Cancelled!     ");
       delay(2000);
}
}
       if(digitalRead(cancelButton) == LOW && digitalRead(enterButton) == HIGH) {
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("     Cancelled!     ");
       delay(2000);
}
    }
  }
