// NOTE: Signature guides probably deprecated.
// General Signature Guides:
// Purple (Elderly): Signature One
// Red (Fire/Danger): Signature Two
// Blue (Water): Signature Three
// Green/Yellow (Eletank): CC Signature Four/Five

#include <SPI.h>
#include <Pixy.h>
#include <Servo.h>

#define NPO 4 // Number of Pixy Objects
#define PWT 20 // Pixy Width Threshold (To Reliably Initialize Correct PixyObj Struct using Correct Image)
#define MAD 25 // Minumum distance between eletank and target before it can perform a eletank-to-object action (save elder, get water).

#define PIN_SPIGOT      11
#define PIN_SYRINGE      9

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

Servo servo_spigot;
Servo servo_syringe;

struct PixyObj {
  int width;
  int height;
  int x_pos;
  int y_pos;
  int block_pos; // Pixy Block Array Position
};

struct Location {
  int x_pos;
  int y_pos;
};

Pixy pixy; // Create Pixy Object

//PixyObj elderly; // bacon
//PixyObj danger; // bacon
PixyObj eletank;
//PixyObj water; // bacon

//Location water_loc; // bacon
//Location pickup_loc; // bacon
//Location cc_loc; // bacon
//Location safezone_loc; // bacon
//Location szb_loc; // Safe Zone Boundry // bacon

// Location elderly_pos; // bacon
Location eletank_pos;

// Important Distances:
int szd; // Distance (Height) from Concave Corner Border to Safe Zone.
int wep; // Distance (Height) from Water to Elderly Pick Up Location.

// Other Important Variables:
// State curr_state; // bacon
Location curr_target;

// Elderly Signal Variables:

int elderly_sig_out = 12; // Elderly Detection Output Signal 
int elderly_sig_inp = 13; // Elderly Detection Inupt  Signal

void setup() {
  Serial.begin(9600);
  Serial.println("Begin Initialization...\n");
  
  pixy.init(); // Initialize Pixy Object

  // Initalizing Servos:
  servo_spigot.attach(PIN_SPIGOT);
  servo_syringe.attach(PIN_SYRINGE);

  // Initalizing Pins:
  pinMode(PIN_L_ENABLE, OUTPUT);
  pinMode(PIN_L_FORWARD, OUTPUT);
  pinMode(PIN_L_BACKWARD, OUTPUT);
  pinMode(PIN_R_ENABLE, OUTPUT);
  pinMode(PIN_R_FORWARD, OUTPUT);
  pinMode(PIN_R_BACKWARD, OUTPUT);
//  pinMode(elderly_sig_out, OUTPUT); // bacon
//  pinMode(elderly_sig_inp, INPUT); // bacon
//  digitalWrite(elderly_sig_out, HIGH); // bacon

//  int nvb = 0; // Number of Viewed Blocks ("Seen Signtaures") // bacon
//  while (nvb <= NPO) { // Before Initializing Structs, ensure that all signatures have been detected. // bacon
//   nvb += pixy.getBlocks(); // bacon
//  } // bacon

/*  // bacon
  // Initialize PixyObj Structs
  int block_pos ;
  for (int block_pos = 0; block_pos < NPO; block_pos++) {
    
    // Wait for All Signatures to have good resolution:
    while (pixy.blocks[block_pos].width < PWT);

    // Collect Info on Currently Observed Signature:
    int pixy_sig = pixy.blocks[block_pos].signature;
    int w = pixy.blocks[block_pos].width;
    int h = pixy.blocks[block_pos].height;
    int x = pixy.blocks[block_pos].x;
    int y = pixy.blocks[block_pos].y;
    // Initialize New Object using above data:
    switch (pixy_sig) { 
      case 0:
        elderly = { w, h, x, y, block_pos};
        break;
      case 1:
        danger = {w, h, x, y, block_pos};
        break;
      case 2:
        eletank = {w, h, x, y, block_pos};
        break;
      case 3:
        water = {w, h, x, y, block_pos};
        break;
      default:
         // Do Nothing
         break;
    }
  }
  */  // bacon
  // For Testing:
  int pixy_sig = pixy.blocks[block_pos].signature;
  int eletank_sig = 0; // Place signature no. of eletank here!
  if (pixy_sig == eletank_sig){
     eletank = {w, h, x, y, block_pos};
  }
  //End
  
  // Initialize Reference Locations and Relative Distances
  /* bacon
  //water_loc = {water.x_pos, water.y_pos}; 
  //pickup_loc = {elderly.x_pos, elderly.y_pos};
  //wep = abs(getVDist(water_loc,pickup_loc)); // Distance (Height) from Water to Elderly Pick Up Location.
  //cc_loc = {water.x_pos - elderly.x_pos,water.y_pos - elderly.y_pos}; // Concave Corner Location
  //szd = elderly.y_pos - water.width; // Use the Y Position of Water - ((Distance Between Elderly and Water) + Length of Body of Water)
  //safezone_loc = {water_loc.x_pos, szd}; // Point to aim to drop off the elderly
  */
  
  // Initialize Moving Object Positions:
  //elderly_pos = {elderly.x_pos, elderly.y_pos};
  eletank_pos = {eletank.x_pos, eletank.y_pos};

  curr_target = eletank_pos; // Starting target is where eletank already is.
  Serial.println("Initialization Complete...\n");
}

