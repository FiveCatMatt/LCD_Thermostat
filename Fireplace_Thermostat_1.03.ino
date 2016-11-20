#include <SPI.h>
#include <SdFat.h>
#include <UTFT.h>
#include <URTouch.h>
#include <UTFT_SdRaw.h>
#include <avr/pgmspace.h>
#include <math.h>                               // Used for thermistor value
#include <Time.h>                               // Time and date keeping this just points to TimeLib.h
#include <TimeLib.h>

extern uint8_t Ubuntu[9124];                    // Font includes
extern uint8_t SevenSegNumFontPlusPlus[2604];
extern uint8_t arial_normal[3044];
extern uint8_t arial_bold[3044];
extern uint8_t SmallFont[];



UTFT myGLCD(ILI9325D_16, 38, 39, 40, 41);       // SainSmart LCD      ** comment this stuff lazy ass
URTouch  myTouch( 6, 5, 4, 3, 2);               // SainSmart touchscreen
UTFT_SdRaw myFiles(&myGLCD);                    // Asign SD card name

#define SD_CHIP_SELECT  46                      // SD chip select pin
SdFat sd;                                       // SD card file system object
SdFile myFile;                                  // Enable file write

int x, y;                                       // Touchscreen coordinates
float clicker = 68.5;                           // TEMP - testing fonts delete when done
static char dtostrfbuffer[5];                   // Convert float to string for LCD printing
int FireAnimationCounter = 0;                   // Used to delay fireplace animation
int FontPick = 1;                               // TEMP - testing fonts delete when done
boolean FireOn = 0;                             // Signals fireplace status
int xPosGraph[3] = {0, 0, 0};                   // Counter for temperature graph
float LastGraphValue[3] = {0, 0, 0};            // Last value plotted on graph
boolean InitalizeGraph[3] = {0, 0, 0};          // Draw chart element - legend and plot area
float TemperatureStorage[40];                   // Draw decay on temperatue graph
int temperatureLoop = 0;                        // How many times the temp graph has run
int ReverseNGcount = 5;                         // dunno why this is working - y pos graph decay fix, off by 5 px on first pass

int LCDbacklight = 9;                           // Takes brightness value from below
int MotionIN = 8;                               // Motion detector, high means motion active
byte LCDbrightness = 0;                         // Sets pwm for backlight, ** backwards 0 is full bright, 255 is off **
boolean LCDlightOn = 0;                         // Off or on state used by fade off code

int hrs;                                        // Time temporary valuse - could probably be deleted
int mins;
int secs;
String mon;
int ClkDigit[4];                                // Split time in to ints
int LastDigit[4];                               // Last split time values

const String FCprefix = "FlipClk";              // Flip clock file name constants
const String FCsuffix = ".raw";

long WunderTime = 1478045640; // Value for timekeeping - to be deleted

//BEGIN Thermistor
#define ThermistorPIN 15                        // Analog Pin 15
const float a1 = 3.35E-03;                      // Values from thermistor datasheet
const float b1 = 2.57E-04;
const float c1 = 1.92E-06;
const float d1 = 1.10E-07;
float vcc = 4.9;                                // only used for display purposes, if used set to the measured Vcc.
float pad = 10000;                              // balance/pad resistor value, set this to the measured resistance of your pad resistor
float thermr = 10000;                           // thermistor nominal resistance
float temp;                                     // For thermistor value

float Thermistor(int RawADC) {
  long Resistance;
  float Temp;  // Dual-Purpose variable to save space.

  Resistance = pad * ((1024.0 / RawADC) - 1);
  Temp = log(Resistance / thermr);
  Temp = (1 / ((a1 + b1 * Temp) + (c1 * pow(Temp, 2)) + (d1 * pow(Temp, 3)))) - 273.15;
  Temp = (Temp * 9.0) / 5.0 + 32.0;                 // Convert to Fahrenheit

  return Temp; // Return the Temperature
}

//#define TempTimer (1000UL * 60)    // Thermistor sample time. Currently set at 1 min (* 60 * 5 = five mins)
#define TempTimer (1000UL * 5)    // Thermistor sample time. Currently set at 5 seconds (* 60 * 5 = five mins)
//#define TempTimer (1000UL)    // Thermistor sample time. Currently set at 1 seconds (* 60 * 5 = five mins)
//#define TempTimer (0)    // Thermistor sample time. Currently set at 0?
unsigned long rolltime = millis() + TempTimer;
// END Thermistor

