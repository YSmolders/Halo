int pinA = 2;
int pinB = 3;
int count =0;

int valA;
int valB;

unsigned long currentTime;
unsigned long countPlusTime = 0;
unsigned long countMinTime = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pinA), updateCount, RISING);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  currentTime = millis();
  //Serial.print("A: ");
  //Serial.print(valA);
  //Serial.print(" ");
  //Serial.print("   B: ");
  //Serial.println(valB); 
  Serial.println(count);

}

void updateCount(){
  valA = digitalRead(pinA);
  valB = digitalRead(pinB);
  
  if(valA == valB && (currentTime > countMinTime + 50)){
    count++;
    countPlusTime = currentTime;
  }
  if(valA =! valB && (currentTime > countPlusTime + 50)){
    count--;
    countMinTime = currentTime;
  }

  

}

