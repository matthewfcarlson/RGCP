#include <EEPROM.h>
#include <Servo.h>
#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <SD.h>
#include <SerialFlash.h>
#include <TinyGPS++.h>

//GPS stuff
HardwareSerial GPS_SERIAL = HardwareSerial();
TinyGPSPlus gps;
const float deg2rad = 0.01745329251994;
const float rEarth = 20902200; //set for feet 6371000.0 meters, 3958.75 mi, 6370.0 km, or 3440.06 NM
#define MILES_TO_FEET 5280.0
#define METERS_TO_FEET 3.28084
#define RANGE  60   //60 meters by default - not great accuracy but eh
boolean gpsHasFix = false;


//Screen stuff
#define TFT_DC      20
#define TFT_CS      21
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     7
#define TFT_SCLK    14
#define TFT_MISO    12
#define TFT_LED      6
#define TFT_HEIGHT  320
#define TFT_WIDTH   240
#define TFT_CX      TFT_WIDTH / 2
#define TFT_CY      TFT_HEIGHT / 2

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

//SERVO stuff
Servo latchServo;   // create servo object to control a servo
int   latchPos  = 0; // variable to store the servo position
#define SERVO_CONTROL 4
#define SERVO_ON  5


//Front button
#define UNLOCK_SWITCH 1
#define HINT_BTN   3

//EEPROM
#define DATA_YEAR 1
#define DATA_MONTH 2
#define DATA_DAY 3

#define DATA_HOUR 4
#define DATA_MINUTE 5


//Time keeping
unsigned minute = 0;
unsigned hour = 0;
unsigned day = 0;
unsigned month = 0;
unsigned year = 0;
void printFloat(double f, int digits = 2);


//The objectives
struct Puzzle {
  double lat;
  double lon;
  String des;
  double range;
} Puzzle;

#define PUZZLE_ADDR 42
#define HINT_ADDR 24

int hintsUsed = 0;
int currentPuzzle = 0;
#define NUM_HINTS  50
#define NUM_PUZZLES 4
struct Puzzle puzzles[NUM_PUZZLES];

