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
#define PIN_L_FORWARD    6
#define PIN_L_BACKWARD   7

#define PIN_R_ENABLE     3
#define PIN_R_FORWARD    2
#define PIN_R_BACKWARD   1

#define MSEC_FILLTIME 1000
//#define MSEC_OUNCE  // To be determined later
#define A_REST_SPIGOT  135
#define A_FILL_SPIGOT  -45

Servo servo_spigot;
Servo servo_syringe;

enum State {Save_Elderly, Retrieve_Water, Extinguish_Fire};

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

PixyObj elderly;
PixyObj danger;
PixyObj eletank;
PixyObj water;

Location water_loc;
Location pickup_loc;
Location cc_loc;
Location safezone_loc;
Location szb_loc; // Safe Zone Boundry

Location elderly_pos;
Location eletank_pos;

// Important Distances:
int szd; // Distance (Height) from Concave Corner Border to Safe Zone.
int wep; // Distance (Height) from Water to Elderly Pick Up Location.

// Other Important Variables:
State curr_state;
Location curr_target;

void setup() {
  Serial.begin(9600);
  Serial.println("Begin Initialization...\n");
  
  pixy.init(); // Initialize Pixy Object

  // Initalizing Servos
  servo_spigot.attach(PIN_SPIGOT);
  servo_syringe.attach(PIN_SYRINGE);

  int nvb = 0; // Number of Viewed Blocks ("Seen Signtaures")
  while (nvb <= NPO) { // Before Initializing Structs, ensure that all signatures have been detected.
   nvb += pixy.getBlocks();
  }
  
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
  
  // Initialize Reference Locations and Relative Distances
  water_loc = {water.x_pos, water.y_pos}; 
  pickup_loc = {elderly.x_pos, elderly.y_pos};
  wep = abs(getVDist(water_loc,pickup_loc));
  cc_loc = {water.x_pos - elderly.x_pos,water.y_pos - elderly.y_pos}; // Concave Corner Location
  szd = water_loc.y_pos - (wep + water.height); // Use the Y Position of Water - ((Distance Between Elderly and Water) + Length of Body of Water)
  szb_loc = {water_loc.x_pos, water_loc.y_pos - wep};
  safezone_loc = {water_loc.x_pos, water_loc.y_pos - (wep + szd)};
  // Initialize Moving Object Positions:
  elderly_pos = {elderly.x_pos, elderly.y_pos};
  eletank_pos = {eletank.x_pos, eletank.y_pos};

  curr_state = -1;
  curr_target = eletank_pos; // Starting target is where eletank already is.
  Serial.println("Initialization Complete...\n");
}

// Using Starting Position, State Machines, and Functions / Reference Locations to Direct EleTank.
void loop() {
  
  //Update EleTank Position
  eletank_pos.x_pos = pixy.blocks[eletank.block_pos].x;
  eletank_pos.y_pos = pixy.blocks[eletank.block_pos].y;
  
  switch (curr_state) {
    case 0:
        static bool elderly_retrieved = false;
        if (!elderly_retrieved) {
          curr_target = pickup_loc;
          if (getHDist(eletank_pos, curr_target) < MAD) { // Can't assume we'll approach elderly from a Horizontal Position
            commandTankSave();
            elderly_retrieved = true;
          }
        } else {
          curr_target = safezone_loc;
          if (getHDist(eletank_pos, curr_target) < MAD) {
            commandTankDeposit();
            curr_state = Retrieve_Water;
          }
        }
        commandTankMovement(curr_target);
      break;
    case 1:
      // Retrieve Water, Not Built Yet
      break;
    case 2:
      // Put Out Fire, Not Implemented Yet
      break;
    default:
      // Do Nothing... Enventually, Keep Motors idle.
      break;
  }
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

int getShortestPath(Location a, Location b) { // May include an "Obstacle Flag? / Eventually should return "aimed for" Location.
  int atan_var = getVDist(a,b)/getHDist(a,b);
  int angle = atan(atan_var);
  int x_dir = getHDist(a,b);
  int y_dir = getVDist(a,b);
  String x_direct;
  String y_direct;
  
  if (b.x_pos - a.x_pos < 0) {
    x_direct = "Right";
  } else {
    x_direct = "Left";
  }

  if (b.y_pos - a.y_pos < 0) {
    y_direct = "Up";
  } else {
    y_direct = "Down";
  }
  
  printf("Tank must travel %d %s and %d %s OR travel in the %d direction\n",x_dir, x_direct.c_str(), y_dir, y_direct.c_str(), angle);

  return angle;
  // Eventually return Location...
}

void commandTankMovement(Location target) { // May need some threshold flag to help this function decide when it's close enough to the SZB.
  // If Target and Tank are beyond boundary...
  if (isBeyondBoundary(eletank_pos) && isBeyondBoundary(target)) {
    getShortestPath(eletank_pos, target); 
    // Above function will return angle, use this to move the tank.
  }

  // If tank isn't beyond boundary, but target is OR If tank is beyond bounary, but target isn't...
  if (!isBeyondBoundary(eletank_pos) || !isBeyondBoundary(target)) {
    getShortestPath(eletank_pos, szb_loc);
  }
}

void commandTankSave(){
  // Stop Tank from moving!
  // Once the tank is within range, pick up the elderly.
}

void commandTankDeposit(){
  // Moving Spigot down to detach elderly.
  Spigot(A_FILL_SPIGOT); // Currently using spigot fill angle.
  Spigot(A_REST_SPIGOT); // Currently using spigot resting angle.
}

void commandTankExniguish(){
  // Stop Tank from moving!
  // Angle "Tank Hose" in position to extenguish fire.
  // Hose Fire Down by Pushin on Syringe
}

void commandTankFill(){
  Spigot(A_FILL_SPIGOT);
  SyringeFill()
  Spigot(A_REST_SPIGOT);
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

void SyringeFill() {
  // Fill Syringe, assuming position-controlled servo.
  servo_syringe.write(0); // "0" for filling syringe.
  delay(MSEC_FILLTIME); // Time to fill is experimentally determined.
  servo_syringe.write(90); // "90" to stop filling syringe.
}

void SyringeFire(msec_time) {
  servo_syringe.write(180); // "180" for emptying syringe.
  delay(msec_time); // Spray water for specified amount of time.
  servo_syringe.write(90); // "90" to stop emptying syringe.
}

void Spigot(input_angle) {
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

