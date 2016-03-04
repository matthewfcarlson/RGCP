#include <EEPROM.h>
#include <Servo.h>
#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <SD.h>
#include <SerialFlash.h>
#include <TinyGPS++.h>


//AUDIO stuff
AudioPlaySdWav           playSdWav1;     //xy=301,216
AudioControlSGTL5000     sgtl5000_1;

//GPS stuff
HardwareSerial GPS_SERIAL = HardwareSerial();
TinyGPSPlus gps;
const float deg2rad = 0.01745329251994;
const float rEarth = 20902200; //set for feet 6371000.0 meters, 3958.75 mi, 6370.0 km, or 3440.06 NM
float range = 50;   // distance from HERE to THERE
boolean gpsHasFix = false;


//Screen stuff
#define TFT_DC      20
#define TFT_CS      21
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     7
#define TFT_SCLK    14
#define TFT_MISO    12
#define TFT_LED      6
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
struct Puzzle{
  double lat;
  double lon;
  String des;
} Puzzle;

#define PUZZLE_ADDR 42
#define HINT_ADDR 24

int hintsUsed = 0;
int currentPuzzle = 0;
#define NUM_HINTS  50
#define NUM_PUZZLES 3
struct Puzzle puzzles[NUM_PUZZLES];

bool newGPSData = false;
void setup() {
  //FIRST PUZZLE
  puzzles[0].lat = 0.0;
  puzzles[0].lon = 0.0;
  puzzles[0].des = "Cocoa Bean";

  //SECOND PUZZLE
  puzzles[1].lat = 0.0;
  puzzles[1].lon = 0.0;
  puzzles[1].des = "Cocoa Bean 2";

  //SECOND PUZZLE
  puzzles[1].lat = 0.0;
  puzzles[1].lon = 0.0;
  puzzles[1].des = "Cocoa Bean 3";
  
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
  digitalWrite(TFT_LED,HIGH);


  currentPuzzle = EEPROM.read(PUZZLE_ADDR);
  hintsUsed = EEPROM.read(HINT_ADDR);
  displayGPSWait();
  digitalWrite(TFT_LED,HIGH);
  while (!newGPSData){
    updateGPSData(); 
    //Check if our fix is valid
  }

  while (!gpsHasFix){
    updateGPSData(); 
    if (gps.location.isValid()) gpsHasFix = true;
  }
  digitalWrite(TFT_LED,LOW);
  
  lock();  

}




void setupTFT()
{
  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(Arial_12);
}
double targetLat = 0;
double targetLng = 0;
double distanceMeters = 0;
double courseTo = 0;
unsigned long start = 0;

String direction = "N/A";
//Loop assumes it just woke up
void loop() {
  
  digitalWrite(TFT_LED,LOW);
  
  while(digitalRead(HINT_BTN) == HIGH){
    tft.fillScreen(ILI9341_GREEN);
    delay(5);
  }
  digitalWrite(TFT_LED,HIGH);
  gpsHasFix = false;
  newGPSData = false;
  //unlock();
  Serial.print("Time since GPS last update:");
  Serial.println(gps.location.age());
  //Get new GPS data
  
 
  
  targetLat = puzzles[currentPuzzle].lat;
  targetLng = puzzles[currentPuzzle].lon;

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




  //Turn on the screen
  digitalWrite(TFT_LED, LOW);
  start = millis();

  //if we are in range of the target
  if (distanceMeters <= range){
    //advance to next winner
    if (currentPuzzle >= NUM_PUZZLES){
      //Show winner on screen
      //play sound
      delay(200);
      unlock();
    }
    else{
      currentPuzzle ++;
      
    }
    
  }
  else{ //use a hint
    hintsUsed++;
    displayHintInfo(distanceMeters, direction);
    //Show the hint
  }

  //update the screen
  displayGPSInfo();

  Serial.println("Acquired Data");
  Serial.println("-------------");
  //gpsdump(gps);
  Serial.println("-------------");
  Serial.println();

  //wait for 5 seconds
  //delay(5000);
  delay(2000);

  //turn of screen backlight
  digitalWrite(TFT_LED, HIGH);
  delay(500);
  //go back to sleep
  //Snooze.sleep( sleepconfig );
}

void displayGPSWait(){
  tft.setFont(Arial_32);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.println("Looking for Signal...");
  
}

