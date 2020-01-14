// PIN CONNECTIONS
// SDA -----  A4
// SCL -----  A5

// Rotary Encoder
//Set the digital 2 to A pin
//Set the digital 3 to B pin
//Set the digital 4 to S pin

// SPST Switch
// NormalOpenPin ------ 12

// Stepper Motor
// dir-  >>>>> GND
// pul-  >>>>> GND
// dir+  >>>>> 10
// pul+  >>>>> 11

// include the library code
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <AccelStepper.h>

LiquidCrystal_I2C lcd(0x27,20,4);

const int APin= 2; //Set the digital 2 to A pin
const int BPin= 3; //Set the digital 3 to B pin
const int SPin= 4 ;//Set the digital 4 to S pin

const int startPin= 12 ;
const int dirPin= 10 ;
const int stepPin= 11 ;
int motorInterfaceType = 1;
// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);


int lastButtonState = HIGH;
int lineNumber = 0;
bool anyUpdate = false;
bool doneAction = true;
int lastReading = 0;
int eeAddress = 0;   //Location we want the data to be put.

double readings[4] = {1.0, 20.0, 1.0, 20.0};
double minReadings[4] = {0.0, 25.0, 0.0, 25.0};
double maxReadings[4] = {10.0, 200.0, 10.0, 200.0};
double startReadings[4] = {1.0, 20.0, 1.0, 20.0};
double increments[4] = {0.1, 1.0, 0.1, 1.0};



struct ReadingStruct {
  double lift_distance;
  double lift_speed;
  double plunge_distance;
  double plunge_speed;
};

void putData(){
  //Data to store.
  ReadingStruct Reading2save = {
    readings[0],
    readings[1],
    readings[2],
    readings[3]
  };
  EEPROM.put(eeAddress, Reading2save);
  Serial.print("Written readings data !");
}

void processData(){
  // This function will get data from EEPROM and move motor accordingly
  ReadingStruct Reading2get;  
  EEPROM.get(eeAddress, Reading2get);
  Serial.println("Read motor data from EEPROM: ");
  Serial.println(Reading2get.lift_distance);
  Serial.println(Reading2get.lift_speed);
  Serial.println(Reading2get.plunge_distance);
  Serial.println(Reading2get.plunge_speed);


  int steps2Lift = int(Reading2get.lift_distance/0.04);
  int speed2Lift = int(Reading2get.lift_speed/0.04);
  int steps2Plunge = int(Reading2get.plunge_distance/0.04);
  int speed2Plunge = int(Reading2get.plunge_speed/0.04);
  
  Serial.println("Motor will run now");
  Serial.println(steps2Lift);
  Serial.println(speed2Lift);
  Serial.println(steps2Plunge);
  Serial.println(speed2Plunge);

  // the lift operation
  stepper.setMaxSpeed(speed2Lift);
  stepper.moveTo(steps2Lift);
  stepper.runToPosition();
  stepper.setCurrentPosition(0);
  delay(100);

  // the plunge operation
  
  stepper.setMaxSpeed(speed2Plunge);
  stepper.moveTo(-steps2Plunge);
  stepper.runToPosition();
  stepper.setCurrentPosition(0);

  doneAction = false;
}

int getRotaryEncoder(void){
  static int oldA = HIGH; //set the oldA as HIGH
  static int oldB = HIGH; //set the oldB as HIGH
  int result = 0;
  int newA = digitalRead(APin); //read the value of APin to newA
  int newB = digitalRead(BPin); //read the value of BPin to newB
  if (newA != oldA || newB != oldB)//if the value of APin or the BPin has changed
   {
   if (oldA == HIGH && newA == LOW)// something has changed
     {
     result = (oldB * 2 - 1);
     }
   }
  oldA = newA;
  oldB = newB;
  return result;
}

void updateDisplay(int i, double data){
  for(int k=0; k<4; k++)
  {
    
    if(i==k){
      lcd.setCursor(0, k);
      lcd.write(0b00011101);
    }
    else
    {
      lcd.setCursor(0, k);
      lcd.print(" ");
    }
  }
  lcd.setCursor(10, i);
  lcd.print("       ");
  lcd.setCursor(10, i);
  lcd.print(data);
  anyUpdate = false;
}

void updateReadings(int i, double data){
  data = readings[i] + (data * increments[i]);
  if(data < minReadings[i])
    data = minReadings[i];
  if(data > maxReadings[i])
    data = maxReadings[i];

   readings[i] = data;

}

void setup() {
  pinMode(APin, INPUT_PULLUP);//initialize the A pin as input
  pinMode(BPin, INPUT_PULLUP);//initialize the B pin as input
  pinMode(SPin, INPUT_PULLUP);//initialize the S pin as input
  pinMode(startPin, INPUT_PULLUP);
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  
  Serial.begin(9600); //opens serial port, sets data rate to 9600 bps
  
  lcd.init();  //initialize the lcd
  lcd.backlight();  //open the backlight
  lcd.setCursor(0, 0);
  lcd.write(0b00011101);
  lcd.print(" Lift D: ");
  lcd.setCursor(0, 1);
  lcd.print("  Lift S: ");
  lcd.setCursor(0, 2);
  lcd.print("  Plunge D: ");
  lcd.setCursor(0, 3);
  lcd.print("  Plunge S: ");

  for(int k=0; k<4; k++){
    lcd.setCursor(10, k);
    lcd.print(startReadings[k]);
  }
  
}

void loop() {
  int currentSPSTState = digitalRead(startPin);
  if(currentSPSTState == LOW && doneAction == true){
    processData();
    while(!digitalRead(startPin));
    doneAction = true;
  }
  
  // Get any change in rtary encoder
  int raw_reading = getRotaryEncoder();

  // Check if any value saved by rotary encoder
  int currentButtonState = digitalRead(SPin);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    anyUpdate = true;    
    lineNumber++;

    //NOTE: when all 4 values are SET, the data will be saved in EEPROM
    if(lineNumber==4){
      lineNumber=0;
      putData();  // saving data
    }
  }

  // If we have any change in reading from rotary encoder
  if(raw_reading != lastReading || anyUpdate == true){
    updateReadings(lineNumber, raw_reading);
    updateDisplay(lineNumber, readings[lineNumber]);
  }

  
  // Store the button's state so we can tell if it's changed next time round
  lastButtonState = currentButtonState;
  lastReading = raw_reading;
}
