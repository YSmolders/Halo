unsigned long currentTime;
unsigned long oldTime =0;

int pinLinks = A0;
int pinRechts = A2;
int pinVoor = A1;

int valLinks = 0;
int valRechts = 0;
int valVoor = 0;

int maxLinks= 0;
int maxRechts = 0;
int maxVoor = 0;

const int ledLinks =2;
const int ledRechts =3;
const int ledVoor =4;

void setup() {
  // put your setup code here, to run once:
  pinMode(pinLinks, INPUT);
  pinMode(pinRechts, INPUT);
  pinMode(pinVoor, INPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

  currentTime = millis(); //reading the on time of the board


  readVals();

  highestInSecond();

  


  
}


//functions:

void readVals(){
  valLinks = analogRead(pinLinks);
  valRechts = analogRead(pinRechts);
  valVoor = analogRead(pinVoor);
}

void printValues(){

  if(maxLinks+maxRechts+maxVoor < 1100){
    return;
  }
  
  Serial.print("Links");
  Serial.println(maxLinks);

  Serial.print("Rechts");
  Serial.println(maxRechts);

  Serial.print("Voor: ");
  Serial.println(maxVoor);
  
}

void highestInSecond(){
  if(valLinks > maxLinks){
    maxLinks = valLinks;
  }
  if(valRechts > maxRechts){
    maxRechts = valRechts;
  }
  if(valVoor > maxVoor){
    maxVoor = valVoor;
  }

  if(currentTime > oldTime + 1000){
    printValues();
    oldTime=currentTime;
    doLed();
    maxLinks =0;
    maxRechts=0;
    maxVoor=0;
  }
}

void doLed(){
  digitalWrite(ledLinks, LOW);
  digitalWrite(ledRechts, LOW);
  digitalWrite(ledVoor, LOW);  

  if(maxLinks+maxRechts+maxVoor < 1100){
    return;
  }

  if(maxLinks > maxRechts){
    if(maxLinks>maxVoor){
      digitalWrite(ledLinks, HIGH);
    }
    else{
      digitalWrite(ledVoor,HIGH);
    }
  }
  else{
    if(maxRechts > maxVoor){
      digitalWrite(ledRechts, HIGH);
    }
    else{
      digitalWrite(ledVoor, HIGH);
    }
  }
}






  