void setup() {

  pinMode(MotionIN, INPUT);                       // Motion detect set pin to input
  pinMode(LCDbacklight, OUTPUT);                  // Pin to turn on display backlight 0 on - 1 off
  digitalWrite(LCDbacklight, HIGH);               // turn off backlight for boot

  // Begin LCD
  myGLCD.InitLCD(PORTRAIT);
  myGLCD.clrScr();
  myGLCD.setFont(arial_normal);
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  myGLCD.setBackColor(0, 0, 0);
  // End LCD

  // SD card start
  bool mysd = 0;
  while (!mysd)
  {
    if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) {
      myGLCD.setColor(0xFF4C);
      myGLCD.print("SD Card Failed", CENTER, 20);
    }
    else
    {
      mysd = 1;
    }
  }
  // End SD card start

  TimeKeeping(1);

  myGLCD.setBackColor(VGA_TRANSPARENT);           // Default font background color

  //myFiles.load(0, 0, 240, 320, "DNNtheme.raw");   // TEMP - Font testing BG
  myFiles.load(0, 0, 240, 320, "BG_Clock_Stork.raw");   // TEMP - testing flip clock
  
  Serial.begin(9600);
}

void Page_Home() {

  myFiles.load(10, 10, 70, 70, "UpArrow.raw");
  myFiles.load(10, 90, 70, 70, "DownArrow.raw");

  for (int w = 1; w < 240; w++) {
    myFiles.load(w, 115, 1, 30, "BlueLine.raw");
  }

  myGLCD.setFont(arial_normal);
  myGLCD.setColor(0xFF4C);
  myGLCD.print("Home Page", CENTER, 120);

  BlueButton("Font", 10, 255);                        // ButtonText, ButtonStartX, ButtonStartY

  myFiles.load(110, 150, 23, 100, "FireplaceLeft23w.raw");
  for (int w = 0; w < 54; w++) {
    myFiles.load(w + 133, 150, 1, 100, "FireplaceMid1w.raw");
  }
  myFiles.load(187, 150, 23, 100, "FireplaceRight23w.raw");
  myFiles.load(138, 194, 45, 50, "FireAniOff45w50h.raw");
}

void BlueButton(String ButtonText, int ButtonStartX, int ButtonStartY) {

  // Font is 16 x 16, image is 40px high. One char space left and right 16|16. Y spacing is 12|12 px.
  int StringLength = 0;
  int ButtonWidth = 0;                            // Will determine overall lenght of mid section
  int TextStartX = 0;                             // First x coord of the text string
  StringLength = ButtonText.length();       // Get # of chars in string
  StringLength++;

  myFiles.load(ButtonStartX, ButtonStartY, 18, 40, "ButtonLeft.raw");

  ButtonWidth = 16 * StringLength;                // Chars are 16px wide
  ButtonWidth = ButtonWidth + 32;                 // 16px spacing left and right
  TextStartX = (ButtonStartX + 18) + 12;          // Left image start + it's width (18) + 16px left spacer  ## wtf why 12? ##

  for (int w = ButtonStartX + 18; w <= ButtonWidth; w++) {
    myFiles.load(w, ButtonStartY, 1, 40, "ButtonMid.raw");
  }

  myGLCD.setFont(arial_normal);                   // 16 x 16
  myGLCD.setColor(0xFFFF);
  myGLCD.print(ButtonText, TextStartX, ButtonStartY + 12);  // 12px top spacer

  myFiles.load(ButtonWidth, ButtonStartY, 18, 40, "ButtonRight.raw");
}

void FireLoop() {
  if (FireOn) {
    FireAnimationCounter++;
    switch (FireAnimationCounter) {
      case 1:
        myFiles.load(138, 194, 45, 50, "FireAni145w50h.raw");
        break;
      case 10000:
        myFiles.load(138, 194, 45, 50, "FireAni245w50h.raw");
        break;
      case 20000:
        myFiles.load(138, 194, 45, 50, "FireAni345w50h.raw");
        break;
      case 30000:
        FireAnimationCounter = 0;
        break;
    }
  }
}

