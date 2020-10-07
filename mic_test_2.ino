#define ARRAYSIZE 100

int micPin = A0;
unsigned long time;
unsigned long lastPoll;
unsigned long micLastPoll;
int pollTime = 200;
int micPollTime = 20;


int val;
int valMax[ARRAYSIZE]={};
int valPrint;
byte noise = 65;


bool pushData = false;

void setup() {
  // put your setup code here, to run once:
  pinMode(A0, INPUT);
  Serial.begin(9600);
  lastPoll = 0;
  micLastPoll = 0;

  valPrint = 0;


}

void loop() {
  // put your main code here, to run repeatedly:
  time = millis();
  
  updateMic();
  
  Serial.println(valPrint);
  
  if (abs(time - lastPoll) > pollTime){
    Serial.println(valPrint);
    lastPoll = time;
  }
}

void updateMic() {
  val = abs(analogRead(micPin) - 512);
  int i=0;
  
  for(int i = 0; i < ARRAYSIZE; i++){
    if(valMax[i] < 1){
        valMax[i]= val;
        return;
    }
  }
  
  calcMax();
  resetMicData();


}
void resetMicData(){
    for(int i = 0 ; i<ARRAYSIZE;i++){
        valMax[i]=0;
    }
    
}
void calcMax(){
    valPrint = 0;
    for(int i = 0 ; i<ARRAYSIZE;i++){
        if(valMax[i] > valPrint && valMax[i] > noise){
            valPrint = valMax[i];
        }
    }
}
