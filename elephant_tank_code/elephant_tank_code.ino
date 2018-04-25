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

//#define PIN_SPIGOT      13
#define PIN_SYRINGE      9

#define RX 11
#define TX 10

#define PIN_L_ENABLE     5
#define PIN_L_FORWARD    3
#define PIN_L_BACKWARD   4

#define PIN_R_ENABLE     6
#define PIN_R_FORWARD    7
#define PIN_R_BACKWARD   8

#define MSEC_FILLTIME 1000
//#define MSEC_OUNCE  // To be determined later
#define A_REST_SPIGOT  135
#define A_FILL_SPIGOT  -45

SoftwareSerial TankBluetooth(10, 11); // RX, TX
Servo servo_spigot;
Servo servo_syringe;

struct PixyObj {
  int signature;
  int x_pos;
  int y_pos;
  int width;
  int height;
  int angle;
};

struct Location {
  int x;
  int y;
};

// BT Variables:
int infoArray[6] = {0,0,0,0,0,0};
String incomingString;

bool dangerDetected = false;
bool elderlyDetected = false;
bool waterDetected = false;
bool eletankDetected = false;

// Distance Variables:
int szd; // Distance (Height) from Concave Corner Border to Safe Zone.
int wep; // Distance (Height) from Water to Elderly Pick Up Location.

// Elderly Signal Variables:
//int elderly_sig_out = 12; // Elderly Detection Output Signal 
//int elderly_sig_inp = 13; // Elderly Detection Inupt  Signal

PixyObj elderly = {0,0,0,0,0,0}; 
PixyObj danger = {0,0,0,0,0,0}; 
PixyObj eletank = {0,0,0,0,0,0};
PixyObj water = {0,0,0,0,0,0}; 

Location elderly_pos = {0,0}; 
Location eletank_pos = {0,0}; 
Location curr_target = {0,0}; 

// Updated after referencing core locations:
Location water_loc; 
Location pickup_loc; 
Location cc_loc; // Concave Corner
Location safezone_loc; 
Location szb_loc;  // Safe Zone Boundry 

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
}

// Using Starting Position, State Machines, and Functions / Reference Locations to Direct EleTank.
void loop() {

  if (dangerDetected==false) {
    tankStop();
    //tankDrive(255);
  }

  if (TankBluetooth.available()) {
    incomingString = TankBluetooth.readString(); 
    parsePixyInformation(incomingString);
    curr_target = pickup_loc;

    Serial.print("Current Target: (");
    Serial.print(curr_target.x);
    Serial.print(", ");
    Serial.print(curr_target.y);
    Serial.println(")");
    
    /*tankDrive(255);
    delay(500);
    tankStop();*/
    
    if (dangerDetected) {
      Serial.println("Danger Detected!");
      commandTankMovement(curr_target); // <--- Issue begins here!
    }
    
    if (!digitalRead(PIN_ELDERLY_CONNECT)) { // <--- Need a proximity function in mean time to digitally trigger pin.
      curr_target = safezone_loc;
      commandTankMovement(curr_target);
    }
  }
}

/************************************************/
/**********                         *************/
/**********     HELPER FUNCTIONS    *********** */
/**********                         *************/
/************************************************/

int getHDist(Location a, Location b) { // Note: Tank, Destination
  if (b.x - a.x < 0) {
    Serial.print("Orient Tank Right\n");
  } else {
    Serial.print("Orient Tank Left\n");
  }
  return abs(b.x - a.x);
}

int getVDist(Location a, Location b) { // Note: Tank, Destination
  if (b.y - a.y < 0) {
    Serial.print("Orient Tank Up\n");
  } else {
    Serial.print("Orient Tank Down\n");
  }
  return abs(b.x - a.x);
}

double getDistance(Location a, Location b) {
    double distance;
    distance = sqrt((a.x - b.x) * (a.x - b.x) + (a.y-b.y) *(a.y-b.y));
    return distance;
}