void fontLoop(int directionKey) {

  if (directionKey == 1) {
    clicker = clicker + .1;
  } else if (directionKey == 0) {
    clicker = clicker - .1;
  }
  if (clicker > 99.9) {
    dtostrf(clicker, 4, 0, dtostrfbuffer);
  } else {
    dtostrf(clicker, 4, 1, dtostrfbuffer);
  }
  dtostrf(clicker, 4, 1, dtostrfbuffer);

  switch (FontPick) {
    case 1:
      myGLCD.setFont(Ubuntu);
      myGLCD.setColor(0x0000);
      myGLCD.fillRect(90, 1, 240, 100);
      break;
    case 2:
      myGLCD.setFont(SevenSegNumFontPlusPlus);
      myGLCD.setColor(0x0000);
      myGLCD.fillRect(90, 1, 240, 100);
      break;
    case 3:
      myGLCD.setFont(arial_normal);
      myGLCD.setColor(0x0000);
      myGLCD.fillRect(90, 1, 240, 100);
      break;
    case 4:
      myGLCD.setFont(SevenSegNumFontPlusPlus);
      myGLCD.setColor(0x0000);
      myGLCD.fillRect(90, 1, 240, 100);
      break;
  }

  if (clicker < 45) {
    myGLCD.setColor(0x8E99);  // Light Blue - Coldest
  } else if (clicker > 45 && clicker < 65) {
    myGLCD.setColor(0x4433);  // Blue - Cool
  } else if (clicker > 65 && clicker < 72) {
    myGLCD.setColor(0xFF4C);  // Yellow - Perfect
  } else if (clicker > 72) {
    myGLCD.setColor(0xFB6D);  // Red - Hot
  }

  myGLCD.print(dtostrfbuffer, RIGHT, 15);

}

