#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_pinIO.h>

#define NUM_DIR 8

//Globals, things that cannot be stored locally. Might change when refactoring or when passing params is too slow.
//Time stuff
unsigned long lastPoll; //Last time we updated the LEDs
unsigned long lastDisPoll; 
unsigned long time; //Current time
unsigned long rotCWTime; //Last CW rotation detected time
unsigned long rotCCWTime; //Last CCW rotation detected time
unsigned long writeTime; //Time at which we detected that we want to write the settings to memory
unsigned long pushTime;
unsigned long lastLEDRefresh;
unsigned long ledRefreshRate = 50;
unsigned long endTime; //When the central LED should turn off

byte mode; //Which setting to change with a valid range between 0 and modes - 1. Pls no negatives thx.
int northMax; //Max amplitude detected by the north mic
int eastMax; //Max amplitude detected by the east mic
int southMax; //Max amplitude detected by the south mic
int westMax; //Max amplitude detected by the west mic
byte firstLED[NUM_DIR];
byte secondLED[NUM_DIR];
byte thirdLED[NUM_DIR];
bool buttonPushed;
byte lastDir; //Last direction that we output
bool settingChanged; //Whether (and which) setting has changed. Default value 255, if anything different then nothing has changed
const byte noise = 140; //the amount which we say is the very minimun of an actual sound input
int batterySaved;
bool loud; //Is there a loud sound?
byte loudBright; //Current brightness of the "loud" LED
int detectTime; //Not actually time but eh deal with it


//Settings stuff
byte lowerBound; //The threshold of sounds that we consider to be a sound as opposed to noise. Varies from 0 to 254.
byte upperBound; //The threshold of what is configured as a loud sound. Varies (after transformation) from 256 to 511.
byte soundLength; //In seconds, ranging from 0 to 255

//Constants, things that don't change during execution
const short pollTime = 100; //Polltime in milliseconds (ms) with a valid range between 0 and 32,767. Don't go over and don't go negative pls, thanks.
const short disPollTime = 400; //Time between display updates in ms with a valid range between 0 and 32,767. Don't go over and don't go negative pls, thanks.
const int wTime = 60000; //Time needed to wait until we want to write something to memory in ms. Valid values range between 0 and 255
const byte modes = 3; //The number of settings we can change. May not be larger than 255  though so many settings don't fit in memory either. 
const byte sensLowerBound = 3; //Address where lower sensitivity bound is stored
const byte sensUpperBound = 4; //Address where upper sensitivity bound is stored
const byte soundLengthAdress = 5; //Address where length of sound is stored
const byte maxVolDiff = 75; //Maximum difference between two volumes such that they are both considered as loud sounds
const byte NUMLED = 25; //Amount of LEDs we have
const byte centralLed = 24;

//Pin assignments
const byte northMic = A0;
const byte eastMic = A1;
const byte southMic = A3;
const byte westMic = A4;
const byte batteryPin = A5;

const byte rotPinA = 10;
const byte rotPinB =11;
const byte buttonPin = 12;

const int rsPinDisplay = 4;
const int ePinDisplay = 5;
const int d4PinDisplay = 6;
const int d5PinDisplay = 7;
const int d6PinDisplay = 8;
const int d7PinDisplay = 9;

const byte ledPin = 2;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMLED, ledPin, NEO_GRB + NEO_KHZ800); //Class needed for the LED strip to work
hd44780_pinIO lcd(rsPinDisplay, ePinDisplay, d4PinDisplay, d5PinDisplay, d6PinDisplay, d7PinDisplay); //Class needed for the Display to work

