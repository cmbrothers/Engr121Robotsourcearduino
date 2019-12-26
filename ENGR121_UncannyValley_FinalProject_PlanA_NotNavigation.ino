//Coding for color sensor adapted from source/ example code provided by Adafruit
//Coding for driving LCD with 595 shift register adapted from source/ example code by Rowan Simms
//Code abandoned or adapted into final iterations (PLAN C/ D) at last minute when original navigation fell through.



#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <Servo.h>
#include <LiquidCrystal595.h>    // include the library for driving lcd with 595 shift register

LiquidCrystal595 lcd(6,7,8);     // datapin, latchpin, clockpin

//attach servos
Servo LEFT;
Servo RIGHT;
Servo DROP;

//rangefinder pin arrays
int ping[3] = {3,4,5};
int echo[3] = {0,1,2};

//RGB minmax values for floor, threshold, yellow, orange, red, green, blue
int redmin[7]; 
int redmax[7];
int grnmin[7];  
int grnmax[7];
int blumin[7];  
int blumax[7];

//misc variables
int button = 13;
int calibrated = 0;
int medsleft = 0;

//RGB global values
float r, g, b;

//gammatable for incident
byte gammatable[255];

//Initialize sensor, No gain and 154ms integration time
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_1X);

void setup() {
  Serial.begin(9600);
  Serial.println("Color View Test!");
  
  lcd.begin(16,2);             // 16 characters, 2 rows
  lcd.clear();
  
  //servo pin declaration
  LEFT.attach(12);
  RIGHT.attach(11);
  DROP.attach(10);
	
	//color sensor operational?
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }  
  
  for (int i=1; i<256; i++) { //Build Gamma table
    float x = i;
    x /= 256;
    x = pow(x, 2.5);
    x *= 256;   
    gammatable[i-1] = x;
  }
  pinMode(button,INPUT);
  calsensorservo();  //call calibration function
}

void loop() {
  colorsensor();	//polls colorsensor
}

void roomsearch(int search){ //intended to determine if room was found via threshold detection, then execute grid-style scan for terget by sweeping robot back and forth, navigation dependant.
  if(search == 1){
    lcd.setCursor(2,1);
    lcd.print("Room Found");
        
    do{
      
      delay(1000);
      lcd.setCursor(0,1);
      lcd.print("  ScanningRoom  ");
      
      colorsensor();
  
    }while(colorsensor() == 0);
  } 
}

void targetlock(int target){ //execute target centering based on detected colors, navigation dependant.
  
  switch(target){
    case 1:
      lcd.setCursor(2,1);
      lcd.print("Yellow Found");
      colorsensor();
      break;
    case 2:
      lcd.setCursor(2,1);
      lcd.print("Orange Found");
      colorsensor();
      break;
    case 3:
      lcd.setCursor(2,1);
      lcd.print(" Red Found  ");
      colorsensor();
      break;
    case 4:
      lcd.setCursor(2,1);
      lcd.print(" Green Found");
      colorsensor();
      break;
    case 5:
      lcd.setCursor(2,1);
      lcd.print(" Blue Found ");
      delay(500);
      dropmeds();
      medsleft--;
      break;
  }
  
}

void dropmeds(){ //drop meds function, incomplete in this iteration. semi-functional in PLAN D based on teammates original plan
  
}


