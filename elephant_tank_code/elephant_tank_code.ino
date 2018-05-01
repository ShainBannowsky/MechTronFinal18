// NOTE: Signature guides probably deprecated.
// General Signature Guides:
// Purple (Elderly): Signature One
// Red (Fire/Danger): Signature Two
// Blue (Water): Signature Three
// Green/Yellow (Eletank): CC Signature Four/Five

#include <SPI.h>
#include <Servo.h>
#include <SoftwareSerial.h>

#define RX 11
#define TX 10

#define PIN_ELDERLY_CONNECT      13

//#define PIN_SPIGOT      13
#define PIN_SYRINGE      9

#define PIN_L_ENABLE     5
#define PIN_L_FORWARD    3
#define PIN_L_BACKWARD   4

#define PIN_R_ENABLE     6
#define PIN_R_FORWARD    7
#define PIN_R_BACKWARD   8

#define MSEC_FILLTIME 2000 // Yet to be determined experimentally
#define MSEC_OUNCE     600  // To be determined later
#define A_REST_SPIGOT  135
#define A_FILL_SPIGOT  -45

SoftwareSerial TankBluetooth(10, 11); // RX, TX
Servo servo_spigot;
Servo servo_syringe;

// BT Variables:
double transmissionData[6] = {0,0,0,0,0,0};
String incomingString;

// Elderly Signal Variables:
//int elderly_sig_out = 12; // Elderly Detection Output Signal 
//int elderly_sig_inp = 13; // Elderly Detection Inupt  Signal

// Mission:
int mission = 0;
bool fire_extinguished = false;

void setup() {
  Serial.println("Begin Initialization...\n");
  
  TankBluetooth.begin(9600);
  Serial.begin(9600); 

  // Initalizing Servos:
  //servo_spigot.attach(PIN_SPIGOT);
  //servo_syringe.attach(PIN_SYRINGE);

  pinMode(PIN_ELDERLY_CONNECT, INPUT_PULLUP);
  //  digitalWrite(elderly_sig_out, HIGH); 

  // Initalizing Pins:
  pinMode(PIN_L_ENABLE, OUTPUT);
  pinMode(PIN_L_FORWARD, OUTPUT);
  pinMode(PIN_L_BACKWARD, OUTPUT);
  pinMode(PIN_R_ENABLE, OUTPUT);
  pinMode(PIN_R_FORWARD, OUTPUT);
  pinMode(PIN_R_BACKWARD, OUTPUT);
  
  Serial.println("Initialization Complete...\n");
  if (TankBluetooth.available()) {
    tankStop();
    
    while(transmissionData[5] == 0 && transmissionData[6] == 0) {
      // Stay here until danger is detected.
      incomingString = TankBluetooth.readString();
      int n = 0;
      char strCharArray[40];
      while(incomingString[n] != '\n') {
        strCharArray[n] = incomingString[n];
        n++;
      }
    stringToDoubleArray(strCharArray);
    }
    Serial.println("Danger Detected!");
  }
}

// Using Starting Position, State Machines, and Functions / Reference Locations to Direct EleTank.
void loop() {

  if (TankBluetooth.available()) {
    incomingString = TankBluetooth.readString(); 
    int n = 0;
    char strCharArray[40];
    while(incomingString[n] != '\n') {
        strCharArray[n] = incomingString[n];
        n++;
    }
    stringToDoubleArray(strCharArray); // <--- Gets and Stores Info in Global Double Array.
  }

  switch (mission) {
    case 1: // Go To Elderly
      if (transmissionData[1] != 0) {  
        tankTurn(transmissionData[0]);
        tankDrive(transmissionData[1]);
      } else if (transmissionData[1] == 0 && fire_extinguished == false) {
        mission = 2;
      } else if (transmissionData[1] == 0 && fire_extinguished == true) {
        tankStop();
        while(1); // Done.
      }
    case 2: // Get Water
      if (transmissionData[3] != 0) {
        tankTurn(transmissionData[2]);
        tankDrive(transmissionData[3]);
        // Fine adjustments go here
        spigot(A_FILL_SPIGOT);
        syringeFill(MSEC_FILTIME);
      } else {
        mission = 3;
      }
    case 3: // Extinguish Fire
      if (transmissionData[5] != 0) {
        tankTurn(transmissionData[4]);
        tankDrive(transmissionData[5]);
        // Fire targetting stuff goes here - currently not implemented.
        // Need to calculate spigot angle.
        // Could potentially make actual fire extinguishing next mission.
        syringeFire(MSEC_OUNCE);
      } else {
        mission = 4;
        fire_extinguished = true;
      }
    default:
      tankStop();
  }
  
}

