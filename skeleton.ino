//Globals, things that cannot be stored locally. Might change when refactoring or when passing params is too slow.
unsigned long lastPoll;
short mode; //Which setting to change with a valid range between 0 and 2. Pls no negatives thx.
int northMax;
int eastMax;
int southMax;
int westMax;

//Constants, things that don't change during execution
const short pollTime = 100; //Polltime in milliseconds with a valid range between 0 and 32,767. Don't go over and don't go negative pls, thanks.
const short modes = 3; //The number of settings we can change. 

//Pin assignments
const int northMic = A0;
const int eastMic = A1;
const int southMic = A2;
const int westMic = A3;

void setup(){
    //Code to initialise rotary encoder

    //Code to initialise mics
    pinMode(northMic, INPUT);
    pinMode(eastMic, INPUT);
    pinMode(southMic, INPUT);
    pinMode(westMic, INPUT);

    //Code to initialise display

    Serial.begin(9600);

    //Initialise global variables
    lastPoll = 0; 
    mode = 0;
}

void loop() {
    unsigned long time = millis();

    //Get data from mics
    getMicData();

    if (lastPoll + pollTime < time){
        //Determine direction
        int dir = calculateDirection();
        //Determine volume (if it cannot be read out trivially)
        int volume = -1; //Replace by call to volume method (if not read out trivially, in that case it is assigned in getMicData)
        //Change LEDs
        changeLED(dir, volume);
    }

    if (changeDetected()) {
        //Change settings based on the rotary encoder
        //If pushed
        changeMode();

        //If turned CW
        increase();

        //If turned CCW
        decrease();
    }
}

boolean changeDetected(){
    //Logic to determine whether a change in the rotary encoder was detected
    return true;
}

void getMicData() {
    int north = analogRead(northMic);
    int south = analogRead(southMic);
    int east = analogRead(eastMic);
    int west = analogRead(westMic);

    if (north > northMax) {
        northMax = north;
    }

    if (south > southMax) {
        southMax = south;
    }

    if (east > eastMax) {
        eastMax = east;
    }

    if (west > westMax) {
        westMax = west;
    }
}

int calculateDirection() {
    //Calculate the direction given the data from the mics, might need to introduce params
    return -1;
}

void changeLED(int dir, int vol) {
    //Change which LEDs turn on based on direction and volume
}

void changeMode() {
    if (mode + 1 > modes) {
        mode = 0;
    } else {
        mode += 1;
    }
}

void increase() {
    //Increase the setting
}

void decrease() {
    //Decrease the setting
}