void displayGPSInfo(){
  tft.setFont(Arial_24);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.println("Latitude");
  
  tft.setFont(Arial_12);
  tft.setCursor(10, 40);
  tft.println(gps.location.lat(),9);
  
  tft.setFont(Arial_24);
  tft.setCursor(0, 80);
  tft.println("Longitude");

  tft.setFont(Arial_12);
  tft.setCursor(10, 120);
  tft.println(gps.location.lng(),9);
}

void displayHintInfo(double distance, String direction){
  /*tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.println("Latitude");
  tft.setCursor(40, 40);
  tft.println(currentPuzzle);
  tft.setCursor(80, 80);
  tft.println("Longitude");
  tft.setCursor(80, 80);
  tft.println(gps.location.lng());*/
}

void unlock(){
  digitalWrite(SERVO_ON,HIGH);
  latchServo.write(90);
}
void lock(){
  digitalWrite(SERVO_ON,HIGH);
  latchServo.write(0);
}
/*
Serial.println(gps.location.lat(), 6); // Latitude in degrees (double)
Serial.println(gps.location.lng(), 6); // Longitude in degrees (double)
Serial.print(gps.location.rawLat().negative ? "-" : "+");
Serial.println(gps.location.rawLat().deg); // Raw latitude in whole degrees
Serial.println(gps.location.rawLat().billionths);// ... and billionths (u16/u32)
Serial.print(gps.location.rawLng().negative ? "-" : "+");
Serial.println(gps.location.rawLng().deg); // Raw longitude in whole degrees
Serial.println(gps.location.rawLng().billionths);// ... and billionths (u16/u32)
Serial.println(gps.date.value()); // Raw date in DDMMYY format (u32)
Serial.println(gps.date.year()); // Year (2000+) (u16)
Serial.println(gps.date.month()); // Month (1-12) (u8)
Serial.println(gps.date.day()); // Day (1-31) (u8)
Serial.println(gps.time.value()); // Raw time in HHMMSSCC format (u32)
Serial.println(gps.time.hour()); // Hour (0-23) (u8)
Serial.println(gps.time.minute()); // Minute (0-59) (u8)
Serial.println(gps.time.second()); // Second (0-59) (u8)
Serial.println(gps.time.centisecond()); // 100ths of a second (0-99) (u8)
Serial.println(gps.speed.value()); // Raw speed in 100ths of a knot (i32)
Serial.println(gps.speed.knots()); // Speed in knots (double)
Serial.println(gps.speed.mph()); // Speed in miles per hour (double)
Serial.println(gps.speed.mps()); // Speed in meters per second (double)
Serial.println(gps.speed.kmph()); // Speed in kilometers per hour (double)
Serial.println(gps.course.value()); // Raw course in 100ths of a degree (i32)
Serial.println(gps.course.deg()); // Course in degrees (double)
Serial.println(gps.altitude.value()); // Raw altitude in centimeters (i32)
Serial.println(gps.altitude.meters()); // Altitude in meters (double)
Serial.println(gps.altitude.miles()); // Altitude in miles (double)
Serial.println(gps.altitude.kilometers()); // Altitude in kilometers (double)
Serial.println(gps.altitude.feet()); // Altitude in feet (double)
Serial.println(gps.satellites.value()); // Number of satellites in use (u32)
Serial.println(gps.hdop.value()); // Horizontal Dim. of Precision (100ths-i32)
 */
void updateGPSData(){
  static int updateCount = 0;
  while (GPS_SERIAL.available()) {
      char c = GPS_SERIAL.read();
      //Serial.print(c);  // uncomment to see raw GPS data
      
      if (gps.encode(c)) {
        updateCount++;
        if (updateCount > 5){
          newGPSData = true;
          updateCount = 0;
        }
      }
    }
    
}

//Calculates the distance to the point
float distanceFromTo(double lat1, double lon1, double lat2, double lon2){
  // returns the great-circle distance between two points (radians) on a sphere
  float h = sq((sin((lat1 - lat2) / 2.0))) + (cos(lat1) * cos(lat2) * sq((sin((lon1 - lon2) / 2.0))));
  float d = 2.0 * rEarth * asin (sqrt(h)); 
  //Serial.println(d);
  return d;
  
}

//Calculates the direction from (lat1, lon1) to (lat2, lon2)
String directionFromTo(double lat1, double lon1, double lat2, double lon2){
  if (lat2 > lat1){ //North
    if (lon2 > lon1) //EAST
      return "NE";
    else
      return "NW";
  }
  else{ //South
    if (lon2 > lon1) //EAST
      return "SE";
    else
      return "SW";
    
  }
}

