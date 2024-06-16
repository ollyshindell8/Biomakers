#include "TimerOne.h"
#include <util/atomic.h>

// Encoder setup
const int LCHA_PIN = 18;
const int LCHB_PIN = 19;
const int RCHA_PIN = 20;
const int RCHB_PIN = 21;
volatile int Lenc_count = 0; volatile int Lenc_change = 0;
int Ltemp_count = 0;
volatile int Renc_count = 0; volatile int Renc_change = 0;
int Rtemp_count = 0;
long int speed_time = 0;
long Lspeed = 0;
long Rspeed = 0;
int desired_speed = 200;
float sampling_delay = 10;
long int pos_time = 0; float turnPos = 420; float Rpos = 0; bool turnSwitch = 0; long int turnSettle = 6000;

// Ultrasonic setup
#define TIMER_US 50                                   // 50 uS timer duration 
#define TICK_COUNTS 4000                              // 200 mS worth of timer ticks
const int RtrigPin = 12;
const int RechoPin = 2;
const int FtrigPin = 10;
const int FechoPin = 3;
volatile long echo_start = 0;                         // Records start of echo pulse 
volatile long echo_end = 0;    
volatile long echo_duration = 0;                      
volatile int trigger_time_count = 0;                  
volatile long range_flasher_counter = 0;              
long int ultraSpace = 69; // variable to control distance change reading

// Motor control setup
int leftMotor = 8;
int rightMotor = 9;
int test = 7;
int In1 = 34;
int In2 = 32;
int In3 = 30;
int In4 = 28;
float Lerror = 0; float Lerror_prior = 0; float Lintegral_prior = 0; float Lintegral = 0; float Lderivative = 0; float Ldrive = 0;
float Rerror = 0; float Rerror_prior = 0; float Rintegral_prior = 0; float Rintegral = 0; float Rderivative = 0; float Rdrive = 0; float Rturn = 0;
float actual_time = 0;
float LKP = .1; float LKI = 0.03; float LKD = .5; float RKP = .1; float RKI = 0.03; float RKD = .5;
float Rscaling_factor = 7; float Lscaling_factor = 7; float dist_tot = 0;

//SubFSM setup
int plantdrive = 1; int walldrive = 2; int turning = 3; int stop = 4; int prevstate = stop; int state = stop;


void setup (){
  pinMode(LCHA_PIN, INPUT);
  pinMode(LCHB_PIN, INPUT);
  pinMode(RCHA_PIN, INPUT);
  pinMode(RCHB_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(LCHA_PIN), Lencoder, RISING);
  attachInterrupt(digitalPinToInterrupt(RCHA_PIN), Rencoder, RISING);
  pinMode(leftMotor, OUTPUT);
  pinMode(rightMotor, OUTPUT);
  pinMode(test, INPUT);
  pinMode(In1, OUTPUT);
  pinMode(In2, OUTPUT);
  pinMode(In3, OUTPUT);
  pinMode(In4, OUTPUT);
  pinMode(RtrigPin, OUTPUT); // Sets trigPin as output
  pinMode(RechoPin, INPUT); // Sets echoPin as input
  pinMode(FtrigPin, OUTPUT); // Sets trigPin as output
  pinMode(FechoPin, INPUT); // Sets echoPin as input
  Timer1.initialize(TIMER_US);                      // Initialise timer 1
  Timer1.attachInterrupt(timerIsr);                 // Attach interrupt to the timer service routine 
  attachInterrupt(digitalPinToInterrupt(2), Recho_interrupt, CHANGE);  // Attach interrupt to the sensor echo input
  attachInterrupt(digitalPinToInterrupt(3), Fecho_interrupt, CHANGE);
  Serial.begin(9600);
}


