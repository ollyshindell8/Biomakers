#include <Wire.h>          //enable I2C.
#include "max6675.h"
#include <Ezo_i2c_util.h>  //brings in common print statements
#include <Ezo_i2c.h>       //include the EZO I2C library from https://github.com/Atlas-Scientific/Ezo_I2c_lib (this link also includes the previous library)


//Pins for thermocouples
int SO_PIN = 50;    // Serail Out (SO) pin
int CS_PIN_A = 49;  // Chip Select (CS) pin
int CS_PIN_B = 53;  // Chip Select (CS) pin
int SCK_PIN = 52;   // Clock (SCK) pin

//Pins for relays
int peltierPower = 23;
int solenoidPower = 22;
int heaterPower = 24;
int pumpPower = 26;
int lightPower = 25;

// Create instances of the MAX6675 class with the specified pins
MAX6675 thermocoupleA(SCK_PIN, CS_PIN_A, SO_PIN);
MAX6675 thermocoupleB(SCK_PIN, CS_PIN_B, SO_PIN);

//Create instances of EZO sensors
Ezo_board PH = Ezo_board(99, "PH");     //create a PH circuit object, who's address is 99 and name is "PH"
Ezo_board DO = Ezo_board(97, "DO");     //create an DO circuit object who's address is 97 and name is "DO"
Ezo_board RGB = Ezo_board(112, "RGB");  //create an RGB circuit object who's address is 112 and name is "RGB"

// Variables for serial inputs
char computerdata[20];            //we make a 20 byte character array to hold incoming data from a pc/mac/other.
byte received_from_computer = 0;  //we need to know how many characters have been received.
String inputString = " ";


//Create global varaibles for data
float tempA;
float tempB;

//Utility variables
int safetyTemp = 30;                                    //Safety temperature for thermocouple next to heater to trigger shutoff
float targetTemp = 27;                                  //Target temperature in Celcius
bool lightManual = 0;                                   //Switch between manual control of lights and automatic
bool pumpsManual = 0;                                   //Switch between manual control of pumps and automatic
bool airManual = 0;                                     //Switch between manual control of aeration and automatic
unsigned long lightTimer = 0;                           //Timer for the light cycle
unsigned long RGBInterval = 180000;                     //In milliseconds, time between RGB sensor readings
unsigned long PHInterval =  180000;                     //In milliseconds, time between PH sensor readings
unsigned long DOInterval =  180000;                     //In milliseconds, time between DO sensor readings
unsigned long RGBTime = 30000;                          //Time elapsed tracker for color reading. Initialized time will be time of first reading
unsigned long PHTime =  35000;                          //Time elapsed tracker for PH reading. Initialized time will be time of first reading
unsigned long DOTime =  40000;                          //Time elapsed tracker for DO reading. Initialized time will be time of first reading
unsigned long RGBShift = 0;                             //Shifted time for taking RGB readings
unsigned long PHShift =  0;                             //Shifted time for taking PH readings
unsigned long DOShift =  0;                             //Shifted time for taking DO readings
unsigned long lightsOnTime =  60000;                    //In milliseconds, period for lights to remain on with automated control (14 hours is 50400000)
unsigned long lightsOffTime = 60000;                    //In milliseconds, period for lights to remain off with automated control (10 hours is 36000000)
bool startLightsOn = 1;                                 //Switch to 0 to start with the lights off
unsigned long airDelay =  3000;                         //Time that aeration turns off before color reading
unsigned long pumpDelay = 45000;                        //Time that pumps turn off before color reading

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();                                         //enable I2C port.
  RGB.send_cmd("l,100,t");                              //set LED brightness for color sensor (refer to sensor data sheet for how to change)
  pinMode(peltierPower, OUTPUT);
  pinMode(solenoidPower, OUTPUT);
  pinMode(heaterPower, OUTPUT);
  pinMode(pumpPower, OUTPUT);
  pinMode(lightPower, OUTPUT);
  digitalWrite(peltierPower, LOW);                      //Start all the relays as off
  digitalWrite(solenoidPower, LOW);
  digitalWrite(heaterPower, LOW);
  digitalWrite(pumpPower, LOW);
  digitalWrite(lightPower, LOW);
}

void readColor() {
  RGB.send_cmd("r");
  delay(400);      //RGB sensor needs 300 ms to take reading
  receive_and_print_response(RGB);
}

void readPH() {
  PH.send_read_cmd();
  delay(1000);      //PH sensor needs 900 ms to take reading
  receive_and_print_reading(PH);
  Serial.println();
}

void readDO() {
  DO.send_read_cmd();
  delay(700);       //DO sensor needs 600 ms to take reading
  receive_and_print_reading(DO);
  Serial.println();
}

void readTemp() {
  tempA = thermocoupleA.readCelsius();
  // Serial.print("Temperature A: ");
  // Serial.println(tempA);
  delay(10);
  tempB = thermocoupleB.readCelsius();
  // Serial.print("Temperature B: ");
  // Serial.println(tempB);
}