void setup(){
    //Read values from memory
    lowerBound = EEPROM.read(sensLowerBound);
    upperBound = EEPROM.read(sensUpperBound);
    soundLength = EEPROM.read(soundLengthAdress);
  
    if (lowerBound == 255 || lowerBound < noise) {
        lowerBound = noise;
    }
  
    if (upperBound == 255) {
        upperBound = 0;
    }
  
    if (soundLength == 255) {
        soundLength = 1; //Sound of length 0 seconds is weird, therefore initialise it as 1
    }
  
    //Code to initialise rotary encoder
    pinMode(rotPinA, INPUT_PULLUP);
    pinMode(rotPinB, INPUT_PULLUP);
    pinMode(buttonPin, INPUT_PULLUP);
  
    //Code to initialise mics
    pinMode(northMic, INPUT);
    pinMode(eastMic, INPUT);
    pinMode(southMic, INPUT);
    pinMode(westMic, INPUT);
    pinMode(batteryPin, INPUT);

    //Code to initialise display
    lcd.begin(16, 2);

    //Code to initialise LEDs
    //pinMode(ledPin, OUTPUT);
    strip.begin(); //Initialise LED strip
    strip.show(); //Turn off all LEDs

    //Needed to communicate with pc
    Serial.begin(9600);

    //Initialise global variables
    lastPoll = 0; 
    lastDisPoll =0;
    mode = 0;
  
    northMax = 0;
    eastMax = 0;
    southMax = 0;
    westMax = 0;
  
    buttonPushed = false;
    rotCWTime = 0;
    rotCCWTime = 0;
    pushTime = 0;
    lastDir = 255;
    settingChanged = false;
  
    lastLEDRefresh = 0;
    batterySaved = 10000;
  
    attachInterrupt(digitalPinToInterrupt(buttonPin), changeMode, FALLING);
    attachInterrupt(digitalPinToInterrupt(rotPinA), checkRotation, FALLING);
}

void loop() {
    time = millis();
    //Get data from mics
    getMicData();

    if (abs(time - lastPoll) > pollTime){
        //Determine direction
        int dir = calculateDirection();

        //Change LEDs
        setLED(dir);

        resetMicData();

        lastPoll  = millis() ;
    }
  
    if(abs(time - lastLEDRefresh) > (10*soundLength)){
        changeLED();
        lastLEDRefresh = time;
    }

    if (buttonPushed) {
        buttonPushed = false; //Reset pushed state and change the mode
        changeMode();
    }

    if (settingChanged && abs(time - writeTime) > wTime) {
        EEPROM.update(sensLowerBound, lowerBound);
        EEPROM.update(sensUpperBound, upperBound);
        EEPROM.update(soundLengthAdress, soundLength);
        settingChanged = false;
    }
    
    if (abs(time - lastDisPoll) > disPollTime) {
        updateDisplay();
        lastDisPoll = millis();
    }
}

void updateDisplay() {
    
    int batteryRaw = analogRead(batteryPin);
    int batteryPerc = map(batteryRaw, 714, 900, 0, 100);

    if (batteryPerc <= 20) {
        updateBattery(batteryPerc);   
    } else {
        lcd.clear();
        batterySaved = 10000;
        switch(mode) {
          case 0:
            lcd.print("Lower Sens:");
            lcd.setCursor(0, 1);
            lcd.print(map(lowerBound, noise, 254, 0, 100));
            
            break;
          case 1:
            lcd.print("Upper Sens:");
            lcd.setCursor(0, 1);
            lcd.print(map(upperBound, 0, 254, 0, 100));
            
            break;
          case 2:
            lcd.print("Sound Length:");
            lcd.setCursor(0, 1);
            lcd.print(soundLength);
            
            break;
        }
    }
}

void updateBattery(int batteryPerc) {
    if (batteryPerc < 1) {
        batteryPerc = 1;
    }
    
    if (batteryPerc < batterySaved) {
        lcd.clear();
        batterySaved = batteryPerc;
        lcd.print("Please Charge");
        lcd.setCursor(0, 1);
        lcd.print("Battery: ");
        lcd.print(batterySaved); 
        lcd.print("%");
    }
}


void getMicData() {
    int avg = 512;
    int north = abs(analogRead(northMic) - avg);
    int south = abs(analogRead(southMic) - avg);
    int east = abs(analogRead(eastMic) - avg);
    int west = abs(analogRead(westMic) - avg);

    if (north > northMax && north > lowerBound) {
        northMax = north;
    }
    
    if (south > southMax && south > lowerBound) {
        southMax = south;
    } 
    if (east > eastMax && east > lowerBound) {
        eastMax = east;
    }
    
    if (west > westMax && west > lowerBound) {
        westMax = west;
    }
    
    
}

