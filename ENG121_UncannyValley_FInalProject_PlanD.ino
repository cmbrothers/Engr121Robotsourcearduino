//Coding for color sensor adapted from source/ example code provided by Adafruit
//coding for driving LCD with 595 shift register adapted from source/ example code by Rowan Simms

#include <Wire.h>                // Arduino I2C library
#include "Adafruit_TCS34725.h"   // adafruit color sensor library
#include <Servo.h>               //arduino servo library
#include <LiquidCrystal595.h>    // include the library

LiquidCrystal595 lcd(6,7,8);     // datapin, latchpin, clockpin

Servo LEFT;
Servo RIGHT;
Servo DROP;

//Ping sensor array assignments 0 - RIght 1 - Front 2 - Left
int ping[3] = {3,4,5};
int echo[3] = {0,9,2};
unsigned long time[3]; 
unsigned long range[3];
int rangeselect[5] = {20,50,60,70,90};

//Array containers for color range values.
int redmin[2]; 
int redmax[2];
int grnmin[2];  
int grnmax[2];
int blumin[2];  
int blumax[2];

//button pin and software debounce variables.
int button = 13;
int buttonState[2] = {HIGH,LOW};
long debounce[2] = {0,50};

//misc variables, trigger calibrated condition and hold if room has been found. #of meds left in tank.
int calibrated = 0;
int medsleft = 0;
int roomfound;
int select;

//RGB values for global access
float r, g, b;

//gamma table of RGB values
byte gammatable[255];

//Initialize sensor, No gain and 24ms integration time
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_1X);

void setup() {
  Serial.begin(9600);
  Serial.println("Color View Test!");
  
  lcd.begin(16,2);             // 16 characters, 2 rows
  lcd.clear();
  
  //attach servos to pins
  LEFT.attach(12);
  RIGHT.attach(11);
  DROP.attach(10);

//test if color sensor is found
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }  
  
  //pin assignments for ping and echo of range finders.
  for(int i = 0; i <= 2; i++){
    pinMode(echo[i],INPUT);
    pinMode(ping[i],OUTPUT);
  }
  
  for (int i=1; i<256; i++) { //Build Gamma table
    float x = i;
    x /= 256;
    x = pow(x, 2.5);
    x *= 256;   
    gammatable[i-1] = x;
  }
  pinMode(button,INPUT);
  calsensorservo();  
}

void loop() { //basic navigation
  
  sonar();
  switch(medsleft){
    case 0:
      lcd.setCursor(1,0);
      lcd.print("Meds Exhausted");
      STOP();
      break;
  }
  if(range[1] > rangeselect[select]){ //check forward range against selected range: 20,50,60,70 cm
    FWD();
  } 
  else{
    STOP();
    if(range[0] < range[2]){ //if right hand range is less than left turn left 90 degrees
      LNINETY();
      if(medsleft == 1){
        select = 3;
      }
    }
    if(range[0] > range[2]){ //if left hand range is less than right turn right 90 degrees
      RNINETY();
      if(medsleft == 1){
        select = 3;
      }
    }
  }
  if(colorsensor() == 1){ //chekc if color sensor has detected room
    lcd.setCursor(3,1);
    lcd.print("Room Found");
    room();
  }
}

void drop(){
  switch(medsleft){
    case 1:
      DROP.write(0);
      break;
    case 2:
      DROP.write(45);
      break;
    case 3:
      DROP.write(90);
      break;  
    case 4:
      DROP.write(135);
      break;
  }
}

void room(){ //room found execution of medicine drop.
    delay(2000);
    STOP();
    delay(500);
    REV();
    delay(500);
    sonar();  //once meds have been dropped execute range check and turn 180 degrees
    if(range[0]>range[2]){
      RTURNABOUT();
    }
    else if(range[0]<range[1]){
      LTURNABOUT();
    }
    lcd.setCursor(1,1);
    lcd.print("Dropping Meds");
    delay(1000);
    //drop();
    medsleft--;
    lcd.setCursor(1,1);
    lcd.print("   Leaving   ");
    while(colorsensor() != 1){  //until room threshold is detected continue forward.
      UFWD();    
    }
    lcd.setCursor(3,1);
    lcd.print("Room Left ");
    delay(250);
    lcd.setCursor(3,1);
    lcd.print("Searching"); 
}