void calsensorservo(){ //Measure RGB values, compute and store min/ max values for target acquisition. Missing debounce function for button that is in PLAN D

  String calstep;
  
  int i = 0;
  int rednom[7]; 
  int grnnom[7];
  int blunom[7];

  lcd.print("Servo Cal");

  while(i==0){ //write stop value to both servos for calibration if needed.
    LEFT.write(90);
    RIGHT.write(90);
    if(digitalRead(button) == LOW){
      i++;
    }
  }
  
  lcd.clear();
  
  while(i <= 7 && i!=0){
    
    switch(i){ //changes displayed value to be calibrated.
      case 1:
        calstep = String("FLOOR");
        break;
      case 2:
        calstep = String("THRESH");
        break;
      case 3:
        calstep = String("YELLOW");
        break;
      case 4:
        calstep = String("ORANGE");
        break;
      case 5:
        calstep = String("RED   ");
        break;
      case 6:
        calstep = String("GREEN ");
        break;
      case 7:
        calstep = String("BLUE  ");
        break;
    }
    
    colorsensor(); //polls color sensor
	
	//prints cal step and RGB values to LCD
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
   
   
    //calculates and stores min/max values for RGB detection, increments calstep
    if (digitalRead(button) == LOW){
      delay(100);
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
  
  
  //changes display and allows for loading 'medicine' into hopper, based on teammates original plan.
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Load Medicine:");
  delay(1000);
  while(medsleft<=3){
    switch(medsleft){
      case 0:
        lcd.setCursor(7,1);
        lcd.print("01");
        DROP.write(0);
        break;
      case 1:
        lcd.setCursor(7,1);
        lcd.print("02");
        DROP.write(45);
        break;
      case 2:
        lcd.setCursor(7,1);
        lcd.print("03");
        DROP.write(90);
        break;
      case 3:
        lcd.setCursor(7,1);
        lcd.print("04");
        DROP.write(135);
        break;
    }
    if (digitalRead(button) == LOW){
        delay(100);
        medsleft++;
    }
  }
    
  DROP.write(155);
  
  calibrated = 1;
  
  lcd.clear(); 
  lcd.setCursor(2,0);
  lcd.print("Search Mode:"); //puts countdown timer on LCD
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
  
  
}

int colorsensor(){ //polls color sensor and stores values globally for comparison. Serial print commands for debug use only.
  uint16_t trans, red, green, blue; //unit16_t shorthand for unsigned short

  delay(160);  //160ms between samples 
  
  tcs.getRawData(&red, &green, &blue, &trans);
  
  // Figure out some basic hex code for visualization
  unsigned int sum = trans;
 
  r = red; r /= sum;
  g = green; g /= sum;
  b = blue; b /= sum;
  r *= 255; g *= 255; b *= 255;
  
  if(calibrated == 1){

    Serial.print("R: ");Serial.print((int)r);Serial.print(" ");
    Serial.print("G: ");Serial.print((int)g);Serial.print(" ");
    Serial.print("B: ");Serial.print((int)b);Serial.print(" ");
    if((int)r == constrain((int)r,redmin[0],redmax[0])&&(int)g == constrain((int)g,grnmin[0],grnmax[0])&&(int)b==constrain((int)b,blumin[0],blumax[0])){
      Serial.print(" FLOOR"); 
      return 0;
    }
    if((int)r == constrain((int)r,redmin[1],redmax[1])&&(int)g == constrain((int)g,grnmin[1],grnmax[1])&&(int)b==constrain((int)b,blumin[1],blumax[1])){
      Serial.print(" THRESH"); 
      roomsearch(1);
    }
    if((int)r == constrain((int)r,redmin[2],redmax[2])&&(int)g == constrain((int)g,grnmin[2],grnmax[2])&&(int)b==constrain((int)b,blumin[2],blumax[2])){
      Serial.print(" YELLOW");
      targetlock(1);
    }
    if((int)r == constrain((int)r,redmin[3],redmax[3])&&(int)g == constrain((int)g,grnmin[3],grnmax[3])&&(int)b==constrain((int)b,blumin[3],blumax[3])){
      Serial.print(" ORANGE"); 
      targetlock(2);
    }
    if((int)r == constrain((int)r,redmin[4],redmax[4])&&(int)g == constrain((int)g,grnmin[4],grnmax[4])&&(int)b==constrain((int)b,blumin[4],blumax[4])){
      Serial.print(" RED"); 
      targetlock(3);
    }
    if((int)r == constrain((int)r,redmin[5],redmax[5])&&(int)g == constrain((int)g,grnmin[5],grnmax[5])&&(int)b==constrain((int)b,blumin[5],blumax[5])){
      Serial.print(" GREEN"); 
      targetlock(4);
    }
    if((int)r == constrain((int)r,redmin[6],redmax[6])&&(int)g == constrain((int)g,grnmin[6],grnmax[6])&&(int)b==constrain((int)b,blumin[6],blumax[6])){
      Serial.print(" BLUE"); 
      targetlock(5);
    }
      Serial.println();
  }
}