// For testing
int target_angle;
// Using Starting Position, State Machines, and Functions / Reference Locations to Direct EleTank.
void loop() {
  
  //Update EleTank Position with each run of the main loop.
  Serial.println("Turning to 30 deg");
  eletankCommandTurn(30);
  Serial.println("Turning to 90 deg");
  eletankCommandTurn(90);
  Serial.println("Turning to -30 deg");
  eletankCommandTurn(-30);
  Serial.println("Turning to 30 deg");
  eletankCommandTurn(30);
  Serial.println("Turning to -30 deg");
  eletankCommandTurn(-30);
  Serial.println("Turning to 90 deg");
  eletankCommandTurn(90);

  /* Bacon
  curr_target = pickup_loc;
  if (getHDist(eletank_pos, curr_target) < MAD) { // Can't assume we'll approach elderly from a Horizontal Position
    while (digitalRead(elderly_sig_inp) == LOW) {
      commandTankSave();
    }
  }*/

  //curr_target = safezone_loc;v// bacon
  //commandTankMovement(curr_target);//bacon

  // Consider adding an interrupt for re-retrieving the elderly!
  
  /* Bacon
   * if (getHDist(eletank_pos, curr_target) < MAD) {
    commandTankDeposit();
  }*/
        
  // Retrieve Water...
  // Put Out Fire...
  // Quench Elderly...
  // Wait Forever...
}

int getHDist(Location a, Location b) { // Note: Tank, Destination
  if (b.x_pos - a.x_pos < 0) {
    Serial.print("Right");
  } else {
    Serial.print("Left");
  }
  return abs(b.x_pos - a.x_pos);
}

int getVDist(Location a, Location b) {
  if (b.y_pos - a.y_pos < 0) {
    Serial.print("Up");
  } else {
    Serial.print("Down");
  }
  return abs(b.x_pos - a.x_pos);
}

double getDistance(Location a, Location b) {
    double distance;
    distance = sqrt((a.x_pos - b.x_pos) * (a.x_pos - b.x_pos) + (a.y_pos-b.y_pos) *(a.y_pos-b.y_pos));
    return distance;
}

double getAngle1(Location a, Location b) {
  double angle = atan2(a.y_pos - b.y_pos, a.x_pos - b.x_pos);
  return (angle * 180 / PI);
}

double getAngle2(Location a, Location b) {
  double atan_var = getVDist(a,b)/getHDist(a,b);
  double angle = atan(atan_var);
  return angle;
}