void resetMicData(){
    northMax = 0;
    eastMax = 0;
    southMax = 0;
    westMax = 0;
}

int calculateDirection() {
    //Calculate the direction given the data from the mics, outpute a value between 0 (north) and 7 (north west), following the compass in the N, E, S, W order
    int neDiff = abs(northMax - eastMax);
    int nwDiff = abs(northMax - westMax);
    int seDiff = abs(southMax - eastMax);
    int swDiff = abs(southMax - westMax);
  
    if (neDiff < maxVolDiff && neDiff > westMax && neDiff > southMax) {
        return 1;
    } else if (seDiff < maxVolDiff && seDiff > westMax && seDiff > northMax) {
        return 3;
    } else if (swDiff < maxVolDiff && swDiff > eastMax && swDiff > northMax) {
        return 5;
    } else if (nwDiff < maxVolDiff && nwDiff > eastMax && nwDiff > southMax) {
        return 7;
    } else if (northMax > southMax && northMax > eastMax && northMax > westMax) {
        return 0;
    } else if (eastMax > southMax && eastMax > northMax && eastMax > westMax) {
        return 2;
    } else if (southMax > northMax && southMax > eastMax && southMax > westMax) {
        return 4;
    } else if (westMax > southMax && westMax > eastMax && westMax > northMax) {
        return 6;
    } else {
        return -1;
    }
}

void setLED(int dir){
    switch(dir){
        case 0:
            firstLED[dir] = 100;
        
            if (northMax > (((upperBound + 255) - lowerBound) / 3) + lowerBound){
                secondLED[dir] = 100;
            }
        
            if (northMax > (((upperBound + 255) - lowerBound) / 3)*2 + lowerBound) {
                thirdLED[dir] = 100;
            }

            if (northMax > (upperBound + 255)) {
                enableCentral();
            }
        
            break;
        case 1:
            int NE;
        
            if (northMax > eastMax) {
                NE  = northMax;
            } else {
                NE = eastMax;
            }
        
            firstLED[dir] = 100;

            if (NE > (((upperBound + 255) - lowerBound) / 3) + lowerBound) {
                secondLED[dir] = 100;
            }
        
            if (NE > (((upperBound + 255) - lowerBound) / 3)*2 + lowerBound) {
                thirdLED[dir] = 100;
            }

            if (NE > (upperBound + 255)) {
                enableCentral();
            }
        
            break;
        
        case 2:
            firstLED[dir] = 100;
        
            if (eastMax > (((upperBound + 255) - lowerBound) / 3) + lowerBound){
                secondLED[dir] = 100;
            }
        
            if (eastMax > (((upperBound + 255) - lowerBound) / 3)*2 + lowerBound) {
                thirdLED[dir] = 100;
            }

            if (eastMax > (upperBound + 255)) {
                enableCentral();
            }
        
            break;
        
        case 3:
            int SE;
        
            if (southMax > eastMax) {
                SE  = southMax;
            } else {
                SE = eastMax;
            }
        
            firstLED[dir] = 100;

            if (SE > (((upperBound + 255) - lowerBound) / 3) + lowerBound) {
                secondLED[dir] = 100;
            }
        
            if (SE > (((upperBound + 255) - lowerBound) / 3)*2 + lowerBound) {
                thirdLED[dir] = 100;
            }

            if (SE > (upperBound + 255)) {
                enableCentral();
            }
        
            break;
        
        case 4:
            firstLED[dir] = 100;
        
            if (southMax > (((upperBound + 255) - lowerBound) / 3) + lowerBound){
                secondLED[dir] = 100;
            }
        
            if (southMax > (((upperBound + 255) - lowerBound) / 3)*2 + lowerBound) {
                thirdLED[dir] = 100;
            }

            if (southMax > (upperBound + 255)) {
                enableCentral();
            }
        
            break;
        
        case 5:
            int SW;
        
            if (southMax > westMax) {
                SW  = southMax;
            } else {
                SW = westMax;
            }
        
            firstLED[dir] = 100;

            if (SW > (((upperBound + 255) - lowerBound) / 3) + lowerBound) {
                secondLED[dir] = 100;
            }
        
            if (SW > (((upperBound + 255) - lowerBound) / 3)*2 + lowerBound) {
                thirdLED[dir] = 100;
            }

            if (SW > (upperBound + 255)) {
                enableCentral();
            }
        
            break;
        
        case 6:
            firstLED[dir] = 100;
        
            if (westMax > (((upperBound + 255) - lowerBound) / 3) + lowerBound){
                secondLED[dir] = 100;
            }
        
            if (westMax > (((upperBound + 255) - lowerBound) / 3)*2 + lowerBound) {
                thirdLED[dir] = 100;
            }

            if (westMax > (upperBound + 255)) {
                enableCentral();
            }
        
            break;
        
         case 7:
            int NW;
        
            if (northMax > westMax) {
                NW  = northMax;
            } else {
                NW = westMax;
            }
        
            firstLED[dir] = 100;
           
            if (NW > (((upperBound + 255) - lowerBound) / 3) + lowerBound) {
                secondLED[dir] = 100;
            }
        
            if (NW > (((upperBound + 255) - lowerBound) / 3)*2 + lowerBound) {
                thirdLED[dir] = 100;
            }

            if (NW > (upperBound + 255)) {
                enableCentral();
            }
        
            break;
    }
}