bool newGPSData = false;
void setup() {

  //FIRST PUZZLE 40.245026, -111.646285
  puzzles[0].lat = 40.245026;
  puzzles[0].lon = -111.646285;
  puzzles[0].des = "JDawgs";
  puzzles[0].range = RANGE; //use the default range

  //SECOND PUZZLE 40.244503, -111.651225
  puzzles[1].lat = 40.244503;
  puzzles[1].lon = -111.651225;
  puzzles[1].des = "Duck Pond";
  puzzles[1].range = RANGE; //use the default range

  //THIRD PUZZLE
  //40.245026, -111.646285
  puzzles[2].lat = 40.238986969;
  puzzles[2].lon = -111.645584106;
  puzzles[2].des = "Home";
  puzzles[2].range = RANGE; //use the default range

  //FOURTH PUZZLE 40.234725, -111.637950
  //40.187421, -111.639565
  puzzles[3].lat = 40.234725;
  puzzles[3].lon = -111.637950;
  puzzles[3].lat = 40.1874215;
  puzzles[3].lon = -111.6399565;
  
  puzzles[3].des = "Random place";
  puzzles[3].range = 50000;//RANGE;

  // put your setup code here, to run once:
  Serial.begin(115200);
  GPS_SERIAL.begin(9600);
  Serial.print("Starting up!");
  setupTFT();

  latchServo.attach(SERVO_CONTROL);  // attaches the servo
  unlock();

  pinMode(HINT_BTN, INPUT_PULLUP);
  pinMode(UNLOCK_SWITCH, INPUT_PULLUP);
  pinMode(TFT_LED, OUTPUT);
  pinMode(SERVO_ON, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  digitalWrite(SERVO_ON, LOW);


  displayGPSWait();
  
  
  while (!newGPSData) {
    updateGPSData();
    //Check if our fix is valid
  }

  while (!gpsHasFix) {
    updateGPSData();
    if (gps.location.isValid()) gpsHasFix = true;
  }

  //make sure the screen is off
  digitalWrite(TFT_LED, LOW);

}


/*
 * Set up the screen
 */

void setupTFT()
{
  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(Arial_12);
}

/*
 * Vars I'll need for the loop
 */
double range = RANGE;
double targetLat = 0;
double targetLng = 0;
double distanceMeters = 0;
double courseTo = 0;
unsigned long start = 0;
boolean firstTime = true;

String direction = "N/A";
//Loop assumes it just woke up
void loop() {

  digitalWrite(TFT_LED, LOW);

  while (digitalRead(HINT_BTN) == HIGH) {
    delay(50); // we must call check every .13 seconds
    checkGPSData();
    if (digitalRead(UNLOCK_SWITCH) == LOW){
      delay(50);
      if (digitalRead(UNLOCK_SWITCH) == LOW) {
        unlock();
      }
    }
  }

  //Start up loop again
  
  gpsHasFix = false;
  newGPSData = false;
  //Get new GPS data
  while (!gps.location.isUpdated()) {
    updateGPSData();
    //Check if our fix is valid
  }


  //Get the current coordinates
  targetLat = puzzles[currentPuzzle].lat;
  targetLng = puzzles[currentPuzzle].lon;
  range = puzzles[currentPuzzle].range;

  distanceMeters =
    TinyGPSPlus::distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      targetLat,
      targetLng);

  courseTo =
    TinyGPSPlus::courseTo(
      gps.location.lat(),
      gps.location.lng(),
      targetLat,
      targetLng);

  direction = TinyGPSPlus::cardinal(courseTo);

  //Show the puzzle screen if this is the first time they've pressed the button
  if (firstTime){
    displayPuzzleNew();
    firstTime = false;
    delay(4000);
  }

  start = millis();

  //if we are in range of the target
  if (distanceMeters <= range) {
    //advance to next winner
    
    if (currentPuzzle >= NUM_PUZZLES - 1) {
      //Show winner on screen
      displayWinner();
      delay(2000);
      //unlock box
      unlock();
      //We don't need to do anything else after this
      digitalWrite(TFT_LED, LOW);
      while (1);
      
    }
    else {
      //wish us congrats
      displayPuzzleSolved(puzzles[currentPuzzle].des);
      currentPuzzle ++;
      delay(3000);
      displayPuzzleNew();
      //
    }

  }
  //Otherwise out of range
  else if (hintsUsed < NUM_HINTS) { //use a hint
    hintsUsed++;
    //Show the hint
    displayHintInfo(distanceMeters, direction);
    
  }
  else { //we've run out of hints
    int maxPuzzles = NUM_PUZZLES - 1;
    if (maxPuzzles == currentPuzzle) {
      displayHintInfo(distanceMeters, direction);
    }
    else {
      currentPuzzle = maxPuzzles;
      displayOutOfHint();
    }
  }
  


  //wait for 5 seconds
  delay(5000);

  //turn off screen backlight
  digitalWrite(TFT_LED, LOW);
  //go back to sleep
}


/*
 * Display functions
 */

void displayGPSWait() {
  tft.setFont(Arial_28);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 70);
  tft.println("Doing Magic");
  tft.setCursor(20, TFT_CY+50);
  tft.setFont(Arial_16);
  tft.println("Please take outside");
  
  digitalWrite(TFT_LED, HIGH);

}

void displayGPSInfo() {
  tft.setFont(Arial_24);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.println("Latitude");

  tft.setFont(Arial_12);
  tft.setCursor(10, 40);
  tft.println(gps.location.lat(), 9);

  tft.setFont(Arial_24);
  tft.setCursor(0, 80);
  tft.println("Longitude");

  tft.setFont(Arial_12);
  tft.setCursor(10, 120);
  tft.println(gps.location.lng(), 9);
  
  digitalWrite(TFT_LED, HIGH);
}

