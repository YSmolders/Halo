#include <EEPROM.h>
#include <FastLED.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_pinIO.h>

//Default memory usage: 112 bytes + the two classes.

//Globals, things that cannot be stored locally. Might change when refactoring or when passing params is too slow.
//Time stuff
unsigned long lastPoll; //Last time we updated the LEDs
unsigned long lastDisPoll; 
unsigned long time; //Current time
unsigned long rotCWTime; //Last CW rotation detected time
unsigned long rotCCWTime; //Last CCW rotation detected time
unsigned long writeTime; //Time at which we detected that we want to write the settings to memory
unsigned long pushTime; //Time when we last detected that the button was pushed.

//Time stuff for the directions, records the last time we detected sound coming from that direction
unsigned long turnedOnTime[8]; //When the lights for that direction turned on
unsigned long loudDetectTime; //Last time since we detected a loud sound, used for turning off the loud sound indicator after x seconds (new setting?)

byte mode; //Which setting to change with a valid range between 0 and modes - 1. Pls no negatives thx.
int northMax; //Max amplitude detected by the north mic
int eastMax; //Max amplitude detected by the east mic
int southMax; //Max amplitude detected by the south mic
int westMax; //Max amplitude detected by the west mic
bool buttonPushed;
byte settingChanged; //Whether (and which) setting has changed. Default value 255, if anything different then nothing has changed
const byte noise = 145; //The value gotten from the mics that we consider noise

//Settings stuff
byte lowerBound; //The threshold of sounds that we consider to be a sound as opposed to noise. Varies from 0 to 254.
byte upperBound; //The threshold of what is configured as a loud sound. Varies (after transformation) from 256 to 511.
byte soundLength; //In seconds, ranging from 0 to 255

//Constants, things that don't change during execution
const short pollTime = 100; //Polltime in milliseconds (ms) with a valid range between 0 and 32,767. Don't go over and don't go negative pls, thanks.
const short disPollTime = 400; //Time between display updates in ms with a valid range between 0 and 32,767. Don't go over and don't go negative pls, thanks.
const short wTime = 2000; //Time needed to wait until we want to write something to memory in ms. Valid values range between 0 and 32,767
const byte onTime = 1; //How long we leave a LED on after it has last detected a sound in seconds. Valid range from 1 to 255

const byte modes = 3; //The number of settings we can change. May not be larger than 255  though so many settings don't fit in memory either. 
const byte sensLowerBound = 3; //Address where lower sensitivity bound is stored
const byte sensUpperBound = 4; //Address where upper sensitivity bound is stored
const byte soundLengthAdress = 5; //Address where length of sound is stored

const byte maxVolDiff = 75; //Maximum difference between two volumes such that they are both considered as loud sounds
const byte NUMLED = 5; //Amount of LEDs we have
const byte centralLed = 4; //Index of the LED in the middle

//Pin assignments
const byte northMic = A0;
const byte eastMic = A1;
const byte southMic = A2;
const byte westMic = A3;

const byte rotPinA = 2;
const byte rotPinB = 3;
const byte buttonPin = 4;

const int rsPinDisplay = 12;
const int ePinDisplay = 11;
const int d4PinDisplay = 10;
const int d5PinDisplay = 9;
const int d6PinDisplay = 8;
const int d7PinDisplay = 7;

const byte ledPin = 5;

hd44780_pinIO lcd(rsPinDisplay, ePinDisplay, d4PinDisplay, d5PinDisplay, d6PinDisplay, d7PinDisplay); //Class needed for the Display to work
CRGB leds[NUMLED];

void setup(){
    //Read values from memory
    lowerBound = EEPROM.read(sensLowerBound);
    upperBound = EEPROM.read(sensUpperBound);
    soundLength = EEPROM.read(soundLengthAdress);
  
    if (lowerBound == 255) {
        lowerBound = 0;
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
    //pinMode(eastMic, INPUT);
    //pinMode(southMic, INPUT);
    //pinMode(westMic, INPUT);

    //Code to initialise display
    lcd.begin(16, 2);

    //Code to initialise LEDs
    //pinMode(ledPin, OUTPUT);
    FastLED.addLeds<WS2812, ledPin, GRB>(leds, NUMLED);

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
    settingChanged = 255;

  	for (int i = 0; i < sizeof(turnedOnTime); i++) {
        turnedOnTime[i] = 0;
    }

    loudDetectTime = 0;
  
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
        changeLED(dir);

        resetMicData();

        lastPoll  = millis() ;
    }

    if (buttonPushed) {
        buttonPushed = false; //Reset pushed state and change the mode
        changeMode();
    }

    if (settingChanged != 255 && abs(time - writeTime) > wTime) {
        switch (settingChanged) {
        case 0:
            EEPROM.write(sensLowerBound, lowerBound);

            break;

        case 1:
            EEPROM.write(sensUpperBound, upperBound);

            break;

        case 2:
            EEPROM.write(soundLengthAdress, soundLength);

            break;
        }

        settingChanged = 255; //Reset to nothing has changed
    }
    
    if (abs(time - lastDisPoll) > disPollTime) {
        updateDisplay();
        lastDisPoll = millis();
    }

}

void updateDisplay() {
    lcd.clear();
    switch(mode) {
      case 0:
        lcd.print("Lower Sens:");
        lcd.setCursor(0, 1);
        lcd.print((float)(lowerBound/255.0) * 100);
        
        break;
      case 1:
        lcd.print("Upper Sens:");
        lcd.setCursor(0, 1);
        lcd.print((float)(upperBound/255.0) * 100);
        
        break;
      case 2:
        lcd.print("Sound Length:");
        lcd.setCursor(0, 1);
        lcd.print(soundLength);
        
        break;
    }
}


