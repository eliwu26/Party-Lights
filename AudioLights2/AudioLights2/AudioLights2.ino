#include <EEPROM.h>

#define PARTYMODE 0
#define SLOW 1
#define SINGLE 2
#define SINGLE_WHITE 3
#define BASSFADE 4
#define AMBIENT_GREEN 5
#define NIGHT_VISION 6
#define WHITE 7
#define OFF 8
#define MODE_MAX 8

bool isChanging = false;
volatile unsigned long timer_start;
int mode = PARTYMODE;
int strobe = 4; // strobe pins on digital 4
int res = 5; // reset pins on digital 5
int ledPin = 9;
int ledPin2 = 10;
int ledPin3 = 11;
int statusLedPin = 6;
int micPin = A0;
double led1Value = 0;
double led2Value = 0;
double led3Value = 0;
double led4Value = 0;
double allLedValue = 0;
double lastUpdate = 0;
double volumeMultiplier = 1;
int counter = 0;
double left[7]; // store band values in these arrays
double right[7];
double averagedLeft[7]; // store band values in these arrays
double averagedRight[7];
double timeMultiplier = 1;
int band;

static inline int8_t sign(int val) {
 if (val < 0) return -1;
 if (val==0) return 0;
 return 1;
}

void writeLED(int pin,double value){
  double val = 1/(1+exp(((value/21)-6)*-1))*255;
  analogWrite(pin,val);
}

void rawWriteColor(double red,
               double green,
               double blue, double value){
    double multiplier = value/255;
    double finalRed = red*multiplier;
    double finalBlue = blue*multiplier;
    double finalGreen = green*multiplier;
    analogWrite(ledPin2,finalRed);
    analogWrite(ledPin3,finalGreen);
    analogWrite(ledPin,finalBlue);
}

void changeMode(){
  if(micros()-timer_start < 100000){return;}
  int timestart = micros();
  for(int i = 0; i < 20000; i++){
    if(digitalRead(2) == 1) return;
    Serial.print("");
  }
  mode++;
  if(mode > MODE_MAX){
    mode = 0;
  }
  EEPROM.write(0, mode);
  rawWriteColor(0,0,0,0);
  timer_start = micros();
  isChanging = false;
  rawWriteColor(0,0,0,0);
  digitalWrite(statusLedPin,1);
  for(int i = 0; i < 30000; i++){  }
  digitalWrite(statusLedPin,0);
  for(int i = 0; i < 30000; i++){  }
}

void setup()
{
  
 Serial.begin(115200);
 Serial.println(EEPROM.read(0));
 mode = EEPROM.read(0);
 pinMode(res, OUTPUT); // reset
 pinMode(strobe, OUTPUT); // strobe
 digitalWrite(res,LOW); // reset low
 digitalWrite(strobe,HIGH); //pin 5 is RESET on the shield
 pinMode(ledPin, OUTPUT); 
 pinMode(ledPin2, OUTPUT); 
 pinMode(ledPin3, OUTPUT); 
 pinMode(statusLedPin, OUTPUT);
 pinMode(2,INPUT_PULLUP);
 attachInterrupt(0, changeMode, FALLING);
}
void readMSGEQ7()
// Function to read 7 band equalizers
{
 digitalWrite(res, HIGH);
 digitalWrite(res, LOW);
 for(band=0; band <7; band++)
 {
   digitalWrite(strobe,LOW); // strobe pin on the shield - kicks the IC up to the next band 
   delayMicroseconds(30); // 
   left[band] = analogRead(0); // store left band reading
   right[band] = analogRead(1); // ... and the right
   digitalWrite(strobe,HIGH); 
 }
}
void averagedMSEGEQ7(){
  for(int i = 0; i < 10; i++){
    readMSGEQ7();
    for(int z=0; z <7; z++)
     {
       averagedLeft[z] += left[z];
       averagedRight[z] += right[z];
     }
     delayMicroseconds(100);
  }
  for(int z=0; z <7; z++)
     {
       averagedLeft[z] /= 10.0;
       averagedRight[z] /= 10.0;
//       Serial.println(left[z]);
     }
}
double globalRed = 0;
double globalGreen = 0;
double globalBlue = 0;
void writeColor(double red,
               double green,
               double blue, double value){
    double multiplier = value/255;
    double finalRed = red*multiplier;
    double finalBlue = blue*multiplier;
    double finalGreen = green*multiplier;
    globalRed += finalRed;
    globalGreen += finalGreen;
    globalBlue += finalBlue;
    writeLED(ledPin2,globalRed);
    analogWrite(ledPin3,globalGreen);
    writeLED(ledPin,globalBlue);
}



