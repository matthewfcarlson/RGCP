#include <Servo.h>
#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <SD.h>
#include <SerialFlash.h>
#include <TinyGPS.h>

//State varaibles
int CurrentPuzzle = 0;


//AUDIO stuff
AudioPlaySdWav           playSdWav1;     //xy=301,216
AudioControlSGTL5000     sgtl5000_1;

//GPS stuff -use Serial 2
HardwareSerial GPS_SERIAL = HardwareSerial();
TinyGPS gps;
const float deg2rad = 0.01745329251994;
const float rEarth  = 6371000.0;
float range = 2000;
boolean gpsHasFix = false;


//Screen stuff
#define TFT_DC      20
#define TFT_CS      21
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     7
#define TFT_SCLK    14
#define TFT_MISO    12
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

//SERVO stuff
Servo latchServo;   // create servo object to control a servo 
int   latchPos = 0; // variable to store the servo position 
#define SERVO_CONTROL 4
#define SERVO_ONOFF   3

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
void gpsdump(TinyGPS &gps);
void printFloat(double f, int digits = 2);


//The objectives
struct Puzzle{
  long lat;
  long lon;
  String location;
}
#define NUM_PUZZLES 3
int hintsUsed = 0;
#define NUM_HINTS = 
struct Puzzle puzzles[NUM_PUZZLES];


void setup() {
  puzzles[0].lat = 0.0;
  // put your setup code here, to run once:
  Serial.begin(115200);
  GPS_SERIAL.begin(9600);
  setupTFT();

  latchServo.attach(SERVO_CONTROL);  // attaches the servo

  pinMode(3, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  
  delay(1000);

}

void setupTFT()
{
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(Arial_12);
  //tft.setTextSize(3);
  tft.setCursor(40, 8);
  tft.println("Peak Meter");
}

bool newGPSData = false;

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

void loop() {
  // put your main code here, to run repeatedly:
 
  unsigned long start = millis();

  //if (millis() - start > 5000) 
    updateGPSData(); //Must be called every .13 seconds
  
  if (newGPSData) {
    Serial.println("Acquired Data");
    Serial.println("-------------");
    gpsdump(gps);
    Serial.println("-------------");
    Serial.println();
   
    newGPSData = false;
  }
  delay(50);
}
void gpsdump(TinyGPS &gps)
{
  long lat, lon;
  float flat, flon;
  unsigned long age, date, time, chars;
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned short sentences, failed;

  gps.get_position(&lat, &lon, &age);
  Serial.print("Lat/Long(10^-5 deg): "); Serial.print(lat); Serial.print(", "); Serial.print(lon); 
  Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");
  
  // On Arduino, GPS characters may be lost during lengthy Serial.print()
  // On Teensy, Serial prints to USB, which has large output buffering and
  //   runs very fast, so it's not necessary to worry about missing 4800
  //   baud GPS characters.
  tft.fillScreen(ILI9341_BLACK);
   tft.setCursor(40, 8);
   tft.println("Location: ");
   tft.setCursor(80, 80);
   tft.println(String(lat));
   tft.setCursor(100, 100);
   tft.println(lon);


  gps.f_get_position(&flat, &flon, &age);
  Serial.print("Lat/Long(float): "); printFloat(flat, 5); Serial.print(", "); printFloat(flon, 5);
  Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");

  gps.get_datetime(&date, &time, &age);
  Serial.print("Date(ddmmyy): "); Serial.print(date); Serial.print(" Time(hhmmsscc): ");
    Serial.print(time);
  Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");

  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  Serial.print("Date: "); Serial.print(static_cast<int>(month)); Serial.print("/"); 
    Serial.print(static_cast<int>(day)); Serial.print("/"); Serial.print(year);
  Serial.print("  Time: "); Serial.print(static_cast<int>(hour)); Serial.print(":"); 
    Serial.print(static_cast<int>(minute)); Serial.print(":"); Serial.print(static_cast<int>(second));
    Serial.print("."); Serial.print(static_cast<int>(hundredths));
  Serial.print("  Fix age: ");  Serial.print(age); Serial.println("ms.");

  Serial.print("Alt(cm): "); Serial.print(gps.altitude()); Serial.print(" Course(10^-2 deg): ");
    Serial.print(gps.course()); Serial.print(" Speed(10^-2 knots): "); Serial.println(gps.speed());
  Serial.print("Alt(float): "); printFloat(gps.f_altitude()); Serial.print(" Course(float): ");
    printFloat(gps.f_course()); Serial.println();
  Serial.print("Speed(knots): "); printFloat(gps.f_speed_knots()); Serial.print(" (mph): ");
    printFloat(gps.f_speed_mph());
  Serial.print(" (mps): "); printFloat(gps.f_speed_mps()); Serial.print(" (kmph): ");
    printFloat(gps.f_speed_kmph()); Serial.println();

  gps.stats(&chars, &sentences, &failed);
  Serial.print("Stats: characters: "); Serial.print(chars); Serial.print(" sentences: ");
    Serial.print(sentences); Serial.print(" failed checksum: "); Serial.println(failed);
}

void printFloat(double number, int digits)
{
  // Handle negative numbers
  if (number < 0.0) {
     Serial.print('-');
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;
  
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  Serial.print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0)
    Serial.print("."); 

  // Extract digits from the remainder one at a time
  while (digits-- > 0) {
    remainder *= 10.0;
    int toPrint = int(remainder);
    Serial.print(toPrint);
    remainder -= toPrint;
  }
}

//Calculates the distance to the point
float distanceTo(float lat1, float lon1, float lat2, float long2){
  float h = sq((sin(lat1-lat2) /2.0))) + (cost(lat1) * cos(lat2) * sq((sin((lon1 - lon2) /2.0))))
  float d = 2.0 * rEarth * asin (sqrt(h));
  return d;
}