void getMicData() {
    int avg = 512;
    int north = abs(analogRead(northMic) - avg);
    int south = abs(analogRead(southMic) - avg);
    int east = abs(analogRead(eastMic) - avg);
    int west = abs(analogRead(westMic)- avg);

    if (north > northMax && north > noise) {
        northMax = north;
    }
    
    if (south > southMax && south > noise) {
        southMax = south;
    }

    if (east > eastMax && east > noise) {
        eastMax = east;
    }

    if (west > westMax && west > noise) {
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
        return -1; //In case everything is 0 (hence only noise is detected) indicate this with this special value.
    }
}

void changeLED(int dir) {
    //Assuming LEDs are connected per direction, so 0 1 2 are the north facing LEDs, 3 4 5 the north east facing LEDs, etc.
    
    int max = 0;

    switch (dir) {
    case 0:
        if (northMax > (upperBound + 255)) {
            leds[centralLed] = CRGB(255, 0, 0);
            loudDetectTime = millis();
        }
        
        max = northMax;

        break;

    case 1:
        if (northMax > (upperBound + 255) || eastMax > (upperBound + 255)) {
            leds[centralLed] = CRGB(255, 0, 0);
            loudDetectTime = millis();
        }
        
        if (northMax > eastMax) {
            max = northMax;
        } else {
            max = eastMax;
        }

        break;

    case 2:
        if (eastMax > (upperBound + 255)) {
            leds[centralLed] = CRGB(255, 0, 0);
            loudDetectTime = millis();
        }

        max = eastMax;

        break;

    case 3:
        if (eastMax > (upperBound + 255) || southMax > (upperBound + 255)) {
            leds[centralLed] = CRGB(255, 0, 0);
            loudDetectTime = millis();
        }
        
        if (southMax > eastMax) {
            max = southMax;
        } else {
            max = eastMax;
        }

        break;

    case 4:
        if (southMax > (upperBound + 255)) {
            leds[centralLed] = CRGB(255, 0, 0);
            loudDetectTime = millis();
        }
        
        max = southMax;

        break;

    case 5:
        if (southMax > (upperBound + 255) || westMax > (upperBound + 255)) {
            leds[centralLed] = CRGB(255, 0, 0);
            loudDetectTime = millis();
        }
        
        if (southMax > westMax) {
            max = southMax;
        } else {
            max = westMax;
        }

        break;

    case 6:
        if (westMax > (upperBound + 255)) {
            leds[centralLed] = CRGB(255, 0, 0);
            loudDetectTime = millis();
        }
        
        max = westMax;

        break;

    case 7:
        if (northMax > (upperBound + 255) || westMax > (upperBound + 255)) {
            leds[centralLed] = CRGB(255, 0, 0);
            loudDetectTime = millis();
        }
        
        if (northMax > westMax) {
            max = northMax;
        } else {
            max = westMax;
        }

        break;
    
    default:
        break;
    }

    if (leds[centralLed] == CRGB(255, 0, 0) && abs(time - loudDetectTime) > (onTime * 1000)) {
        leds[centralLed] = CRGB(0, 0, 0);
    }

    //Note down the time when we turn on the LED
    if (dir != -1 && turnedOnTime[dir] == 0) {
        turnedOnTime[dir] = time;
    }

    //Check if the lights need to be turned red
    for (byte i = 0; i < sizeof(turnedOnTime); i++) {
        if (abs(time - turnedOnTime[i]) > (onTime * 1000)) {
            for (byte j = 0; j < 3; j++) {
                leds[i + j] = CRGB(255, 0, 0);
            }
        }
    }

    for (byte i = 0; i < sizeof(turnedOnTime); i++) {
        for (byte j = 0; j < 3; j++) {
            byte red = leds[i*3 + j].red;
            byte green = leds[i*3 + j].green;
            byte blue = leds[i*3 + j].blue;

            red -= 10;
            green -= 10;
            blue -=10;

            if (red == 0 && blue == 0 && green == 0) {
                turnedOnTime[i] = 0;
            }

            leds[i*3 + j] = CRGB(red, green, blue);
        }
    }
    

    //Check if lights need to be turned on
    if (dir != -1) {
        if (max > lowerBound) {
            leds[dir*3] = CRGB(0, 0, 255);
        }

        if (max > (2*255) / 3) {
            leds[dir*3 + 1] = CRGB(0, 0, 255);
        }
    
        if (max > (4*255) / 3) {
            leds[dir*3 + 2] = CRGB(0, 0, 255);
        }
    }

    FastLED.show(); //Update LEDs
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
    writeTime = millis();
    switch(mode) {
        case 0:
            if (lowerBound != 254) {
                lowerBound++;
                settingChanged = mode;
                
            }

            break;

        case 1:
            if (upperBound != 254) {
                upperBound++;
                settingChanged = mode;
            }

            break;

        case 2:
            if (soundLength != 254) {
                soundLength++;
                settingChanged = mode;
            }

            break;
    }
}

void decrease() {
    writeTime = millis();
    switch(mode) {
        case 0:
            if (lowerBound != 0) {
                lowerBound--;
                settingChanged = mode;
            }

            break;

        case 1:
            if (upperBound != 0) {
                upperBound--;
                settingChanged = mode;
            }

            break;

        case 2:
            if (soundLength != 1) {
                soundLength--;
                settingChanged = mode;
            }

            break;
    }
}