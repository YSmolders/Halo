//Globals, things that cannot be stored locally. Might change when refactoring or when passing params is too slow.
unsigned long lastPoll;
short mode; //Which setting to change with a valid range between 0 and 2. Pls no negatives thx.

//Constants, things that don't change during execution
const short pollTime = 100; //Polltime in milliseconds with a valid range between 0 and 32,767. Don't go over and don't go negative pls, thanks.
const short modes = 3; //The number of settings we can change. 

//Pin assignments

void setup(){
    //Code to initialise rotary encoder
    //Code to initialise mics
    //Code to initialise display

    Serial.begin(9600);

    //Initialise global variables
    lastPoll = 0; 
    mode = 0;
}

void loop() {
    unsigned long time = millis();

    if (lastPoll + pollTime < time){
        //Get data from mics
        getMicData();
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
    //Read the data from the mics
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