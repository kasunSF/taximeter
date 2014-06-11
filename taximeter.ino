//pinouts
#define OPTION A0
#define START 13
#define END 12
#define REPORT 11
#define SERVICE 10

#define HALL 2
#define LCDPOWER A1
#define SERVICE_REM A2

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>

//variables
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
short pulseCounter;
unsigned short meters;
unsigned int vehicleSpeed;
float totalKm;
float hireTotalKm;
long income;
float hireDistance;
unsigned int fixedFare;
unsigned int fare;
int dailyRun;
long time1;
long time2;
bool working;
int waitingTime;
long waitingClock;
long showReportDelay;
tmElements_t tm;


bool commandSent;
bool onHire;
bool waiting;
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
  hireDistance = 0;
  fixedFare = 50;
  fare = fixedFare;
  vehicleSpeed = 0;
  onHire = false;
  waiting = false;
  time1 = millis();
  working = false;
  waitingTime = 0;
  showReportDelay = 0;
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
        lcd.clear();
      }
      else
        digitalWrite(LCDPOWER, LOW);
    }
  }
  
  time2 = millis()-time1;
  if(time2 >= 1000){
    vehicleSpeed = pulseCounter*3600/time2;
    if(vehicleSpeed == 0){
      waiting = true;
      waitingClock = millis();
    }
    else
      waiting = false;
      
    time1 = millis();
    pulseCounter = 0;    
  }
  
  if(meters >= 100){
    meters = 0;
    totalKm += 0.1;
    if(onHire){
      hireDistance += 0.1;
      hireTotalKm+=0.1;
    }
    if(hireDistance > 1){
      RTC.read(tm);
      if(tm.Hour > 21 && tm.Hour < 7){
        fare += 5;
        income += 5;
      }
      else{
        fare += 4;
        income += 4;
      }
    }
  }
  
  lcd.setCursor(0, 0);
  
  if(onHire){
    lcd.print(hireDistance);
    lcd.print("km ");
  
    lcd.print(fare);
    lcd.print("LKR      ");
  
  }
  
  lcd.setCursor(0, 1);
  if(onHire && waiting){
    if(millis() - waitingClock >= 30000){
      fare += 5;
      waitingClock = millis();
    }
    lcd.print("Waiting...");
  }
  else{
    lcd.print(vehicleSpeed);
    lcd.print("km/h        ");
  }
  
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
  lcd.setCursor(0, 0);
  lcd.print("Press service");
  lcd.setCursor(0, 1);
  lcd.print("for time/date");
  while(!commandSent){
    if(digitalRead(SERVICE) == LOW){
      Serial.println("Show time");
      delay(50);
      showTime();
      commandSent = true;
    }
    if(!commandSent){
      Serial.println(inputWait);
      --inputWait;
    }
    
    if(inputWait <= 0){
      Serial.println("No Input Detected");
      commandSent = true;
    }
  }
  lcd.clear();
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
    income += 50;
}

void endButton(){
  delay(50);
  if(!isLongPress(END)){
    if(onHire){
      Serial.println("Hire ended");
      onHire = false;
      saveData(1,hireTotalKm);
      saveData(3,income);
    }
  }
  else{
    Serial.println("Shutdown initiated");
    if(onHire){
      onHire = false;
      saveData(1,hireTotalKm);
      saveData(3,income);
    }
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
    Serial.println("Total distance");
    showTotDist();
  }
  else{
    Serial.println("Service Reminder Reset");
    digitalWrite(SERVICE_REM, LOW);
    clearDistance();
  }
}

void showDailyRpt(){
  showReportDelay = millis();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(income-readLong(4));
  lcd.print(" LKR");
  lcd.setCursor(0, 1);
  lcd.print(hireTotalKm-readFloat(5));
  lcd.print(" km");
  while(digitalRead(OPTION) == HIGH && millis() - showReportDelay < 5000);
  delay(600);
  lcd.clear();
}