/************************************************/
/**********                         *************/
/**********     HELPER FUNCTIONS    *********** */
/**********                         *************/
/************************************************/

void syringeFill(msec_time) {
  // Fill Syringe, assuming position-controlled servo.
  servo_syringe.write(0); // "0" for filling syringe.
  delay(msec_time); // Time to fill is experimentally determined.
  servo_syringe.write(90); // "90" to stop filling syringe.
}

void syringeFire(msec_time) {
  servo_syringe.write(180); // "180" for emptying syringe.
  delay(msec_time); // Spray water for specified amount of time.
  servo_syringe.write(90); // "90" to stop emptying syringe.
}

void spigot(int input_angle) {
  // Sanity check!
  int angle = (int)input_angle;
  if(angle > 90) {
    servo_spigot.write(180);
  } else if(angle < (-90)) {
    servo_spigot.write(0);
  } else {
    servo_spigot.write(angle);
  }
}

void tankStop() {
  // Stops tank tread's movement.
  analogWrite(PIN_L_ENABLE, 0);
  digitalWrite(PIN_L_FORWARD, LOW);
  digitalWrite(PIN_L_BACKWARD, LOW);
  analogWrite(PIN_R_ENABLE, 0);
  digitalWrite(PIN_R_FORWARD, LOW);
  digitalWrite(PIN_R_BACKWARD, LOW);
}

void tankTurn(int dir) {
  if(dir >= 5) { //Turn Left
    digitalWrite(PIN_L_FORWARD, HIGH);
    digitalWrite(PIN_L_BACKWARD, LOW);
    digitalWrite(PIN_R_FORWARD, LOW);
    digitalWrite(PIN_R_BACKWARD, HIGH);
    analogWrite(PIN_L_ENABLE, 255);
    analogWrite(PIN_R_ENABLE, 255);
  } else if(dir <= -5) { // Turn Right
    digitalWrite(PIN_L_FORWARD, HIGH);
    digitalWrite(PIN_L_BACKWARD, LOW);
    digitalWrite(PIN_R_FORWARD, LOW);
    digitalWrite(PIN_R_BACKWARD, HIGH);
    analogWrite(PIN_L_ENABLE, 255);
    analogWrite(PIN_R_ENABLE, 255);
  } 
  delay(250);
  tankStop();
}

void tankDrive(int del) {
  digitalWrite(PIN_L_FORWARD, HIGH);
  digitalWrite(PIN_L_BACKWARD, LOW);
  digitalWrite(PIN_R_FORWARD, HIGH);
  digitalWrite(PIN_R_BACKWARD, LOW);
  analogWrite(PIN_L_ENABLE, 255);
  analogWrite(PIN_R_ENABLE, 255);
  delay(del);
  tankStop();
}

void tankReverse(int spd, int del) {
  digitalWrite(PIN_L_FORWARD, LOW);
  digitalWrite(PIN_L_BACKWARD, HIGH);
  digitalWrite(PIN_R_FORWARD, LOW);
  digitalWrite(PIN_R_BACKWARD, HIGH);
  analogWrite(PIN_L_ENABLE, spd);
  analogWrite(PIN_R_ENABLE, spd);
  delay(del);
  tankStop();
}

// Functions handling wireless information:

void stringToDoubleArray(char str[]) {
//void stringToDoubleArray(String str) {
  
  double dArray[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  char charArray[6] = {'0', '0', '0', '0', '0', '0'};
  int count = 0;

  // Convert String to char Array
  char *ch;
  char *rest = str;
  ch = strtok_r(rest, " ", &rest);
  
  int i;
  for (i = 0; i < 6; i++) {
    if (ch != NULL) {
      charArray[i] = ch;
      transmissionData[i] = atoi(ch);
      ch = strtok_r(rest, " ", &rest);
    }
  }
  return;
}