void GraphTemperature(int xGraphAreaStart, int yGraphAreaStart, int xGraphAreaEnd, int yGraphAreaEnd, int GraphNum, int xGraphStep, int MinValue, int MaxValue) {
  //   GraphTemperature(0, 220, 240, 320, 0, 5, 60, 100);    // xGraphAreaStart, yGraphAreaStart, xGraphAreaEnd, yGraphAreaEnd, GraphNum, xGraphStep, minValue, MaxValue

  int xChartOffset = 32;
  int yChartOffset = 20;
  int TickMarkSpacing = 2;                        // Multiplyer to space tick marks
  int CharTickMark = 6;                           // Chart tick mark size
  int yGraphStep = 10;
  float CurrentTemp;

  CharTickMark = CharTickMark / 2;

  CurrentTemp = temp;
  CurrentTemp = map(CurrentTemp, MinValue, MaxValue, 20, 100);      // Map temp values for Y axis

  if (!InitalizeGraph[GraphNum]) {                                  // If first run inialize the chart
    myGLCD.setColor(0x0000);                                        // Draw black BG behind chart
    myGLCD.fillRect(xGraphAreaStart, yGraphAreaStart, xGraphAreaEnd, yGraphAreaEnd);

    // Chart legend - only drawn at graph start   SmallFont **** Font is 8px wide and 12px high *****

    xGraphAreaEnd = xGraphAreaEnd - 8;              // TEST scale x axis

    myGLCD.setColor(VGA_GRAY);
    myGLCD.drawLine(xGraphAreaStart + xChartOffset, yGraphAreaEnd - yChartOffset, xGraphAreaEnd, yGraphAreaEnd - yChartOffset);     // Draw X axis
    for (int i = (xGraphAreaStart + xChartOffset) + (xGraphStep * TickMarkSpacing); i <= xGraphAreaEnd; i = i + (xGraphStep * TickMarkSpacing)) {  // Draw X tick marks
      myGLCD.drawLine(i, yGraphAreaEnd - (yChartOffset + CharTickMark), i, yGraphAreaEnd - (yChartOffset - CharTickMark));
    }

    myGLCD.drawLine(xGraphAreaStart + xChartOffset, yGraphAreaEnd - yChartOffset, xGraphAreaStart + xChartOffset, yGraphAreaStart); // Draw Y axis
    for (int i = yGraphAreaStart; i <= yGraphAreaEnd - yChartOffset; i = i + (yGraphStep * TickMarkSpacing)) {                       // Draw Y tick marks
      myGLCD.drawLine(xGraphAreaStart + (xChartOffset + CharTickMark), i, xGraphAreaStart + (xChartOffset - CharTickMark), i);
    }

    myGLCD.setFont(SmallFont);                                      // X axis legend
    myGLCD.setBackColor(0x0000);
    myGLCD.printNumI(40, 224, 308);
    //myGLCD.printNumI((xGraphAreaEnd), 190, 308);
    //myGLCD.printNumI((xGraphAreaStart + xChartOffset), 150, 308);
    myGLCD.print("Seconds", CENTER, 308);

    myGLCD.printNumI(60, 10, 295);                                  // Y axis legend
    myGLCD.printNumI(70, 10, 275);
    myGLCD.printNumI(80, 10, 255);
    myGLCD.printNumI(90, 10, 235);

    InitalizeGraph[GraphNum] = 1;
  }

  /*
       if (CurrentTemp < 64) {                         // Change font color based on temp range
      myGLCD.setColor(0x8E99);                      // Light Blue 0x8E99 - Coldest
    } else if (CurrentTemp >= 64 && CurrentTemp < 68) {
      myGLCD.setColor(0x4433);                      // Blue 0x4433 - Cool
    } else if (CurrentTemp >= 68 && CurrentTemp < 72) {
      myGLCD.setColor(0xFFFF);                      // White 0xFFFF - Perfect
    } else if (CurrentTemp >= 72 && CurrentTemp < 74) {
      myGLCD.setColor(0xFF4C);                      // Yellow 0xFF4C - Warm
    } else if (CurrentTemp >= 74) {
      myGLCD.setColor(0xFB6D);                      // Red 0xFB6D - Hot
    }
  */

  myGLCD.setColor(VGA_LIME); // TESTING


  if (xPosGraph[GraphNum] == xGraphAreaStart) {



    myGLCD.setColor(VGA_LIME);                      // Plot first value
    //myGLCD.drawLine(xPosGraph[GraphNum] + xChartOffset, (yGraphAreaEnd - CurrentTemp - 1), xPosGraph[GraphNum] + xChartOffset, yGraphAreaEnd - CurrentTemp); Shit code don't read

    //LastGraphValue[GraphNum] = CurrentTemp;

  } else {

    myGLCD.drawLine((xPosGraph[GraphNum] - xGraphStep) + xChartOffset, yGraphAreaEnd - LastGraphValue[GraphNum], xPosGraph[GraphNum] + xChartOffset, yGraphAreaEnd - CurrentTemp);

    //LastGraphValue[GraphNum] = CurrentTemp;
  }


  if (temperatureLoop >= 40) {                                       // Error check
    //temperatureLoop = 39;
    myGLCD.print("ERROR", 82, 280);
    //myGLCD.printNumI(temperatureLoop, 180, 280);
  }
  /*
    Serial.print("temperatureLoop = ");
    Serial.println(temperatureLoop);

    if (temperatureLoop == 0) {
    TemperatureStorage[temperatureLoop] = CurrentTemp;                // Store current temp
    Serial.print("Pre Array Num: ");
    Serial.print(temperatureLoop);
    Serial.print(" = ");
    Serial.println(TemperatureStorage[temperatureLoop]);
    temperatureLoop++;
    TemperatureStorage[temperatureLoop] = CurrentTemp;                // Store current temp
    Serial.print("Post Array Num: ");
    Serial.print(temperatureLoop);
    Serial.print(" = ");
    Serial.println(TemperatureStorage[temperatureLoop]);
    } else {
    TemperatureStorage[temperatureLoop] = CurrentTemp;                // Store current temp
    Serial.print("Array Num: ");
    Serial.print(temperatureLoop);
    Serial.print(" = ");
    Serial.println(TemperatureStorage[temperatureLoop]);
    temperatureLoop++;
    } */

  TemperatureStorage[temperatureLoop] = CurrentTemp;                // Store current temp
  LastGraphValue[GraphNum] = CurrentTemp;





  xPosGraph[GraphNum] = xPosGraph[GraphNum] + xGraphStep;           // Start over when the end of the X axis is hit
  if (xPosGraph[GraphNum] + xChartOffset >= xGraphAreaEnd - 8) {    // xGraphAreaEnd scaled for x axis to = 200



    for (int w = 0; w <= temperatureLoop; w++) {

      myGLCD.setColor(VGA_GRAY);
      //((xPosGraph[GraphNum] - xGraphStep) + xChartOffset, yGraphAreaEnd - LastGraphValue[GraphNum], xPosGraph[GraphNum] + xChartOffset, yGraphAreaEnd - CurrentTemp)
      if (w != 39) {
        myGLCD.drawLine((ReverseNGcount - xGraphStep) + xChartOffset, yGraphAreaEnd - TemperatureStorage[w], ReverseNGcount + xChartOffset, yGraphAreaEnd - TemperatureStorage[w + 1]);
      } else {
        myGLCD.drawLine((ReverseNGcount - xGraphStep) + xChartOffset, yGraphAreaEnd - TemperatureStorage[w], ReverseNGcount + xChartOffset, yGraphAreaEnd - TemperatureStorage[w]);
      }

      Serial.println(w);
      //((xPosGraph[GraphNum] - xGraphStep) + xChartOffset, yGraphAreaEnd - LastGraphValue[GraphNum], xPosGraph[GraphNum] + xChartOffset, yGraphAreaEnd - CurrentTemp)
      delay(10);
      ReverseNGcount = ReverseNGcount + xGraphStep;
    }

    ReverseNGcount = 0;
    //temperatureLoop = 0;                        // Reset temperature loop for decay




    xPosGraph[GraphNum] = xGraphAreaStart;                          // return to start of graph
  }
  temperatureLoop++;
}

