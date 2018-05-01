#include <SoftwareSerial.h>
#include <Pixy.h>
#include <TPixy.h>

#define maxBlocks  8
#define minBlockArea 400

String incomingString;
Pixy pixCamera;

// Pixy Global Variables:
int numBlocks = 0;
int currBlock; // Block currently viewed by Pixy.
String currLocationInfo; // Current Information for viewed Block

// Important Current Block Variables (Needed for Filtering)
int currBlockArea = 0;
int currBlockSignature = 0;

 // Filter Variables:
int sig_array[] = {4, 5, 6, 19}; // Signatures we're working with
//int sig_minArea[] = {400, 10, 100, 500}; // Arbitary values to be adjusted later.
int sig_minArea[20] = {0};
int block_cursor = 0; 

// Pixy Block Object Arrays:
int eletankInfo[6];
int elderlyInfo[6];
int waterInfo[6];
int fireInfo[6];

// Transmission Data:
int transmissionData[6] = {0, 0, 0, 0, 0, 0};

SoftwareSerial pixBluetooth(11, 10); // RX, TX

void setup() {
  pixCamera.init();
  pixBluetooth.begin(9600);
  Serial.begin(9600);
  
  // Set Up Min Areas:
  sig_minArea[4] = 400;
  sig_minArea[5] = 10;
  sig_minArea[6] = 100;
  sig_minArea[19]= 500;
}

void loop() {
  int elapsedTime = millis();
  
  collectAllPixyInfo();
  getTargetDirections(elderlyInfo);
  getTargetDirections(waterInfo);
  getTargetDirections(fireInfo);
  pixBluetooth.println(getDirections());
  
  while (millis() - elapsedTime < 1000) {
    // Wait for at least one second to pass before "re-entering" main loop.
  }
}

// VERSION 2 OF COMMUNICATION:

void collectAllPixyInfo() {
  int tempBlockSig, tempBlockArea;
  boolean targetObserved[20] = {false}; // Checks whether it's already populated the correct array.
  for (currBlock = 0; currBlock < numBlocks ; currBlock++) { 
    tempBlockSig = pixCamera.blocks[currBlock].signature; // Obatin Current Block's Signature
    tempBlockArea = pixCamera.blocks[currBlock].x * pixCamera.blocks[currBlock].y; // Obtain Current Block's Area
    if (targetObserved[block_cursor] == false && isValidSignature(tempBlockSig)) {
      if (tempBlockArea >= sig_minArea[block_cursor]) {  
        switch(block_cursor) {
          case 4:
            populatePixyBlockArray(elderlyInfo, currBlock);
          break;
          
          case 5:
            populatePixyBlockArray(waterInfo, currBlock);
          break;
          
          case 6:
            populatePixyBlockArray(fireInfo, currBlock);
          break;
          
          case 19:
            populatePixyBlockArray(eletankInfo, currBlock);
          break;
          
          default:
          break;
        }
        targetObserved[block_cursor] = true;
      }
    }
  }
}

boolean isValidSignature(int sig) {
  if (sig == 4 || sig == 5 || sig == 6 || sig == 19) {
    return true; 
  } else {
    return false;
  }
}

void populatePixyBlockArray(int* pixyObjArray, int blockVal) {
  pixyObjArray[0] = pixCamera.blocks[blockVal].signature; // Signature
  pixyObjArray[1] = pixCamera.blocks[blockVal].x;         // X Posiiton
  pixyObjArray[2] = pixCamera.blocks[blockVal].y;         // Y Position
  pixyObjArray[3] = pixCamera.blocks[blockVal].width;     // Block Width
  pixyObjArray[4] = pixCamera.blocks[blockVal].height;    // Block Height
  pixyObjArray[5] = pixCamera.blocks[blockVal].angle;     // Angle
  return;
}

String getDirections() { // Using updated Pixy Information, Generates the Transmission String (Containing Angles/Distance)
  char directionsInfo[40];
  int i = 0;
  int index = 0;
  for (i=0; i < 6; i++) {
    index += snprintf(&directionsInfo[index], 40-index, "%d ", transmissionData[i]);
  }
  return directionsInfo;
}

int getTargetDirections(int* pixyObjTarget) {
  
  // Get Relative Position of Target:
  int quadrantNo = 0; // Note: Q1:Top Right, Q2:Bottom Right, Q3:Bottom Left, Q4: Top Left
  if (pixyObjTarget[2] >= eletankInfo[2]) {
    if (pixyObjTarget[1] >= eletankInfo[1]) { // Top, Right Quadrant
      quadrantNo = 1;
    } else { // Top, Left Quadrant
      quadrantNo = 4;
    }
  } else {
    if (pixyObjTarget[1] >= eletankInfo[1]) { // Bottom, Right Quadrant
      quadrantNo = 2;
    } else { // Bottom, Left Quadrant
      quadrantNo = 3;
    }
  }
  
  // Convert Angular Position of Tank to 360 Coordinate System:
  // (Starting at 0 Deg North and Clockwise)
  int tankAngularPos; 
  if (eletankInfo[5] < 0) {
    tankAngularPos = eletankInfo[5]*(-1);
  } else {
    tankAngularPos = (180-eletankInfo[5]) + 180;
  }
   
  // Determine Angular Position of Target & Convert to 360 Coordinate System:
  double relativeAngle = ((atan2(eletankInfo[2] - pixyObjTarget[2], eletankInfo[1] - pixyObjTarget[1]))*180)/PI;
  double targetAnglePos;
  if (quadrantNo == 1 || quadrantNo == 2) {
    targetAnglePos = 90 - relativeAngle;
  } else {
    targetAnglePos = 180 + (90 - relativeAngle);
  }

  if (targetAnglePos == 0  || eletankInfo[2] < 80) { // If Not in Open Space
    targetAnglePos = 360;
  }
  
  // Determine Angular Difference (Eletank Angle - Target Angle):
  // (Negative Difference = Right Turn, Positive Difference = Left Turn)
  
  int angularDifference = tankAngularPos - targetAnglePos;
  if (angularDifference >= 180) {
    angularDifference-=360;
  } else if (angularDifference < -180) {
    angularDifference+=360;
  }

  // Get Distance Between Target :
  int x_diff = eletankInfo[1]-pixyObjTarget[1];
  int y_diff = eletankInfo[2]-pixyObjTarget[2];
  double distance = sqrt((x_diff) * (x_diff) + (y_diff) *(y_diff));
  int travel_time = 0; // In Seoncds
  if (distance > 250) {
     travel_time = 3.5;
  } else if (distance < 250 && distance > 150) {
    travel_time = 2.5;
  } else if (distance < 150 && distance > 100) {
    travel_time = 1.5;
  } else if (distance < 100 && distance > 50) {
    travel_time = .75;
  } else if (distance < 25) {
    travel_time = 0;
  }

  if (eletankInfo[2] < 80) { // If Not in Open Space
    travel_time = 1.5;
  }

  // Use Proportional Control constant to either determine number of 
  // seconds to drive or to determine enable value (?)
  
  // Store Angular Difference in Array:
  
  if (pixyObjTarget[0] == 4) { // If Observing Elderly
    transmissionData[0] = angularDifference;
    transmissionData[1] = travel_time; 
  } else if (pixyObjTarget[0] == 5) { // If Observing Water
    transmissionData[2] = angularDifference;
    transmissionData[3] = travel_time;  
  } else if (pixyObjTarget[0] == 6) { // If Observing Fire
    transmissionData[4] = angularDifference;
    transmissionData[5] = travel_time; 
  }

  return;
}