void LTURNABOUT(){ //180 degree turn to the left
  LEFT.write(80);
  RIGHT.write(80);
  delay(1200);
  STOP();
}

void RTURNABOUT(){  //180 degree turn to the right
  LEFT.write(100);
  RIGHT.write(100);
  delay(1300);
  STOP();
}

void RNINETY(){  //90 degree right turn
  LEFT.write(100);
  RIGHT.write(100);
  delay(600);
  STOP();
}

void LNINETY(){  //90 degree left turn
  LEFT.write(80);
  RIGHT.write(80);
  delay(600);
  STOP();
}

/* UNUSED CORRECTION FUNCTIONS
void RCORRECT(){
  LEFT.write(100);
  RIGHT.write(100);
}

void LCORRECT(){
  LEFT.write(80);
  RIGHT.write(80);
}

*/

void UFWD(){ //Uncorrected forward movement
  LEFT.write(95);
  RIGHT.write(85);
}

void FWD(){
  LEFT.write(95);
  RIGHT.write(85);
  /*  INTENDED TO CORRECT FORWARD ORIENTATION BY RANGFINDING TO LEFT AND RIGHT
  if(range[0]>20 && range[2]>20){
    return;
  }
  else if(range[0]>range[2]){
    RCORRECT();
  }
  else if(range[0]<range[2]){
    LCORRECT();
  }
  */
}

void REV(){ //Reverse movement
  LEFT.write(80);
  RIGHT.write(100);
}

void STOP(){  //Stop movement
  LEFT.write(90);
  RIGHT.write(90);
}

void calsensorservo(){ //Measure RGB values, compute and store min/ max values for target acquisition, allow for servo tuning, load meds and increment medsleft counter

  String calstep;
  
  int i = 0;
  int rednom[2]; 
  int grnnom[2];
  int blunom[2];

  lcd.print("Servo Cal");

  while(i==0){
    LEFT.write(90);
    RIGHT.write(90);
    DROP.write(0);
    
    //software debounce for button presses ends servo cal
    int reading = digitalRead(button);
    if (reading != buttonState[1]) {
      debounce[0] = millis();
    } 
    if ((millis() - debounce[0]) > debounce[1]) {
      if (reading != buttonState[0]) {
        buttonState[0] = reading;
        if (buttonState[0] == HIGH) {
          i++;
        }
      }
    }
    buttonState[1] = reading;
  }
  
  lcd.clear();
  
  while(i <= 2 && i!=0){ 
    switch(i){
      case 1:
        calstep = String("FLOOR");
        break;
      case 2:
        calstep = String("THRESH");
        break;
    }
    
    colorsensor(); //poll color sensor

    lcd.setCursor(0,0);
    lcd.print("C.SnsCal: ");
    lcd.print(calstep);
    lcd.setCursor(0,1);
    lcd.print("R");
    if((int)r < 100){
      lcd.print(" ");
    }
    lcd.print((int)r);
    lcd.setCursor(5,1);
    lcd.print("G");
    if((int)g < 100){
      lcd.print(" ");
    }
    lcd.print((int)g);
    lcd.setCursor(10,1);
    lcd.print("B");
    if((int)b < 100){
      lcd.print(" ");
    }
    lcd.print((int)b);
     
     
    //software debounce for button, take readings of RGB values, compute min/max range and store in global array
    int reading = digitalRead(button);
    if (reading != buttonState[1]) {
      debounce[0] = millis();
    } 
    if ((millis() - debounce[0]) > debounce[1]) {
      if (reading != buttonState[0]) {
        buttonState[0] = reading;
        if (buttonState[0] == HIGH) {
          rednom[i-1] = (int)r;
          redmin[i-1] = rednom[i-1] - 5;
          redmax[i-1] = rednom[i-1] + 5;
          grnnom[i-1] = (int)g;
          grnmin[i-1] = grnnom[i-1] - 5;
          grnmax[i-1] = grnnom[i-1] + 5;
          blunom[i-1] = (int)b;
          blumin[i-1] = blunom[i-1] - 5;
          blumax[i-1] = blunom[i-1] + 5;
          i++;
        }
      }
    }
    buttonState[1] = reading; 
  }
    
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Load Medicine:");
  delay(1000);
  while(medsleft<=3){
    switch(medsleft){
      case 0:
        lcd.setCursor(7,1);
        lcd.print("01");
       //DROP.write(180);
        break;
      case 1:
        lcd.setCursor(7,1);
        lcd.print("02");
       // DROP.write(135);
        break;
      case 2:
        lcd.setCursor(7,1);
        lcd.print("03");
       // DROP.write(90);
        break;
      case 3:
        lcd.setCursor(7,1);
        lcd.print("04");
      //  DROP.write(45);
        break;
    }
    
    //software debounce for button presses, increments medsleft and changes loader position based on above switch-case
    int reading = digitalRead(button);
    if (reading != buttonState[1]) {
      debounce[0] = millis();
    } 
    if ((millis() - debounce[0]) > debounce[1]) {
      if (reading != buttonState[0]) {
        buttonState[0] = reading;
        if (buttonState[0] == HIGH) {
          medsleft++;
        }
      }
    }
    buttonState[1] = reading;
  }
  
    
  DROP.write(0);
  
  calibrated = 1;  //set calibrated flag to allow access to necessary code in colorsensor()
  
  lcd.clear(); 
  lcd.setCursor(2,0);
  lcd.print("Search Mode:");
  for(i=10; i!=0; i--){
    lcd.setCursor(7,1);
    if(i==10){
      lcd.print(i);
    }
    else{
      lcd.print("0");
      lcd.print(i);
    }
    delay(1000);
  }
  lcd.setCursor(5,1);
  lcd.print("Engage");
  delay(1000);
  lcd.setCursor(3,1);
  lcd.print("Searching");  
}