void GraphInTemp(int xGraphAreaStart, int yGraphAreaStart, int xGraphAreaEnd, int yGraphAreaEnd, int xGraphStep, int GraphNum, String xLegend, int xMid, int xMax, String yLegend, int yMid, int yMax) {
  //GraphInTemp(0, 200, 240, 320, 5, 0, "Seconds", 20, 40, "Temp", 70, 80);
  int xPlot = 200;                                // Draw area x
  int yPlot = 100;                                // Draw area y
  int Spacer = 4;                                 // Space between legend text and plot area
  int TextX = 8;                                 // Size of text in px
  int TextY = 16;                                 // Size of text in px
  int xOffset = (TextX * 2) + Spacer;             // Two chars and 4px space
  int yOffset = TextY + Spacer;                   // One char and 4px space
  int xStart = xGraphAreaStart + xOffset;         // These 2 line create 0,0 for the chart data
  int yStart = yGraphAreaEnd - yOffset;
  int axisDashSpace = 5;                          // spacer for dashed lines
  boolean DrawDecay = 0;                          // Set to 1 when end of x axis is reached then the decay draw runs
  int xOld, yOld, xNew, yNew;                     // Make the draw commands a bit easier to read
  int yMin = abs((yMax - yMid) - yMid);           // Calculate min y axis value - for mapping if used
  float CurrentTemp;

  myGLCD.setFont(SmallFont);
  //myGLCD.setFont(arial_normal);


  // Draw plot lines and legend data - First run only
  if (!InitalizeGraph[GraphNum]) {
    myGLCD.setColor(VGA_BLACK);                                                 // Set chart background color and print background rectangle
    myGLCD.fillRect(xGraphAreaStart, yGraphAreaStart, xGraphAreaEnd, yGraphAreaEnd);

    myGLCD.setColor(VGA_SILVER);                                                // Set legend color
    myGLCD.print(xLegend, xStart, yStart + Spacer);                             // Print text at plot area x = 0 and y + spacer
    myGLCD.printNumI(xMid, xStart + (xPlot / 2) - TextX, yStart + Spacer);      // Funky math to get value in the middle - Only works for 2 chars modify TextX for more or less
    myGLCD.printNumI(xMax, xStart + xPlot - TextX, yStart + Spacer);            // Funky math to get max value - Only works for 2 chars modify TextX for more or less

    myGLCD.print(yLegend, xGraphAreaStart, yStart, 270);             // Print text at plot area x = 0 and y = 0 then rotate 270 degrees
    myGLCD.printNumI(yMid, xGraphAreaStart, yStart - (yPlot / 2) - (TextY / 2));// These two lines have a max of 2 chars also will break out of chart area
    myGLCD.printNumI(yMax, xGraphAreaStart, yStart - yPlot - (TextY / 2));      // ensure chart area has at least 100px + half of char height free space

    for (int i = 0; i < xPlot; i = i + axisDashSpace) {
      if (i % 2) {                                                              // Modulo will return a zero for even numbers
        // Do something odd
        myGLCD.setColor(VGA_BLACK);                                             // Set to background color and print y axis center line - creates dashes
        myGLCD.drawLine(xStart + i, yStart - (yPlot / 2), (xStart + i) + axisDashSpace, yStart - (yPlot / 2));
      } else {
        // Do something even
        myGLCD.setColor(VGA_SILVER);                                            // Set to legend color and print y axis center line
        myGLCD.drawLine(xStart + i, yStart - (yPlot / 2), (xStart + i) + axisDashSpace, yStart - (yPlot / 2));
      }
    }

    for (int i = 0; i <= yPlot; i = i + axisDashSpace) {
      if (i % 2) {                                                              // Modulo will return a zero for even numbers
        // Do something odd
        myGLCD.setColor(VGA_BLACK);                                             // Set to background color and print x axis center line - creates dashes
        myGLCD.drawLine(xStart + (xPlot / 2), yStart - i, xStart + (xPlot / 2), (yStart - i) - axisDashSpace);
      } else {
        // Do something even
        myGLCD.setColor(VGA_SILVER);                                            // Set to legend color and print x axis center line
        myGLCD.drawLine(xStart + (xPlot / 2), yStart - i, xStart + (xPlot / 2), (yStart - i) - axisDashSpace);
      }
    }

    InitalizeGraph[GraphNum] = 1;
  }

  // Plot data
  CurrentTemp = map(temp, 60, 80, 0, 100);                                      // Map temp values for Y axis
  TemperatureStorage[temperatureLoop] = CurrentTemp;                            // Store current temp
  if (TemperatureStorage[temperatureLoop] > 100) {
    TemperatureStorage[temperatureLoop] = 100;
  } else if (TemperatureStorage[temperatureLoop] < 0) {
    TemperatureStorage[temperatureLoop] = 0;
  }
  //TemperatureStorage[temperatureLoop] = temp;                // Store current temp
  //TemperatureStorage[temperatureLoop] = map(TemperatureStorage[temperatureLoop],

  myGLCD.setColor(VGA_LIME);                                                    // Set plot line color

  if (!xPosGraph[GraphNum]) {                                       // If zero print a single pixel instead of draw line below
    xNew = xStart + xPosGraph[GraphNum];
    yNew = yStart - TemperatureStorage[temperatureLoop];
    myGLCD.drawPixel(xNew, yNew);
  } else {                                                          // Draw line in chart area from last posistion to current
    xOld = xStart + (xPosGraph[GraphNum] - xGraphStep);
    yOld = yStart - TemperatureStorage[temperatureLoop - 1];
    xNew = xStart + xPosGraph[GraphNum];
    yNew = yStart - TemperatureStorage[temperatureLoop];
    myGLCD.drawLine(xOld, yOld, xNew, yNew);
  }

  if ((xPosGraph[GraphNum] + xGraphStep) > xPlot) {
    DrawDecay = 1;
  } else {
    xPosGraph[GraphNum] = xPosGraph[GraphNum] + xGraphStep;       // Increase x posistion by the step value
    temperatureLoop++;                                            // Increase array counter
  }


  // End of x axis - plot delay if needed
  if (DrawDecay) {                                                // If DrawDecay is true redraw graph in new color (grey) and reset x posistion counter
    int xCounter = xPosGraph[GraphNum];                           // Save x posistion as it will be set to zero twice in this if statement - becomes x posistion loop counter

    xPosGraph[GraphNum] = 0;

    for (int i = 0; i <= temperatureLoop; i++) {
      myGLCD.setColor(VGA_GRAY);                                        // Set plot line color

      if (!xPosGraph[GraphNum]) {                                       // If zero print a single pixel instead of draw line below
        xNew = xStart + xPosGraph[GraphNum];
        yNew = yStart - TemperatureStorage[i];
        myGLCD.drawPixel(xNew, yNew);
      } else {                                                          // Draw line in chart area from last posistion to current
        xOld = xStart + (xPosGraph[GraphNum] - xGraphStep);
        yOld = yStart - TemperatureStorage[i - 1];
        xNew = xStart + xPosGraph[GraphNum];
        yNew = yStart - TemperatureStorage[i];
        myGLCD.drawLine(xOld, yOld, xNew, yNew);
      }

      xPosGraph[GraphNum] = xPosGraph[GraphNum] + xGraphStep;           // Increase x posistion by the step value

    }
    xPosGraph[GraphNum] = 0;
    DrawDecay = 0;
    temperatureLoop = 0;
  }

}