int getShortestPath(Location a, Location b) { // May include an "Obstacle Flag? // Eventually should return "aimed for" Location. // May need to be "smarter"

  int target_angle_dir = getAngle1(a,b);
  int x_dir = getHDist(a,b);
  int y_dir = getVDist(a,b);
  int total_distance = getDistance(a,b);
  String x_direct;
  String y_direct;
  
  if (b.x_pos - a.x_pos < 0) {
    x_direct = "Left";
  } else {
    x_direct = "Right";
  }

  if (b.y_pos - a.y_pos < 0) {
    y_direct = "Up";
  } else {
    y_direct = "Down";
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
  
  /* Get the angular position of the tank. */
  int curr_eletank_angle = pixy.blocks[eletank.block_pos].angle;   
  curr_eletank_angle = pixyAngleConvert(curr_eletank_angle);
  
  /* Determine which direction is most efficient to turn. */
  char dir_char = findTurnDirection(curr_eletank_angle, target_angle_dir);  

  /* Determine angular displacement*/
  int ang_diff = curr_eletank_angle - target_angle_dir;
  if (abs(ang_diff) > 180) {
    ang_diff = 360 - abs(ang_diff);
  }

  /* Turn until angular difference between desired position and tank posiion are negligible */
  while (abs(ang_diff) > 3) {
    // INSERT DRIVE FORWARD FUNCTION HERE!
    tankTurn(255, dir_char); // '1' Currently place holder for speed.
    curr_eletank_angle = pixy.blocks[eletank.block_pos].angle;
    curr_eletank_angle = pixyAngleConvert(curr_eletank_angle);
    ang_diff = (curr_eletank_angle - target_angle_dir)%360;
  }
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

bool isBeyondBoundary(Location pos){ // Returns whether the Location position is in "Free Space" (or space beyond boundary)
  if (pos.y_pos < szb_loc.y_pos) {
    return false;
  } else {
    return true;
  }
}

void commandTankNavigate(Location target_location) {
  int dft; // Distance from Target...

  /* Find location of eletank */
  eletank_pos = {eletank.x_pos, eletank.y_pos}; // Not sure if this will work...
  
  /*Find distance from target*/
  dft = getShortestPath(eletank_pos, target_location);
 
  while (dft < MAD) {
    // INSERT DRIVE FORWARD FUNCTION HERE!
    delay(1000);
    eletank_pos = {eletank.x_pos, eletank.y_pos}; // Update location of EleTank
    dft = commandTankMovement(target_location); // Update Distance from target. 
  }
}

/***********************************/
/*Functions "under construction"...*/
/***********************************/

int pixyAngleConvert(int pixy_angle) {
  int temp_angle;
  temp_angle = pixy_angle - 90;
  temp_angle = (temp_angle) * (-1);
  if(temp_angle < 0) {
    temp_angle = 360 - temp_angle;
  }
  return temp_angle;
}

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
  syringeFill()
  spigot(A_REST_SPIGOT);
}

bool isBeyondBoundary(Location pos){ // Returns whether the Location position is in "Free Space" (or space beyond boundary)
  if (pos.y_pos < szb_loc.y_pos) {
    return false;
  } else {
    return true;
  }
}

void commandTankObstacleAvoidance() { // Use distance sensor in front to determine whether it should evade the obstacle
  // Have a set arch to deviate from linear path.  
}

void syringeFill() {
  // Fill Syringe, assuming position-controlled servo.
  servo_syringe.write(0); // "0" for filling syringe.
  delay(MSEC_FILLTIME); // Time to fill is experimentally determined.
  servo_syringe.write(90); // "90" to stop filling syringe.
}

void syringeFire(msec_time) {
  servo_syringe.write(180); // "180" for emptying syringe.
  delay(msec_time); // Spray water for specified amount of time.
  servo_syringe.write(90); // "90" to stop emptying syringe.
}

void spigot(input_angle) {
  // Sanity check!
  if(typeof(input_angle) != int) {
    Serial.println("Spigot() takes integers! (it can handle floats though)");
  }
  int angle = (int)input_angle
  if(angle > 90) {
    servo_spigot.write(180);
  } else if(angle < (-90)) {
    servo_spigot.write(0);
  } else {
    servo_spigot.write(angle);
  }
}

int checkSpeed(spd) {
  // Sanity check for speed.
  if(spd > 255) {
    return 255;
  } else if(spd <= 0) {
    Serial.println("You know you set turning speed to 0 right?");
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

void tankTurn(spd, dir) {
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

void tankDrive(spd) {
  // Drive forwards at a relative speed!
  spd = checkSpeed(spd);
  analogWrite(PIN_L_ENABLE, spd);
  analogWrite(PIN_R_ENABLE, spd);
  digitalWrite(PIN_L_FORWARD, HIGH);
  digitalWrite(PIN_L_BACKWARD, LOW);
  digitalWrite(PIN_R_FORWARD, HIGH);
  digitalWrite(PIN_R_BACKWARD, LOW);
}

void tankReverse(spd) {
  // Drive backwards at a relative speed!
  spd = checkSpeed(spd);
  analogWrite(PIN_L_ENABLE, spd);
  analogWrite(PIN_R_ENABLE, spd);
  digitalWrite(PIN_L_FORWARD, LOW);
  digitalWrite(PIN_L_BACKWARD, HIGH);
  digitalWrite(PIN_R_FORWARD, LOW);
  digitalWrite(PIN_R_BACKWARD, HIGH);
}

