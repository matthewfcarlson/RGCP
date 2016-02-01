#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Adafruit_GPS.h>

//State varaibles
enum currentPuzzle {PUZZLE_WAIT, PUZZLE_ICE, PUZZLE_COCO, PUZZLE_SLAB, PUZZLE_DITTO};

//AUDIO stuff
AudioPlaySdWav           playSdWav1;     //xy=301,216
AudioControlSGTL5000     sgtl5000_1;

//GPS stuff -use Serial 2
#define GPS_RX      9
//#define GPS_SERIAL Serial2
HardwareSerial GPS_SERIAL = Serial2;
Adafruit_GPS GPS(&GPS_SERIAL);

//Screen stuff
#define TFT_DC      20
#define TFT_CS      21
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     7
#define TFT_SCLK    14
#define TFT_MISO    12
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);


void setup() {
  // put your setup code here, to run once:
  GPS.begin(9600);
  GPS_SERIAL.begin(9600);
}

void setupTFT()
{
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(Arial_24);
  //tft.setTextSize(3);
  //tft.setCursor(40, 8);
  //tft.println("Peak Meter");
}

void loop() {
  // put your main code here, to run repeatedly:

}