void ReadTemperature() {

  float AVGreading;                                 // Sample ADC 25 times then average to smooth result
  for (int GetADC = 0; GetADC < 25; GetADC++) {
    AVGreading = analogRead(ThermistorPIN) + AVGreading;
  }
  AVGreading = AVGreading / 25;

  float tempTemp;
  tempTemp = temp;                                // Store last value to skip update if no change
  temp = Thermistor(AVGreading);                  // Sends sampled ADC value to function above and returns temp in F

  // Graph temperature value
  //GraphTemperature(0, 220, 240, 320, 0, 5, 60, 100);    // xGraphAreaStart, yGraphAreaStart, xGraphAreaEnd, yGraphAreaEnd, GraphNum, xGraphStep, CurrentTemp, minValue, MaxValue
  // ** working ** GraphInTemp(0, 150, 240, 320, 5, 0, "Secs", 20, 40, "Temp", 70, 80);
  // Graph temperature value

  // Begin SD card 
  /*
  if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) {
    sd.initErrorHalt();
  }

  if (!myFile.open("Temp.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt();
  }

  myFile.println(temp);

  myFile.close();
  */
  // End SD card


  if (tempTemp != temp) {
    if (temp > 99.9) {                            // Adjust decimal if over 3 digits ie > 100
      dtostrf(temp, 4, 0, dtostrfbuffer);
    } else {
      //dtostrf(temp, 4, 1, dtostrfbuffer);         // testing DNN theme
      dtostrf(temp, 2, 0, dtostrfbuffer);         // testing DNN theme
    }

    myGLCD.setFont(Ubuntu);
    if (temp < 64) {                                // Change font color based on temp range
      myGLCD.setColor(0x8E99);                      // Light Blue 0x8E99 - Coldest
    } else if (temp >= 64 && temp < 68) {
      myGLCD.setColor(0x4433);                      // Blue 0x4433 - Cool
    } else if (temp >= 68 && temp < 72) {
      myGLCD.setColor(0xFFFF);                      // White 0xFFFF - Perfect
    } else if (temp >= 72 && temp < 74) {
      myGLCD.setColor(0xFF4C);                      // Yellow 0xFF4C - Warm
    } else if (temp >= 74) {
      myGLCD.setColor(0xFB6D);                      // Red 0xFB6D - Hot
    }

    /* removed to test DNN theme
        myFiles.load(105, 255, 135, 40, "Moon135x40.raw");     // TEMP - Font testing BG
        myGLCD.print(dtostrfbuffer, 120, 260);
        myGLCD.drawCircle(225, 263, 3);
    */

    //temp = int(temp);
    //String thisString = String(temp);
    
    //myFiles.load(160, 110, 64, 30, "BGtemp64x30.raw");     // TEMP - Font testing BG
    //myGLCD.print(dtostrfbuffer, 173, 115);
    //myGLCD.drawCircle(208, 118, 2);
    
    myGLCD.setBackColor(0x0000);
    myGLCD.setFont(arial_normal);
    myGLCD.print(dtostrfbuffer, 5, 290);
    myGLCD.drawCircle(40, 293, 2);
  }
}