int colorsensor(){   //Polls color sensor and stores RGB values in global variable for comparison to determine wether floor or threshold is detected.
  uint16_t trans, red, green, blue; //unit16_t shorthand for unsigned short

  delay(25);  //25ms between samples 
  
  tcs.getRawData(&red, &green, &blue, &trans);
  
  // Figure out some basic hex code for visualization
  unsigned int sum = trans;
 
  r = red; r /= sum;
  g = green; g /= sum;
  b = blue; b /= sum;
  r *= 255; g *= 255; b *= 255;
  
  if(calibrated == 1){
/*
    Serial.print("R: ");Serial.print((int)r);Serial.print(" ");
    Serial.print("G: ");Serial.print((int)g);Serial.print(" ");
    Serial.print("B: ");Serial.print((int)b);Serial.print(" ");
*/  
    //check if floor or threshold is detected.
    if((int)r == constrain((int)r,redmin[0],redmax[0])&&(int)g == constrain((int)g,grnmin[0],grnmax[0])&&(int)b==constrain((int)b,blumin[0],blumax[0])){
    //  Serial.print(" FLOOR"); 
    return 0;
    }
    if((int)r == constrain((int)r,redmin[1],redmax[1])&&(int)g == constrain((int)g,grnmin[1],grnmax[1])&&(int)b==constrain((int)b,blumin[1],blumax[1])){
    //  Serial.print(" THRESH");
    return 1; 
    }
  //  Serial.println();
  }
}

void sonar(){ //DEtermines range in CM for front right and left of robot.
  for(int i = 0; i <= 2; i++){ //scans sensors form right to left each time funtion is called
    digitalWrite(ping[i], LOW);
    delayMicroseconds(2);
    digitalWrite(ping[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(ping[i], LOW);
    
    //detect return pulse, compute range based on pulse high-time.
    time[i] = pulseIn(echo[i],HIGH);
    range[i] = time[i]/29/2; 
    range[i] = constrain(range[i],2,100);  
  }
  /*
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("R:");lcd.print(range[0]);lcd.print("cm ");
  lcd.print("C:");lcd.print(range[1]);lcd.print("cm ");
  lcd.setCursor(0,1);
  lcd.print("L:");lcd.print(range[2]);lcd.print("cm");
  */
  delay(60);    
}