void showMonthlyRpt(){
  showReportDelay = millis();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(income-readLong(6));
  lcd.print(" LKR");
  lcd.setCursor(0, 1);
  lcd.print(hireTotalKm-readFloat(7));
  lcd.print(" km");
  while(digitalRead(OPTION) == HIGH && millis() - showReportDelay < 5000);
  delay(600);
  lcd.clear();
}

void showTotDist(){
  showReportDelay = millis();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Total distance");
  lcd.setCursor(0, 1);
  lcd.print(totalKm);
  lcd.print(" km");
  while(digitalRead(OPTION) == HIGH && millis() - showReportDelay < 5000);
  delay(600);
  lcd.clear();
}

void clearDistance(){
  saveData(2,totalKm);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reset Suceeded!");
  delay(2000);
  lcd.clear();
}

void startup(){
  digitalWrite(LCDPOWER, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("    HyprLINK");
  lcd.setCursor(0, 1);
  lcd.print("   Taxi Meter");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("    WELCOME!");
  delay(1000);
  working = true;
  totalKm=readFloat(0);
  hireTotalKm = readFloat(1);
  income = readLong(3);
  if(totalKm-readFloat(2)>30000){
    digitalWrite(SERVICE_REM, HIGH);
  }
  
  if (RTC.read(tm)) {
    if(tm.Day!=readLong(8)){
      saveData(4,income);
      saveData(5,hireTotalKm);
      saveData(8,(long)tm.Day);
    }
    else if(tm.Month!=readLong(9)){
      saveData(6,income);
      saveData(7,hireTotalKm);
      saveData(9,(long)tm.Month);
    }
  }
  
}

void shutDown(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Shutting down....");
  saveData(0,totalKm);
  saveData(1,hireTotalKm);
  saveData(3,income);
  delay(2000);
  digitalWrite(LCDPOWER, LOW);
  lcd.clear();
  working = false;
}

/*
 addr 0 : Total distance
 addr 1 : Total hire distance
 addr 2 : Last service kilometer
 addr 3 : Total income
 addr 4 : Day start income
 addr 5 : Day start distance
 addr 6 : Month start income
 addr 7 : Month start distance
 addr 8 : Day
 addr 9 : Month
*/

void saveData(int addr, float data){
  saveData(addr,(long)data*10);
}

void saveData(int addr, long data){
  for(int i=addr*4;i<addr*4+4;i++){
    EEPROM.write(i, data & 0xFF);
    data=data >> 8;
  }
}

long readLong(int addr){
  long result=0;
  for(int i=addr*4+3;i>=addr*4;i--){
    result=(result << 8)+EEPROM.read(i);
  }
  return result;
}
float readFloat(int addr){
  return readLong(addr)/10.0;
}

void showTime(){
  tmElements_t tm;
  int refresh;
  long showReportDelay = millis();
  lcd.clear();
  if (RTC.read(tm)) {
    lcd.setCursor(0, 1);
    lcd.print(tm.Day);
    lcd.print("/");
    lcd.print(tm.Month);
    lcd.print("/");
    lcd.print(tmYearToCalendar(tm.Year));
    
    while(digitalRead(OPTION) == HIGH && millis() - showReportDelay < 5000){
      lcd.setCursor(0, 0);
      RTC.read(tm);
      if (tm.Hour >= 0 && tm.Hour < 10)
        lcd.print("0");
      lcd.print(tm.Hour);
      lcd.print(":");
      
      if (tm.Minute >= 0 && tm.Minute < 10)
        lcd.print("0");
      lcd.print(tm.Minute);
      lcd.print(":");
      
      if (tm.Second >= 0 && tm.Second < 10)
        lcd.print("0");
      lcd.print(tm.Second); 
    }
    delay(200);
  }
  else {
      lcd.setCursor(0, 0);
      lcd.print("Clock error!");
      lcd.setCursor(0, 1);
      lcd.print("Service meter");
      while(digitalRead(OPTION) == HIGH && millis() - showReportDelay < 5000);
  }
}
