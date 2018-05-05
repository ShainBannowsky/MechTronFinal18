// NOTE: Signature guides probably deprecated.
// General Signature Guides:
// Purple (Elderly): Signature One
// Red (Fire/Danger): Signature Two
// Blue (Water): Signature Three
// Green/Yellow (Eletank): CC Signature Four/Five

#include <SPI.h>
#include <Servo.h>
#include <SoftwareSerial.h>

#define PIN_ELDERLY_CONNECT      13

#define PIN_SPIGOT       9
#define PIN_SYRINGE      3

#define PIN_R_ENABLE     5
#define PIN_R_FORWARD    4
#define PIN_R_BACKWARD   2

#define PIN_L_ENABLE     6
#define PIN_L_FORWARD    7
#define PIN_L_BACKWARD   8

#define MSEC_FILLTIME 1000
//#define MSEC_OUNCE  // To be determined later
#define A_REST_SPIGOT  135
#define A_FILL_SPIGOT  -45

SoftwareSerial TankBluetooth(10, 11); // RX, TX
Servo servo_spigot;
Servo servo_syringe;

//Boost Variables
int poss[5];// = [1 2 3 4 5];
int curr = 0;

// BT Variables:
int transmissionData[6];
String incomingString;

// Elderly Signal Variables:
//int elderly_sig_out = 12; // Elderly Detection Output Signal 
//int elderly_sig_inp = 13; // Elderly Detection Inupt  Signal

// Mission:
int mission = 0;
bool fire_extinguished = false;
bool fire_detected = false;

void setup() {
  Serial.println("Begin Initialization...\n");
  
  TankBluetooth.begin(9600);
  Serial.begin(9600); 

  // Initalizing Servos:
  //servo_syringe.attach(PIN_SYRINGE);
  servo_spigot.attach(PIN_SPIGOT);
  servo_spigot.write(50);
  servo_spigot.detach();

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
}

// Using Starting Position, State Machines, and Functions / Reference Locations to Direct EleTank.
void loop() {

  if (TankBluetooth.available()) {
    
    incomingString = TankBluetooth.readStringUntil('$');
    TankBluetooth.println(incomingString);
    populateData(incomingString); 
    Serial.println(incomingString);
    
    if ((transmissionData[4] != 0 || transmissionData[5] != 0) && fire_detected == false) {
      Serial.println("Danger!");
      fire_detected = true;
      mission = 2;
    }
  }
  
  switch (mission) {
    case 1: // Go To Elderly
      if (transmissionData[1] != 0) {  
        tankTurn(transmissionData[0]);
        tankDrive(transmissionData[1]);
      } else if (transmissionData[1] == 0 && fire_extinguished == false) {
         //Should actually play sound/tap to wake up
        servo_spigot.attach(PIN_SPIGOT);
        servo_spigot.write(50);
        servo_spigot.detach();
        delay(2000); //Allow the Elderly to move out of our way
        mission = 2;
      } else if (transmissionData[1] == 0 && fire_extinguished == true) {
        tankStop();
        while(1) {// Done.
        }
      }
      break;
    case 2: // Get Water
      if (transmissionData[3] != 0) {
        tankTurn(transmissionData[2]);
        tankDrive(transmissionData[3]);
      } else {
        //Actuate Water System
        servo_spigot.attach(PIN_SPIGOT);
        servo_syringe.attach(PIN_SYRINGE);
        delay(2000);
        servo_spigot.write(10);
        servo_syringe.write(0); // "0" for filling syringe.
        delay(5000); // Time to fill is experimentally determined.
        servo_syringe.write(90); // "90" to stop filling syringe.
        delay(100);
        servo_syringe.detach();
        servo_spigot.detach();

        //Move out of the way
        tankReverse(5);
        mission = 3;
      }
      break;
    case 3: // Extinguish Fire
      if (transmissionData[5] != 0) {
        tankTurn(transmissionData[4]);
        tankDrive(transmissionData[5]);
      } else {
        mission = 1;
        servo_spigot.attach(PIN_SPIGOT);
        servo_syringe.attach(PIN_SYRINGE);
        servo_spigot.write(50);
        servo_syringe.write(180); // "0" for filling syringe.
        delay(2000); // Time to fill is experimentally determined.
        servo_syringe.write(90); // "90" to stop filling syringe.
        servo_syringe.detach();
        servo_spigot.detach();
        fire_extinguished = true;
      }
      break;
    default:
      tankStop();
      delay(200);
      break;
    }
  Serial.print("On Mission: ");
  Serial.println(mission);

  //checkBoost();
}