void Page_FontTest() {

  /*
    for (int w = 1; w < 240; w++) {
    myGLCD.drawBitmap(w, 115, 1, 30, "BlueLine.raw");
    }

    myGLCD.setColor(0xFFFF);

    myGLCD.setFont(SevenSegNumFontPlusPlus);
    myGLCD.print("5:45", CENTER, 10);

    myGLCD.setFont(arial_normal);
    myGLCD.print("Wednesday", CENTER, 70);
    myGLCD.print("10.19.2016", CENTER, 95);


    myGLCD.setFont(arial_normal);
    myGLCD.setColor(0xFFFF);
    myGLCD.print("Wed", LEFT, 0);
    myGLCD.print("10.19", RIGHT, 0);

    myGLCD.setFont(SevenSegNumFontPlusPlus);
    myGLCD.print("5:45", CENTER, 20);
  */
}

void FlipClock() {
  String FCfileS;
  char FCfileC[13];

  for (int i = 3; i >= 0; i--) {
    if (ClkDigit[i] != LastDigit[i]) {      // Values don't match so run image update
      LastDigit[i] = ClkDigit[i];           // Set last val to current val
      // Draw  bitmap
      switch (i) {
        case 0:   // hours MSB
          FCfileS = FCprefix + ClkDigit[i] + FCsuffix;
          FCfileS.toCharArray(FCfileC, 13);
          myFiles.load(8, 8, 52, 79, FCfileC);
          Serial.println(FCfileC);
          break;
        case 1:   // hours LSB
          FCfileS = FCprefix + ClkDigit[i] + FCsuffix;
          FCfileS.toCharArray(FCfileC, 13);
          myFiles.load(63, 8, 52, 79, FCfileC);
          Serial.println(FCfileC);
          break;
        case 2:   // mins MSB
          FCfileS = FCprefix + ClkDigit[i] + FCsuffix;
          FCfileS.toCharArray(FCfileC, 13);
          myFiles.load(125, 8, 52, 79, FCfileC);
          Serial.println(FCfileC);
          break;
        case 3:   // mins LSB
          FCfileS = FCprefix + ClkDigit[i] + FCsuffix;
          FCfileS.toCharArray(FCfileC, 13);
          myFiles.load(180, 8, 52, 79, FCfileC);
          Serial.println(FCfileC);
          break;
      }

      for (int w = 0; w <= 3; w++) {        // Debug via serial
        Serial.print(ClkDigit[w]);
      }
      Serial.println(" ");
    }
  }
}