double getAngle1(Location a, Location b) {
  double angle = atan2(a.y - b.y, a.x - b.x);
  return (angle * 180 / PI);
}

double getAngle2(Location a, Location b) {
  double atan_var = getVDist(a,b)/getHDist(a,b);
  double angle = atan(atan_var);
  return angle;
}

int getShortestPath(Location a, Location b) { 

  int target_angle_dir = getAngle1(a,b);
  int x_dir = getHDist(a,b);
  int y_dir = getVDist(a,b);
  int total_distance = getDistance(a,b);
  String x_direct;
  String y_direct;
  
  if (b.x - a.x < 0) {
    x_direct = "Left ";
  } else {
    x_direct = "Right ";
  }

  if (b.y - a.y < 0) {
    y_direct = "Up ";
  } else {
    y_direct = "Down ";
  }

  // Tank Orients itself (does not yet drive though!)
  eletankCommandTurn(target_angle_dir);

  // Print Position / Direction Information : 
  printf("Tank must travel %d %s and %d %s OR travel %d in the %d direction. \n",x_dir, x_direct.c_str(), y_dir, y_direct.c_str(), total_distance, target_angle_dir);
  
  return total_distance; // Return the distance betweent the target and the eletank
}

char findTurnDirection(int currentDegree, int target)
{
     int diff = target - currentDegree;
     if(diff < 0)
         diff += 360;
     if(diff > 180)
          return 'L'; // left turn
     else
          return 'R'; // right turn
}

void eletankCommandTurn(int target_angle_dir) {

  Serial.println("Tank now turning!\n");
  /* Get the angular position of the tank. */
  int curr_eletank_angle = eletank.angle;   
  curr_eletank_angle = pixyAngleConvert(curr_eletank_angle);
  
  /* Determine which direction is most efficient to turn. */
  char dir_char = findTurnDirection(curr_eletank_angle, target_angle_dir);  

  /* Determine angular displacement*/
  int ang_diff = curr_eletank_angle - target_angle_dir;
  if (abs(ang_diff) > 180) {
    ang_diff = 360 - abs(ang_diff);
  }

  /* Turn until angular difference between desired position and tank posiion are negligible */
  float timeout = millis();
  while (abs(ang_diff) > 3 && (millis() - timeout) < 5000) {
    // INSERT DRIVE FORWARD FUNCTION HERE!
    tankTurn(127, dir_char); // '1' Currently place holder for speed.
    curr_eletank_angle = eletank.angle;
    curr_eletank_angle = pixyAngleConvert(curr_eletank_angle);
    ang_diff = (curr_eletank_angle - target_angle_dir)%360;
    tankStop();
  }
  return; 
}

int commandTankMovement(Location target) { // May need some threshold flag to help this function decide when it's close enough to the SZB.
  static int distance_from_target = 10000; // Dummy Value
  
  // If Target and Tank are beyond boundary...
  if (isBeyondBoundary(eletank_pos) && isBeyondBoundary(target)) {
    distance_from_target = getShortestPath(eletank_pos, target); 
  }

  // If tank isn't beyond boundary, but target is OR If tank is beyond bounary, but target isn't...
  if (!isBeyondBoundary(eletank_pos) || !isBeyondBoundary(target)) {
    getShortestPath(eletank_pos, szb_loc);
  }

  return distance_from_target;
}

void commandTankNavigate(Location target_location) {
  int dft; // Distance from Target...

  /* Find location of eletank */
  eletank_pos = {eletank.x_pos, eletank.y_pos}; // Not sure if this will work...
  
  /*Find distance from target*/
  dft = getShortestPath(eletank_pos, target_location);
  Serial.print("DFT: ");
  Serial.println(dft);
 
  while (dft > 25) { // Will Adjust Later!
    tankDrive(255);
    delay(2000);
    tankStop();
    eletank_pos = {eletank.x_pos, eletank.y_pos}; // Update location of EleTank
    dft = commandTankMovement(target_location); // Update Distance from target. 
  }
}