void enableCentral() {
    if (!loud) {
        detectTime = 50;
    }
    endTime = millis() + (soundLength * 1000);
    loud = true;
}

void changeLED() {
    strip.clear();
    for (byte i = 0; i < sizeof(firstLED); i++) {
        strip.setPixelColor(i*3, strip.Color(0,0,firstLED[i]*2));
        strip.setPixelColor(i*3 + 1, strip.Color(0,0,secondLED[i]*2));
        strip.setPixelColor(i*3 + 2, strip.Color(0,0,thirdLED[i]*2));
    }
    
    for (byte i = 0; i < sizeof(firstLED); i++) {
        if (firstLED[i] > 0) {
            firstLED[i] = firstLED[i] - 1;
        }

        if (secondLED[i] > 0) {
            secondLED[i] = secondLED[i] - 1;
        }

        if (thirdLED[i] > 0) {
            thirdLED[i] = thirdLED[i] - 1;
        }
    }
    

    int factor = 0;
    if (loud) {
        if (endTime > millis()) {
            detectTime+=1*soundLength;
            int tmp = (detectTime%101) -50;
            factor = abs(tmp);
        } else {
            loud = false;
        }
    }

    strip.setPixelColor(centralLed, factor*5, 0, 0);
    
    strip.show();
}


void changeMode() {
    if(abs(pushTime-time) < 500){
        return;
    }
    mode++;
    if(mode >= modes){
        mode = 0;
    }
    pushTime=millis();
}

void checkRotation() {
    int valA = digitalRead(rotPinA);
    int valB = digitalRead(rotPinB);
  
    if (valA == valB && (time > rotCWTime + 50)) {
        increase();
        rotCWTime = time;
    }
  
    if (valA != valB && (time > rotCCWTime + 50)) {
        decrease();
        rotCCWTime = time;
    }

}

void increase() {
    writeTime = wTime + millis();
    switch(mode) {
        case 0:
            if (lowerBound < 254) {
                lowerBound += 2;
                settingChanged = true;
            }

            break;

        case 1:
            if (upperBound != 254) {
                upperBound += 2;
                settingChanged = true;
            }

            break;

        case 2:
            if (soundLength != 15) {
                soundLength++;
                settingChanged = true;
            }

            break;
    }
}

void decrease() {
    writeTime = wTime + millis();
    switch(mode) {
        case 0:
            if (lowerBound > noise ) {
                lowerBound -= 2;
                settingChanged = true;
            }

            break;

        case 1:
            if (upperBound != 0) {
                upperBound -= 2;
                settingChanged = true;
            }

            break;

        case 2:
            if (soundLength != 1) {
                soundLength--;
                settingChanged = true;
            }

            break;
    }
}
