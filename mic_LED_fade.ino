#include <Adafruit_NeoPixel.h> 



int northMic = A0;
int ledpin = 5;


int northMax;
int eastMax = 0;
int westMax=0;
int southMax = 0;
int pollTime = 200;
int ledRefresh = 50;
int noise = 180;
const byte maxVolDiff = 50; //Maximum difference between two volumes such that they are both considered as loud sounds

int firstLED[8] = {};
int secondLED[8] = {};
int thirdLED[8] = {};

int x;
int y;
int z; 


unsigned long time;
unsigned long lastPoll;
unsigned long LedTime;
unsigned long lastRefresh;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, ledpin, NEO_GRB + NEO_KHZ800);

void setup() {
  // put your setup code here, to run once:
  pinMode(A0, INPUT);
  Serial.begin(9600);

     //Code to initialise LEDs
    //pinMode(ledPin, OUTPUT);
    strip.begin(); //Initialise LED strip
    strip.show(); //Turn off all LEDs
    strip.clear();

  northMax = 0;
  lastPoll=0;
}

void loop() {
  // put your main code here, to run repeatedly:
    
    time = millis();
    getMicData();

    
    if (abs(time - lastPoll) > pollTime){

        int dir = calculateDirection();

        
        setLED(dir);
        
        resetMicData();
        lastPoll=time;
    }
    if(abs(time - lastRefresh) > ledRefresh){
    changeLED();
    lastRefresh = time;
    }
}

void getMicData() {
    int north = abs(analogRead(northMic)-512);

    if (north > northMax && north > noise) {
        northMax = north;
    }
    Serial.println(north);
    
}

void resetMicData(){
    
    northMax = 0;
}

void setLED(int dir){
    
    switch(dir){
        case 0:
            if(northMax > 1){
                firstLED[dir] = 100;
            }
            if(northMax > 300){
                secondLED[dir] = 100;
            }
            if(northMax > 450) {
                thirdLED[dir] = 100;
            }
            break;

            default:
            break;
    }

}
void changeLED(){

    for(int i=0; i<8;i++){
        strip.setPixelColor(i*3, strip.Color(firstLED[i]*2,0,0));
        strip.setPixelColor(i*3+1, strip.Color(secondLED[i]*2,0,0));
        strip.setPixelColor(i*3+2, strip.Color(thirdLED[i]*2,0,0));
    }
    
    for(int i=0;i<8;i++){
        if(firstLED[i] > 0){
            firstLED[i] = firstLED[i] - 1;
        }
        if(secondLED[i] > 0){
            secondLED[i] = secondLED[i] - 1;
        }
        if(thirdLED[i] > 0){
            thirdLED[i] = thirdLED[i] - 1;
        }
    }
    
    strip.show();
    
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