void displayOutOfHint(){
  tft.setFont(Arial_32);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(TFT_CX-70, 40);
  tft.println("OH NO!");

  tft.setFont(Arial_16);
  tft.setCursor(10, 120);
  tft.println("You're out of hints!       We'll take you to the ending right away. Sorry about that :(");
  digitalWrite(TFT_LED, HIGH);
  delay(4000);
}

void displayHintInfo(double distance, String direction) {
  tft.fillScreen(ILI9341_BLACK);
  //Show the direction
  tft.setFont(Arial_48);
  tft.setCursor(TFT_CX - stringWidth(direction,48)/2, TFT_CY);
  tft.println(direction);

  int feetTo = distance * METERS_TO_FEET;
  double milesTo = feetTo / MILES_TO_FEET;
  tft.setFont(Arial_32);
  //tft.print(distance);
  tft.setCursor(40, 40);
  if (feetTo < 3000){
    tft.print(feetTo);
    tft.print(" ft");
  }
  else{
    tft.print(milesTo,2);
    tft.print(" mi");
  }
  //
  tft.setCursor(60, TFT_HEIGHT - 30);
  tft.setFont(Arial_16);
  tft.print("Hint ");
  tft.print(hintsUsed);
  tft.print(" of ");
  tft.print(NUM_HINTS);
  digitalWrite(TFT_LED, HIGH);
}

int stringWidth(String s, int size){
  return s.length() * (size);
}

void displayPuzzleSolved(String des){
  tft.fillScreen(ILI9341_BLACK);
  //Show the direction
  tft.setFont(Arial_40);
  tft.setCursor(0, 28);
  tft.println("Welcome");
  tft.setCursor(48, 80);
  tft.print("to the");

  tft.setFont(Arial_24);
  tft.setCursor(TFT_CX - stringWidth(des,24)/2, TFT_CY+20);
  tft.print(des);

  digitalWrite(TFT_LED, HIGH);
}

void displayPuzzleNew(){
  tft.fillScreen(ILI9341_BLACK);
  //Show the puzzle text
  tft.setFont(Arial_32);
  tft.setCursor(48, 48);
  tft.println("Puzzle");

  //show the puzzle number
  tft.setFont(Arial_72);
  tft.setCursor(TFT_CX - 36, TFT_CY);
  tft.print(currentPuzzle + 1);

  digitalWrite(TFT_LED, HIGH);
}

void displayWinner(){
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(Arial_48);
  tft.setCursor(TFT_CX - 105, 30);
  tft.print("I LOVE");
  tft.setCursor(TFT_CX - 80, TFT_CY - 30);
  tft.print("YOU");
  tft.setCursor(TFT_CX - 105, TFT_HEIGHT - 70);
  tft.print("ELLEN");
  digitalWrite(TFT_LED, HIGH);
}
/*
 * Locking and unlocking
 */
void unlock() {
  digitalWrite(SERVO_ON, HIGH);
  latchServo.write(90);
  delay(15);
  latchServo.write(90);
  delay(1000);
   
  digitalWrite(SERVO_ON,LOW);
}
void lock() {
  digitalWrite(SERVO_ON, HIGH);
  latchServo.write(0);
  delay(1000);
   
  digitalWrite(SERVO_ON,LOW);
}
/*
Updates the GPS Data
*/
void updateGPSData() {
  static int updateCount = 0;
  while (GPS_SERIAL.available()) {
    char c = GPS_SERIAL.read();
    //Serial.print(c);  // uncomment to see raw GPS data

    if (gps.encode(c)) {
      updateCount++;
      if (updateCount > 5) {
        newGPSData = true;
        updateCount = 0;
      }
    }
  }

}

void checkGPSData() {
  while (GPS_SERIAL.available()) {
    char c = GPS_SERIAL.read();
    gps.encode(c);
  }
}

