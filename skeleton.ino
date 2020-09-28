unsigned long time;
unsigned long lastPoll;
short pollTime;

//Pin assignments

void setup(){
    //Code to initialise rotary encoder
    //Code to initialise mics
    //Code to initialise display

    Serial.begin(9600);
    lastPoll = 0;
    pollTime = 100; //Polltime in milliseconds with a valid range between 0 and 32,767. Don't go over and don't go negative pls, thanks.
}

void loop() {
    time = millis();

    if (lastPoll + pollTime < time){
        //Get data from mics
        //Determine direction
        //Determine volume (if it cannot be read out trivially)
        //Change LEDs
    }

    if (changeDetected()) {
        //Change settings based on the rotary encoder
    }
}

boolean changeDetected(){
    //Logic to determine whether a change in the rotary encoder was detected
    return true;
}