void loop()
{
  double avgleft = 0;
  double avgright = 0;
     averagedMSEGEQ7();
   for (band = 0; band < 7; band++)
   {
     avgleft += averagedLeft[band];

   }
   avgleft /= 7;

   for (band = 0; band < 7; band++)
   {
     avgright += averagedRight[band];
//   Serial.print(right[band]);
//   Serial.print(" ");
   }
   avgright /= 7;
  double totalAverage = (averagedLeft[0]+averagedLeft[2]/2+averagedLeft[5]/2)/3;
  volumeMultiplier = 0.5*(400/totalAverage);
  timeMultiplier = volumeMultiplier;
  if(volumeMultiplier > 3.7){
//    out = 0;
//    out2 = 0;
//    out3 = 0;
    volumeMultiplier /= volumeMultiplier/2;
  }
  if(volumeMultiplier < 1){
    volumeMultiplier /= 1;
  }
  int num = (averagedLeft[0]);//*volumeMultiplier;
  int out = map(num,0,600/volumeMultiplier,0,255);
  
  int num2 = (averagedLeft[2]/2);//*volumeMultiplier;
  int out2 = map(num2,0,600/volumeMultiplier,0,255);
  
  int num3 = (averagedLeft[5]/2);//*volumeMultiplier;
  if(num3 < 40) num3 = 40;
  //Serial.println(num3);
  int out3 = (map(num3,40,700/volumeMultiplier,0,255)-out/4);
  if(out3 < 0) out3 = 0;
  Serial.print(num);
  Serial.print(" ");
  Serial.print(num2);
  Serial.print(" ");
  Serial.println(num3);
  int num4 = averagedLeft[1];
  int out4 = map(num4,0,800,0,255);
  
  double intensity = map(avgleft,0,700,0,100);
  
  if(mode == PARTYMODE || mode == SLOW){
    while(abs(led1Value-out)>1.0 || abs(led2Value-out2)>1.0 || abs(led3Value-out3)>1.0 || abs(led4Value-out4)>1.0){
      globalRed = 0;
    globalBlue = 0;
    globalGreen = 0;
      led1Value += 2*sign(out-led1Value);
  ////    writeColor(0,0,255,led1Value);
      writeLED(ledPin,led1Value);
      led2Value += sign(out2-led2Value);
  ////    writeColor(255,0,0,led2Value);
      writeLED(ledPin2,led2Value);
      led3Value += 0.7*sign(out3-led3Value);
  ////    writeColor(102,255,0,led3Value);
      analogWrite(ledPin3,led3Value);
      led4Value += sign(out4-led4Value);
      int delayMS = 200;
      if(mode == SLOW) delayMS = 1000;
      delayMicroseconds(delayMS*pow(timeMultiplier,1));
      }
  //  writeColor(102,255,0,60);
    }
    else if(mode == AMBIENT_GREEN){
      rawWriteColor(102,255,0,60);
    }
    else if(mode == NIGHT_VISION){
      rawWriteColor(255,0,0,100);
    }
    else if(mode == WHITE){
      rawWriteColor(255,255,255,100);
    }
    else if(mode == SINGLE){  
      while(abs(allLedValue-intensity)>1.0){
      allLedValue += 0.4*sign(intensity-allLedValue);
  ////    writeColor(0,0,255,led1Value);
      rawWriteColor(102,255,0,allLedValue);
      
      int delayMS = 3000/pow((abs(lastUpdate-intensity)+1),1);
//      Serial.println(delayMS);
      
      delayMicroseconds(1000*pow(timeMultiplier,1));
      }
      lastUpdate = intensity;
    }
    else if(mode == SINGLE_WHITE){  
      while(abs(allLedValue-intensity)>1.0){
      allLedValue += 0.4*sign(intensity-allLedValue);
  ////    writeColor(0,0,255,led1Value);
      rawWriteColor(255,255,255,allLedValue);
      
      int delayMS = 3000/pow((abs(lastUpdate-intensity)+1),1);
//      Serial.println(delayMS);
      
      delayMicroseconds(1000*pow(timeMultiplier,1));
      }
      lastUpdate = intensity;
    }
    else if(mode == BASSFADE){
      if(out < 15)out = 0;
       while(abs(allLedValue-out)>1.0){
          allLedValue += 2*sign(out-allLedValue);
      ////    writeColor(0,0,255,led1Value);
          writeLED(ledPin,allLedValue);
          writeLED(ledPin3,allLedValue>55?(200-allLedValue):200);
//          rawWriteColor(0,0,255,allLedValue);
          int delayMS = 3000/pow((abs(lastUpdate-out)+1),1);
    //      Serial.println(delayMS);
          
          delayMicroseconds(100*pow(timeMultiplier,1));
      }
      Serial.println(out);
      lastUpdate = out;
    }
    else if(mode == OFF){
      rawWriteColor(0,0,0,0);
    }
//    Serial.println("RNNING");
}