void loop() {
  //readTemp();     //Temperature readings of both thermocouples, values assign to tempA and tempB
  RGBShift = millis()+RGBInterval;
  PHShift = millis()+PHInterval;
  DOShift = millis()+DOInterval;

  if(PHShift >= PHTime+PHInterval){
    readPH();
    PHTime = PHShift;
  }
  if(DOShift >= DOTime+DOInterval){
    readDO();
    DOTime = DOShift;
  }
  //Color reading
  if (RGBShift >= (RGBTime+airDelay+pumpDelay+RGBInterval)) {       //After the pumps have been off for the appropriate amount of time, the reading is taken, and things are reset
    readColor();
    RGBTime = RGBShift;
    if(!pumpsManual){digitalWrite(pumpPower, HIGH);}
    if(!airManual){digitalWrite(solenoidPower, LOW);}
  }
  else if (RGBShift >= (RGBTime+airDelay+RGBInterval)) {            //After the aeration has been off for the appropriate amount of time, the pumps turn off
    if(!pumpsManual){digitalWrite(pumpPower, LOW);}
  }
  else if (RGBShift >= RGBTime+RGBInterval) {                       //When time color interval is first reached, the aeration turns off
    if(!airManual){digitalWrite(solenoidPower, HIGH);}
  }
  else{
    if(!pumpsManual){digitalWrite(pumpPower, HIGH);}
  }
  //Temperature adjustment
  // if(tempControl){
  //   if(tempA > safetyTemp) {
  //     digitalWrite(heaterPower, LOW);
  //   }
  //   else {
  //       if (tempB < targetTemp) {
  //         digitalWrite(heaterPower, HIGH);
  //         digitalWrite(peltierPower, LOW);
  //       } 
  //       else {
  //           digitalWrite(heaterPower, LOW);
  //           digitalWrite(peltierPower, HIGH);
  //         }
  //     }
  // }
  
  //Lights cycle
  if(!lightManual){
    if(startLightsOn){
      if(millis() - lightTimer <= lightsOnTime){
        digitalWrite(lightPower, HIGH);
      }
      else if(millis()-lightTimer < lightsOnTime + lightsOffTime){
        digitalWrite(lightPower, LOW);
      }
      else{
        lightTimer = millis();
      }
    }
    else{
      if(millis() - lightTimer <= lightsOffTime){
        digitalWrite(lightPower, LOW);
      }
      else if(millis()-lightTimer < lightsOnTime + lightsOffTime){
        digitalWrite(lightPower, HIGH);
      }
      else{
        lightTimer = millis();
      }
    }
  }
}

void serialEvent() {                                                              //this interrupt will trigger when the data coming from the serial monitor(pc/mac/other) is received.
  received_from_computer = Serial.readBytesUntil(13, computerdata, 20);           //we read the data sent from the serial monitor(pc/mac/other) until we see a <CR>. We also count how many characters have been received.
  computerdata[received_from_computer] = 0;                                       //stop the buffer from transmitting leftovers or garbage
  inputString = String(computerdata);                                             //convert the serial input to a string
  if(inputString.indexOf("temp") != -1){                                          //Input code required: tempXX.XX
    targetTemp = inputString.substring(4,inputString.length()).toFloat();
    Serial.print("New Temperature: ");
    Serial.println(targetTemp);
  }
  else if(inputString.indexOf("rgbinterval") != -1){                              //Input code required: rgbintervalXXXXXXXX
    RGBInterval = inputString.substring(11,inputString.length()).toInt();
    Serial.print("New RGB Interval: ");
    Serial.println(RGBInterval);
  }
  else if(inputString.indexOf("phinterval") != -1){                               //Input code required: phintervalXXXXXXXX
    PHInterval = inputString.substring(10,inputString.length()).toInt();
    Serial.print("New PH Interval: ");
    Serial.println(PHInterval);
  }
  else if(inputString.indexOf("dointerval") != -1){                               //Input code required: dointervalXXXXXXXX
    DOInterval = inputString.substring(10,inputString.length()).toInt();
    Serial.print("New DO Interval: ");
    Serial.println(DOInterval);
  }
  else{
    switch (String(computerdata).toInt()) {
      case 1:  //color sensor reading
        RGBTime = millis();
        Serial.println("Command 1 received");
        break;
      case 2:  //color sensor calibrate
        RGB.send_cmd("cal");
        Serial.println("Command 2 received");
        break;
      case 3:   //PH reading
        PHTime = millis();
        Serial.println("Command 3 received");
        break;
      case 4:   //PH calibration
        PH.send_cmd("cal");
        Serial.println("Command 4 received");
        break;
      case 5:  //DO reading
        DOTime = millis();
        Serial.println("Command 5 received");
        break;
      case 6:  //DO calibration
        DO.send_cmd("cal");
        Serial.println("Command 6 received");
        break;
      case 7: //Case: turn on the pumps
        digitalWrite(pumpPower, HIGH);
        Serial.println("Command 7 received");
        pumpsManual = 1;
        break;
      case 8: //Case: turn off the pumps
        digitalWrite(pumpPower, LOW);
        Serial.println("Command 8 received");
        pumpsManual = 1;
        break;
      case 9: //Case: switch pump control back to arduino automated cycle
        Serial.println("Command 9 received");
        pumpsManual = 0;
        break;
      case 10: //Case: turn on the air
        digitalWrite(solenoidPower, LOW);
        Serial.println("Command 10 received");
        airManual = 1;
        break;
      case 11:  //Case: turn off the air
        digitalWrite(solenoidPower, HIGH);
        Serial.println("Command 11 received");
        airManual = 1;
        break;
      case 12:  //Case: switch air control back to arduino automated cycle
        airManual = 0;
        Serial.println("Command 12 received");
        break;
      case 13: //Case: turn on the lights
        digitalWrite(lightPower, HIGH);
        Serial.println("Command 13 received");
        lightManual = 1;
        break;
      case 14:  //Case: turn off the lights
        digitalWrite(lightPower, LOW);
        Serial.println("Command 14 received");
        lightManual = 1;
        break;
      case 15:  //Case: switch light control back to arduino automated cycle
        lightManual = 0;
        Serial.println("Command 15 received");
        break;
      // case 16:  //Case: disable heating/cooling
      //   tempControl = 0;
      //   break;
      // case 17:  //Case: enable heating/cooling
      //   tempControl = 1;
      //   break;
    }
  }
}