void loop () {
  switch(state){
    case 1:
    if(millis()-speed_time > sampling_delay){
      actual_time = millis()-speed_time;
      Lspeed = (Lenc_count-Ltemp_count)*1000/(actual_time*Lscaling_factor);
      Rspeed = (Renc_count-Rtemp_count)*1000/(actual_time*Rscaling_factor);
      Serial.println(Rspeed);
      Ltemp_count = Lenc_count;
      Rtemp_count = Renc_count;
      speed_time = millis();
      Lerror = (1.1*desired_speed)-Lspeed;
      Lintegral = Lintegral_prior + Lerror*actual_time;
      Lderivative = (Lerror-Lerror_prior)/actual_time;
      Ldrive = LKI*Lintegral;
      Lerror_prior = Lerror;
      Lintegral_prior = Lintegral;
      Rerror = (1*desired_speed)-Rspeed;
      Rintegral = Rintegral_prior + Rerror*actual_time;
      Rderivative = (Rerror-Rerror_prior)/actual_time;
      Rdrive = RKI*Rintegral;
      Rerror_prior = Rerror;
      Rintegral_prior = Rintegral;
      if(Ldrive > 255){
        Ldrive = 255;
      }
      if(Ldrive < 0){
        Ldrive = 0;
      }
      if(Rdrive > 255){
        Rdrive = 255;
      }
      if(Rdrive < 0){
        Rdrive = 0;
      }
    }
    if((echo_duration/58) > 30 & echo_duration != 0 & echo_duration/58 < 90 & ultraSpace == 69 & millis() > 3000){
      ultraSpace = millis();
    }
    else if(millis()-ultraSpace > 40 & ultraSpace != 69){
      if((echo_duration/58) > 25){
        state = stop;
        prevstate = plantdrive;
        speed_time = millis();
        ultraSpace = 69;
      }
      else{
        ultraSpace = 69;
      }
    }
    break;
    case 2:
    if(millis()-speed_time > sampling_delay){
      actual_time = millis()-speed_time;
      Lspeed = (Lenc_count-Ltemp_count)*1000/(actual_time*Lscaling_factor);
      Rspeed = (Renc_count-Rtemp_count)*1000/(actual_time*Rscaling_factor);
      Ltemp_count = Lenc_count;
      Rtemp_count = Renc_count;
      speed_time = millis();
      Lerror = (1.1*desired_speed)-Lspeed;
      Lintegral = Lintegral_prior + Lerror*actual_time;
      Lderivative = (Lerror-Lerror_prior)/actual_time;
      Ldrive = LKP*Lerror + LKI*Lintegral + LKD*Lderivative;
      Lerror_prior = Lerror;
      Lintegral_prior = Lintegral;
      Rerror = (1*desired_speed)-Rspeed;
      Rintegral = Rintegral_prior + Rerror*actual_time;
      Rderivative = (Rerror-Rerror_prior)/actual_time;
      Rdrive = RKP*Rerror + RKI*Rintegral + RKD*Rderivative;
      Rerror_prior = Rerror;
      Rintegral_prior = Rintegral;
      if(Ldrive > 255){
        Ldrive = 255;
      }
      if(Ldrive < 0){
        Ldrive = 0;
      }
      if(Rdrive > 255){
        Rdrive = 255;
      }
      if(Rdrive < 0){
        Rdrive = 0;
      }
    }
    //Serial.println(echo_duration/58);
    if((echo_duration/58) < 32 & (echo_duration/58) > 2){
      prevstate = walldrive;
      state = stop;
      speed_time = millis();
    }
    break;
    case 3:
    if(millis()-pos_time > sampling_delay){
      actual_time = millis()-pos_time;
      Rpos = Renc_count/Rscaling_factor;
      //Serial.println(Rturn);
      Rtemp_count = Renc_count;
      pos_time = millis();
      Rerror = (turnPos)-Rpos;
      Rderivative = (Rerror-Rerror_prior)/actual_time;
      Rturn = 10*Rerror;
      if(Rpos > (turnPos-10)){
              //Serial.println(Rturn);
        Rintegral = Rintegral_prior + Rerror*actual_time;
        Rturn = 15*Rerror + .01*Rintegral;
      }

      Rerror_prior = Rerror;
      Rintegral_prior = Rintegral;
    }
    if(Rturn < 0){
      //Rturn = abs(Rturn);
      turnSwitch = 1;
    }
    else{
      turnSwitch = 0;
    }
    if(Rturn > 255){
      Rturn = 255;
    }
    if(Rpos > turnPos){
      if(turnSettle == 6000){
        turnSettle = millis();
      }
    }
    if(millis()-turnSettle > 1000 & turnSettle != 6000){
        state = stop;
        prevstate = turning;
        speed_time = millis();
        turnSettle = 6000;
      }
    break;
    case 4:
    if(millis()-speed_time > 1000){
      Lerror = 0; Lerror_prior = 0; Lintegral_prior = 0; Lintegral = 0; Lderivative = 0; Ldrive = 0;
      Rerror = 0; Rerror_prior = 0; Rintegral_prior = 0; Rintegral = 0; Rderivative = 0; Rdrive = 0;
      Lenc_count = 0; Lenc_change = 0; Ltemp_count = 0; Renc_count = 0; Renc_change = 0; Rtemp_count = 0; Lspeed = 0; Rspeed = 0; Rpos = 0;
      // Serial.println(prevstate);
      // Serial.println(state);
      if(prevstate == plantdrive){
        state = walldrive;
      }
      if(prevstate == walldrive){
        state = turning;
      }
      if(prevstate == turning | prevstate == stop){
        state = plantdrive;
        // int iCount = 0;
        // for(iCount < 20; iCount++;){
        //   digitalWrite(RtrigPin, LOW);
        //   delay(0.002);
        //   digitalWrite(RtrigPin, HIGH);
        //   delay(0.01);
        //   Rduration = pulseIn(RechoPin, HIGH);
        //   Rdistance = Rduration*0.034/2;
        //   digitalWrite(RtrigPin, LOW);
        //   medianFilter.AddValue(Rdistance);
          //Serial.println(medianFilter.GetFiltered());
        // }
      }
      prevstate = stop;
    }
    break;
  }

  if(state == walldrive | state == plantdrive){
    digitalWrite(In1, LOW);
    digitalWrite(In2, HIGH);
    digitalWrite(In3, LOW);
    digitalWrite(In4, HIGH);
    analogWrite(leftMotor, Ldrive);
    analogWrite(rightMotor, Rdrive);
  }
  else if(state == turning){
    digitalWrite(In1, HIGH);
    digitalWrite(In2, HIGH);
    if(turnSwitch == 1){
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
    }
    else{
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
    }
    analogWrite(leftMotor, 0);
    analogWrite(rightMotor, Rturn);
  }
  else{
    digitalWrite(In1, HIGH);
    digitalWrite(In2, HIGH);
    digitalWrite(In3, HIGH);
    digitalWrite(In4, HIGH);
    analogWrite(leftMotor, 200);
    analogWrite(rightMotor, 200);
  }
  //Serial.println(state);
}