/************************************************/
/**********                         *************/
/**********     HELPER FUNCTIONS    *********** */
/**********                         *************/
/************************************************/

void checkBoost() {
  if (fire_detected) {
    if (abs(poss[curr] - poss[curr+1%5]) < 15) {
      tankTurn(90);
      tankDrive(10);
    }
  }
  return;
}

void syringeFill() {
  // Fill Syringe, assuming position-controlled servo.
  servo_syringe.write(0); // "0" for filling syringe.
  delay(MSEC_FILLTIME); // Time to fill is experimentally determined.
  servo_syringe.write(90); // "90" to stop filling syringe.
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
  Serial.println(dir);
  if(dir > 15) { // Turn Right
    analogWrite(PIN_L_ENABLE, 255);
    analogWrite(PIN_R_ENABLE, 255);
    digitalWrite(PIN_L_FORWARD, HIGH);
    digitalWrite(PIN_L_BACKWARD, LOW);
    digitalWrite(PIN_R_FORWARD, LOW);
    digitalWrite(PIN_R_BACKWARD, HIGH);
    delay(3*dir);
  } else if (dir < -15) { //Turn Left
    analogWrite(PIN_L_ENABLE, 255);
    analogWrite(PIN_R_ENABLE, 255);
    digitalWrite(PIN_L_FORWARD, LOW);
    digitalWrite(PIN_L_BACKWARD, HIGH);
    digitalWrite(PIN_R_FORWARD, HIGH);
    digitalWrite(PIN_R_BACKWARD, LOW);
    delay(3*dir);
  } else {
    tankStop();
  }
  tankStop();
  delay(100);
}

void tankDrive(int del) {
  analogWrite(PIN_L_ENABLE, 200);
  analogWrite(PIN_R_ENABLE, 200);
  digitalWrite(PIN_L_FORWARD, HIGH);
  digitalWrite(PIN_L_BACKWARD, LOW);
  digitalWrite(PIN_R_FORWARD, HIGH);
  digitalWrite(PIN_R_BACKWARD, LOW);
  delay(del*250);
  tankStop();
  delay(100);
}

void tankReverse(int del) {
  analogWrite(PIN_L_ENABLE, 255);
  analogWrite(PIN_R_ENABLE, 255);
  digitalWrite(PIN_L_FORWARD, LOW);
  digitalWrite(PIN_L_BACKWARD, HIGH);
  digitalWrite(PIN_R_FORWARD, LOW);
  digitalWrite(PIN_R_BACKWARD, HIGH);
  delay(del*100);
  tankStop();
}

// Functions handling wireless information:

void populateData(String incomingString) {
  int firstSpace = incomingString.indexOf(' ');
  String firstStr = incomingString.substring(0,firstSpace);
  String sub1 = incomingString.substring(firstSpace+1);
  transmissionData[0] = firstStr.toInt();
  
  int secSpace = sub1.indexOf(' ');
  String secStr = sub1.substring(0,secSpace);
  String sub2 = sub1.substring(secSpace+1);
  transmissionData[1] = secStr.toInt();
   
  int thirdSpace = sub2.indexOf(' ');
  String thirdStr = sub2.substring(0,thirdSpace);
  String sub3 = sub2.substring(thirdSpace+1);
  transmissionData[2] = thirdStr.toInt();
   
  int fourthSpace = sub3.indexOf(' ');
  String fourthStr = sub3.substring(0, fourthSpace);
  String sub4 = sub3.substring(fourthSpace+1);
  transmissionData[3] = fourthStr.toInt();
    
  int fifthSpace = sub4.indexOf(' ');
  String fifthStr = sub4.substring(0,fifthSpace);
  String sub5 = sub4.substring(fifthSpace+1);
  transmissionData[4] = fifthStr.toInt();
   
  String sixStr = sub5;
  transmissionData[5] = sixStr.toInt();

  if (curr == 5) {
    curr = 0;
  }
  poss[curr] = transmissionData[2];
  curr++;
   
  return;
}