void TimeKeeping(boolean TimeSetIn) {
  if (!TimeSetIn) {
    // Prosses time updates - currently also displays
    hrs = hour();
    if (hrs > 12) hrs = hrs - 12;           // Disables military time
    mins = minute();
    secs = second();

    //hrs = mins;                             // shifting to test display
    //mins = secs;

    Serial.print(hrs);                      // Debug via serial
    Serial.print(":");
    Serial.println(mins);

    int LoopCnt = 4;

    if (mins < 10) {
      ClkDigit[2] = 0;
      ClkDigit[3] = mins;
      LoopCnt = 2;
    } else {
      while (mins > 0) {
        LoopCnt--;
        int digit = mins % 10;
        mins /= 10;
        //print digit
        ClkDigit[LoopCnt] = digit;
      }
    }

    if (hrs < 10) {
      ClkDigit[0] = 0;
      ClkDigit[1] = hrs;
    } else {
      while (hrs > 0) {
        LoopCnt--;
        int digit = hrs % 10;
        hrs /= 10;
        //print digit
        ClkDigit[LoopCnt] = digit;
      }
    }
    FlipClock();

    /*
        myGLCD.setBackColor(0x4433);
        myGLCD.setColor(0xFFFF);
        myGLCD.setFont(arial_bold);           // 16 x 16 font
        //myGLCD.print("5:45", CENTER, 10);


      int corr = 8;
      if (hrs < 10) {
        myGLCD.print(" ", 120 - corr, 160);
        myGLCD.printNumI(hrs, 136 - corr, 160);
      } else {
        myGLCD.printNumI(hrs, 120 - corr, 160);
      }

      myGLCD.print(":", 152 - corr, 160);
      if (mins < 10) {
        myGLCD.print("0", 168 - corr, 160);
        myGLCD.printNumI(mins, 184 - corr, 160);
      } else {
        myGLCD.printNumI(mins, 168 - corr, 160);
      }

      myGLCD.print(":", 200 - corr, 160);
        if (secs < 10) {
        myGLCD.print("0", 216 - corr, 160);
        myGLCD.printNumI(secs, 232 - corr, 160);
      } else {
        myGLCD.printNumI(secs, 216 - corr, 160);
      }

      CurMonInt = month();

      switch (month()) {
        case 1:
        mon = "January";
        break;
        case 2:
        mon = "February";
        break;
        case 3:
        mon = "March";
        break;
        case 4:
        mon = "April";
        break;
        case 5:
        mon = "May";
        break;
        case 6:
        mon = "June";
        break;
        case 7:
        mon = "July";
        break;
        case 8:
        mon = "August";
        break;
        case 9:
        mon = "September";
        break;
        case 10:
        mon = "October";
        break;
        case 11:
        mon = "November";
        break;
        case 12:
        mon = "December";
        break;
      }
      myGLCD.print(mon);
      myGLCD.print(" ");
      myGLCD.print(day());
      myGLCD.print(", ");
      myGLCD.print(year()); */
  } else {
    // Set time sub
    WunderTime = WunderTime - (60 * 60 * 6);
    setTime(WunderTime);
    for (int w = 0; w <= 3; w++) {                    // Set last display to 9999 to reset clock images
      LastDigit[w] = 9;
    }
  }
}

void loop() {

  //Page_Home();
  //BlueButton("Font", 10, 255);                        // ButtonText, ButtonStartX, ButtonStartY

  while (true) {

    FireLoop();                                             // Fireplace animation
    Page_FontTest();
    TimeKeeping(0);

    if (digitalRead(MotionIN) == 1) {                       // Check for motion and turn on LCD if so
      if (!LCDlightOn) {
        //analogWrite(LCDbacklight, LCDbrightness);
        for (int w = 255; w >= LCDbrightness; w--) {        // Fade in display when motion is active
          analogWrite(LCDbacklight, w);                     // 0 is on
          delay(2);
        }
        LCDlightOn = 1;
      }
    } else {
      if (LCDlightOn) {
        for (int w = 100; w <= 255; w++) {                  // Fade out display when no motion is active
          analogWrite(LCDbacklight, w);                     // 255 is off
          delay(10);
        }
        LCDlightOn = 0;
      }
    }

    if ((long)(millis() - rolltime) >= 0) {
      ReadTemperature();                                    // Thermistor reading and print
      rolltime += TempTimer;
    }

    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();

      if ((x >= 10) && (x <= 80) && (y >= 10) && (y <= 80)) { // Button: 1

        //fontLoop(1);
        LCDbrightness = LCDbrightness - 10;
        LCDlightOn = 0;
        delay(100);

      }

      if ((x >= 90) && (x <= 160) && (y >= 10) && (y <= 80)) { // Button: 2

        //fontLoop(0);
        LCDbrightness = LCDbrightness + 10;
        LCDlightOn = 0;
        delay(100);

      }

      if ((x >= 210) && (x <= 310) && (y >= 10) && (y <= 80)) { // Button: 3

        FireOn = !FireOn;
        FontPick++;
        if (FontPick > 4) FontPick = 1;
        delay(500);
        if (!FireOn) {
          myFiles.load(138, 194, 45, 50, "FireAniOff45w50h.raw");
        }

      }
    }
  }
}