void Rencoder() {
  Renc_change = readEncoder(RCHB_PIN);
  if (Renc_change != 0) {
    Renc_count = Renc_count - Renc_change;
  }
}

void Lencoder() {
  Lenc_change = readEncoder(LCHB_PIN);
  if (Lenc_change != 0) {
    Lenc_count = Lenc_count + Lenc_change;
  }
}

int result = 0;
int chA_last = 0;
int chB_last = 0;
int chA_new = 0;
int chB_new = 0;
int readEncoder(int chB) {
  result = 0;
  chB_new = digitalRead(chB);
    if ((chB_new == HIGH)) { // CW turn
      result = 1;
    }
    if ((chB_new == LOW)) { // CW turn
      result = -1;
    }
  return result;
}

void timerIsr() {
  static volatile int ultrastate = 0;                 // State machine variable
  if (!(--trigger_time_count))                   // Count to 200mS
  {                                              // Time out - Initiate trigger pulse
      trigger_time_count = TICK_COUNTS;           // Reload
      ultrastate = 1;                                  // Changing to state 1 initiates a pulse
  }

  switch(ultrastate)                                  // State machine handles delivery of trigger pulse
  {
    case 0:                                      // Normal state does nothing
        break;
    
    case 1:
        if(state == plantdrive){
          digitalWrite(RtrigPin, HIGH); 
        }
        else if(state == walldrive){
          digitalWrite(FtrigPin, HIGH);
        }                                      
        ultrastate = 2;                                // and set state to 2
        break;
    
    case 2:                                      // Complete the pulse
    default:
        if(state == plantdrive){
          digitalWrite(RtrigPin, LOW); 
        }
        else if(state == walldrive){
          digitalWrite(FtrigPin, LOW);
        }                 
        ultrastate = 0;                                // and return state to normal 0
        break;
  }                            // Flash the onboard LED distance indicator
}

void Recho_interrupt()
{
  switch(digitalRead(RechoPin))                     // Test to see if the signal is high or low
  {
    case HIGH:                                      // High so must be the start of the echo pulse
      echo_end = 0;                                 // Clear the end time
      echo_start = micros();                        // Save the start time
      break;
      
    case LOW:                                       // Low so must be the end of hte echo pulse
      echo_end = micros();                          // Save the end time
      echo_duration = echo_end - echo_start;        // Calculate the pulse duration
      break;
  }
}
void Fecho_interrupt()
{
  switch(digitalRead(FechoPin))                     // Test to see if the signal is high or low
  {
    case HIGH:                                      // High so must be the start of the echo pulse
      echo_end = 0;                                 // Clear the end time
      echo_start = micros();                        // Save the start time
      break;
      
    case LOW:                                       // Low so must be the end of hte echo pulse
      echo_end = micros();                          // Save the end time
      echo_duration = echo_end - echo_start;        // Calculate the pulse duration
      break;
  }
}