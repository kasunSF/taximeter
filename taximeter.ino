//pinouts
#define OPTION A0
#define START 10
#define END 11
#define REPORT 12
#define SERVICE 13
#define HALL 2
#define LCDPOWER A1
#define SERVICE_REM A2

#include <LiquidCrystal.h>
#include <EEPROM.h>

//variables
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
short pulseCounter;
unsigned short meters;
unsigned int vehicleSpeed;
float kilometers;
float hireDistance;
unsigned int fixedFare;
unsigned int fare;
int dailyRun;
long time1;
long time2;
bool working;

bool commandSent;
bool onHire;
int inputWait;
int longPress;

//Setup
void setup() {
  Serial.begin(9600);
  
  lcd.begin(16, 2);
  lcd.clear();
  
  pinMode(HALL, INPUT);
  pinMode(OPTION, INPUT);
  pinMode(START, INPUT);
  pinMode(END, INPUT);
  pinMode(REPORT, INPUT);
  pinMode(SERVICE, INPUT);
  pinMode(LCDPOWER, OUTPUT);
  pinMode(SERVICE_REM, OUTPUT);
  
  attachInterrupt(0, calc, RISING);
  
  pulseCounter = 0;
  meters = 0;
  kilometers = 0;
  hireDistance = 0;
  fixedFare = 50;
  fare = fixedFare;
  vehicleSpeed = 0;
  onHire = false;
  time1 = millis();
  working = false;
}

//Main functionn
void loop() {
  while(!working){
    if(digitalRead(START) == LOW){
      digitalWrite(LCDPOWER, HIGH);
      delay(50);
      if(isLongPress(START)){
        Serial.println("Starting taxi meter");
        startup();
      }
      else
        digitalWrite(LCDPOWER, LOW);
    }
  }
  
  time2 = millis()-time1;
  if(time2 >= 1000){
    vehicleSpeed = pulseCounter*3600/time2;
    time1 = millis();
    pulseCounter = 0;    
    //Serial.println(meters);
  }
  
  if(meters >= 10){
    meters = 0;
    kilometers += 0.1;
    if(onHire)
      hireDistance += 0.1;
    if(hireDistance > 1)
      fare += 4;
  }
  
  lcd.setCursor(0, 0);
  
  if(onHire){
    lcd.print(hireDistance);
    lcd.print("km ");
  
    lcd.print(fare);
    lcd.print("LKR     ");
  
  }
  
  lcd.setCursor(0, 1);
  
  lcd.print(vehicleSpeed);
  lcd.print("km/h    ");
  
  if(!onHire){
    if (digitalRead(OPTION) == LOW)
      readCommand();
    else if(digitalRead(START) == LOW)
      startButton();
    else if(digitalRead(REPORT) == LOW)
      reportButton();
    else if(digitalRead(SERVICE) == LOW)
      serviceButton();
  }
  if(digitalRead(END) == LOW)
    endButton();
}

//ISR 0
void calc(){
  ++pulseCounter;
  ++meters;
}

void readCommand(){
  commandSent = false;
  Serial.println("Waiting for command....");
  inputWait = 500;
  
  while(!commandSent){
    if(digitalRead(START) == LOW){
      Serial.println("Set RTC");
      while(digitalRead(START) == LOW);
      setRTC();
      commandSent = true;
    }
    Serial.println(inputWait);
    --inputWait;
    
    if(inputWait <= 0){
      Serial.println("No Input Detected");
      commandSent = true;
    }
  }
}

bool isLongPress(int pin){
  longPress = 300;
  while(digitalRead(pin) == LOW){
    Serial.println(longPress);
    --longPress;
    if(longPress<0)longPress=0;
  }
  if(longPress>100)
    return false;
  else
    return true;
}

void startButton(){
    Serial.println("Hire started");
    hireDistance = 0;
    fare = fixedFare;
    onHire = true;
}

void endButton(){
  delay(50);
  if(!isLongPress(END)){
    if(onHire){
      Serial.println("Hire ended");
      onHire = false;
    }
  }
  else{
    Serial.println("Shutdown initiated");
    shutDown();
  }
}

void reportButton(){
  delay(50);
  if(!isLongPress(REPORT)){
    Serial.println("Daily report");
    showDailyRpt();
  }
  else{
    Serial.println("Monthly report");
    showMonthlyRpt();
  }
}

void serviceButton(){
  delay(50);
  if(!isLongPress(SERVICE)){
    Serial.println("Total kilometers");
    showTotDist();
  }
  else{
    Serial.println("Service Reminder Reset");
    digitalWrite(SERVICE_REM, LOW);
    clearDistance();
  }
}

void showDailyRpt(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(readData(4));
  lcd.print(" LKR");
  lcd.setCursor(0, 1);
  lcd.print(readData(8));
  lcd.print(" km");
  while(digitalRead(OPTION) == HIGH);
  delay(600);
  lcd.clear();
}

void showMonthlyRpt(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(readData(12));
  lcd.print(" LKR");
  lcd.setCursor(0, 1);
  lcd.print(readData(16));
  lcd.print(" km");
  while(digitalRead(OPTION) == HIGH);
  delay(600);
  lcd.clear();
}

void showTotDist(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Total distance");
  lcd.setCursor(0, 1);
  lcd.print(readData(0));
  lcd.print(" km");
  while(digitalRead(OPTION) == HIGH);
  delay(600);
  lcd.clear();
}

void clearDistance(){
  //saveData(0, 0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reset Suceeded!");
  delay(4000);
  lcd.clear();
}

void startup(){
  digitalWrite(LCDPOWER, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("    HyprLINK");
  lcd.setCursor(0, 1);
  lcd.print("   Taxi Meter");
  delay(5000);
  lcd.clear();
  working = true;
  if(readData(0) >= 30000){
    digitalWrite(SERVICE_REM, HIGH);
  }
}

void shutDown(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Shutting down....");
  //saveData();
  delay(5000);
  digitalWrite(LCDPOWER, LOW);
  lcd.clear();
  working = false;
}

void setRTC(){
  
}

/*
 addr 0 : Total distance
 addr 4 : Daily income
 addr 8 : Daily distance
 addr 12 : Monthly income
 addr 16 : Monthly distance
*/
void saveData(int addr, long data){
  for(int i=addr*4;i<addr*4+4;i++){
    EEPROM.write(i, data & 0xFF);
    data >> 8;
  }
}

long readData(int addr){
  long result=0;
  for(int i=addr*4;i<addr*4+4;i++){
    result << 8;
    result+=EEPROM.read(i);
  }
  return result;
}