int pixyAngleConvert(int pixy_angle) {
  int temp_angle;
  temp_angle = pixy_angle - 90;
  temp_angle = (temp_angle) * (-1);
  if(temp_angle < 0) {
    temp_angle = 360 - temp_angle;
  }
  return temp_angle;
}

bool isBeyondBoundary(Location pos){ // Returns whether the Location position is in "Free Space" (or space beyond boundary)
  if (pos.y < szb_loc.y) {
    return false;
  } else {
    return true;
  }
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

int checkSpeed(int spd) {
  // Sanity check for speed.
  if(spd > 255) {
    return 255;
  } else if(spd <= 0) {
    Serial.println("Speed at 0");
    return 0;
  } else {
    return (int)spd;
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

void tankTurn(int spd, int dir) {
  // Continuously turn in given direction and speed.
  spd = checkSpeed(spd);
  // Checking direction.
  if(dir == 'L') {
    digitalWrite(PIN_L_FORWARD, HIGH);
    digitalWrite(PIN_L_BACKWARD, LOW);
    digitalWrite(PIN_R_FORWARD, LOW);
    digitalWrite(PIN_R_BACKWARD, HIGH);
  } else if(dir == 'R') {
    digitalWrite(PIN_L_FORWARD, HIGH);
    digitalWrite(PIN_L_BACKWARD, LOW);
    digitalWrite(PIN_R_FORWARD, LOW);
    digitalWrite(PIN_R_BACKWARD, HIGH);
  } else {
    Serial.print("Invalid direction input to tankTurn(): dir = ");
    Serial.println(dir);
    // Invalid input! Stopping tank!
    tankStop();
  }
}

void tankDrive(int spd) {
  // Drive forwards at a relative speed!
  spd = checkSpeed(spd);
  analogWrite(PIN_L_ENABLE, spd);
  analogWrite(PIN_R_ENABLE, spd);
  digitalWrite(PIN_L_FORWARD, HIGH);
  digitalWrite(PIN_L_BACKWARD, LOW);
  digitalWrite(PIN_R_FORWARD, HIGH);
  digitalWrite(PIN_R_BACKWARD, LOW);
}

void tankReverse(int spd) {
  // Drive backwards at a relative speed!
  spd = checkSpeed(spd);
  analogWrite(PIN_L_ENABLE, spd);
  analogWrite(PIN_R_ENABLE, spd);
  digitalWrite(PIN_L_FORWARD, LOW);
  digitalWrite(PIN_L_BACKWARD, HIGH);
  digitalWrite(PIN_R_FORWARD, LOW);
  digitalWrite(PIN_R_BACKWARD, HIGH);
}

// Functions handling wireless information:

void parsePixyInformation(String incomingBtString) {

    //Serial.print("Incoming String from BT: ");
    //Serial.println(incomingBtString);
  
    // Data from Bluetooth:
    int n = 0;
    char strCharArray[40];
    //while(incomingBtString[n] != '\0') {
    while(incomingBtString[n] != '\n') {
        strCharArray[n] = incomingBtString[n];
        n++;
    }

    Serial.println("\nIncoming Info: ");
    Serial.print("StrCharArray from BT: ");
    Serial.println(strCharArray);
    
    stringToIntArray(strCharArray); // Updates BT InfoArray
    Serial.print("Info Array from BT: ");
    int x;
    for (x = 0; x < 6; x++) {
      Serial.print(infoArray[x]);
      Serial.print(" ");
    }
    Serial.println("");
    
    // Update Object:
    switch (infoArray[0]) { 
      case 4: // Need to change these cases to match signatures
        updatePixyObj(&elderly, infoArray);
        elderly_pos.x = elderly.x_pos;
        elderly_pos.y = elderly.y_pos;
        if (elderlyDetected == false) {
          Serial.println("Elderly Observed!:");
          //dangerDetected = true; // Temporary
          pickup_loc = {elderly_pos.x, elderly_pos.y}; 
          cc_loc = {pickup_loc.x + 200 ,pickup_loc.y }; 
          safezone_loc = {pickup_loc.x + 400 ,pickup_loc.y - 200 }; 
          szb_loc = {pickup_loc.x + 200, pickup_loc.y };  // Safe Zone Boundry 
          elderlyDetected = true;
        }
        break;
      case 19: // Need to change these cases to match signatures
        updatePixyObj(&eletank, infoArray);
        if (eletankDetected == false) {  
          Serial.println("Eletank Observed!:");
          eletankDetected = true;
        }
        eletank_pos.x = eletank.x_pos;
        eletank_pos.y = eletank.y_pos;  
        break;   
      case 5: // Need to change these cases to match signatures
        updatePixyObj(&water, infoArray);
        if (waterDetected == false) {
          water_loc = {water.x_pos, water.y_pos}; 
          waterDetected = true;
        }
        break;
      case 1: // Need to change these cases to match signatures
        updatePixyObj(&danger, infoArray);
         if (dangerDetected == false) { 
           Serial.println("Danger Detected!");
           tankDrive(255);
           delay(2000);
           tankStop();
           dangerDetected = true;
         }
        break;
      default:
         // Do Nothing
         break;
    }
}

void stringToIntArray(char str[]) {
  
  int intArray[6] = {0, 0, 0, 0, 0, 0};
  char charArray[6] = {'0', '0', '0', '0', '0', '0'};
  int count = 0;

  // Convert String to char Array
  char *ch;
  char *rest = str;
  //ch = strtok(str, " ");
  ch = strtok_r(rest, " ", &rest);
  
  int i;
  for (i = 0; i < 6; i++) {
    if (ch != NULL) {
      charArray[i] = ch;
      /*Serial.print("StrTok Value ");
      Serial.print(i);
      Serial.print(" :");
      Serial.println(ch);*/
      infoArray[i] = atoi(ch);
      //ch = strtok(str, " ");
      ch = strtok_r(rest, " ", &rest);
      //ch = strtok(NULL, " ");
    }
  }

  // Convert char Attay to int Array
  /*for(i = 0; i < 6; i++)
  {
    infoArray[i] = atoi(charArray[i]);
  }*/

  /*Serial.print("Info Array from Str to Int Function: ");
  int x;
  for (x = 0; x < 6; x++) {
    Serial.print(infoArray[x]);
    Serial.print(" ");
  }
  Serial.println("");*/
    
    
  return;
}

void updatePixyObj(PixyObj* pxObj, int intArray[]) {
  pxObj->x_pos = intArray[1];
  pxObj->y_pos = intArray[2];
  pxObj->width = intArray[3];
  pxObj->height = intArray[4];
  pxObj->angle = intArray[5];
  return;
}








/***********************************/
/*Functions "under construction"...*/
/***********************************/

void commandTankSave(){
  // Stop Tank from moving!
  // Once the tank is within range, pick up the elderly.
}

void commandTankDeposit(){
  // Moving Spigot down to detach elderly.
  spigot(A_FILL_SPIGOT); // Currently using spigot fill angle.
  spigot(A_REST_SPIGOT); // Currently using spigot resting angle.
}

void commandTankExniguish(){
  // Stop Tank from moving!
  // Angle "Tank Hose" in position to extenguish fire.
  // Hose Fire Down by Pushin on Syringe
}

void commandTankFill(){
  spigot(A_FILL_SPIGOT);
  syringeFill();
  spigot(A_REST_SPIGOT);
}

void commandTankObstacleAvoidance() { // Use distance sensor in front to determine whether it should evade the obstacle
  // Have a set arch to deviate from linear path.  
}

/*void syringeFire(msec_time) {
  servo_syringe.write(180); // "180" for emptying syringe.
  delay(msec_time); // Spray water for specified amount of time.
  servo_syringe.write(90); // "90" to stop emptying syringe.
}*/
