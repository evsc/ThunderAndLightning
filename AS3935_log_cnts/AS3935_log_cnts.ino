/*

  code based on AS3935_example
  of AS3935-Arduino-library
  
  adapted to count lightning strikes and
  man-made disturbers, and output the counts
  every X seconds via the Serial port
  
*/


#include <SPI.h>
#include <AS3935.h>

// #define A_MEGA_1280
#define A_PRO_MINI

void updateAS3935Registers();
byte SPItransfer(byte sendByte);  // SPI communication with AS3935 IC
void AS3935Irq();  // interrupt handler
volatile int AS3935IrqTriggered;

#define IRQpin 2

#ifdef A_MEGA_1280
  #define CSpin 53
#endif

#ifdef A_PRO_MINI
  #define CSpin 10
#endif
  
AS3935 AS3935(SPItransfer,CSpin,IRQpin);  // define Chip-Select and Interrupt Pins

int pNoiseFloor;
int pSpikeRejection;
int pWatchdog;
int pMinLightning;
int pCapValue;
int pIndoors;

int cntDisturber;
int cntLightning;
int cntNoiseLevel;
int strokeDistance;
int strokeEnergy;

int timer;
unsigned long currentTime;
unsigned long lastTime;
unsigned long startTime;
int dt;
int every;

boolean commApp;   // communicate Serial to app, or write to serial-monitor

float speedSound = 0.343;  // km per second

#define LightningPin 4
#define DisturberPin 5
#define ThunderPin 6

int showLightning = 100;  // milliseconds. turn on light
int showDisturber = 50;
int showThunder = 100;
int timerLightning = 0;  // ms
int timerDisturber = 0;  // ms
long delayThunder = 0;  // adjust according to current lightning distance
int timerThunder = 0;    // ms 

void setup() {
  
  commApp = true;
  
  Serial.begin(9600);
  
  SPI.begin();
  SPI.setDataMode(SPI_MODE1);  // NB! chip uses SPI MODE1
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setBitOrder(MSBFIRST);
  
  AS3935.reset();  // reset all internal register values to defaults
  
  
  
  pNoiseFloor = 0;      // 0 to 7
  pSpikeRejection = 0;  // 0 to 7
  pWatchdog = 0;
  pMinLightning = 1;
  pCapValue = -1;
  
  cntDisturber = 0;
  cntLightning = 0;
  cntNoiseLevel = 0;
  strokeDistance = -1;
  strokeEnergy = -1;
  
  AS3935.setNoiseFloor(pNoiseFloor);
  AS3935.setSpikeRejection(pSpikeRejection);
  AS3935.setWatchdogThreshold(pWatchdog);
  
  //AS3935.setIndoors();
  AS3935.setOutdoors();
  
  // AS3935.enableDisturbers();
  AS3935.disableDisturbers();
  
  AS3935IrqTriggered = 0;
  attachInterrupt(0,AS3935Irq,RISING);
  
  updateAS3935Registers();
  
  // now let's calibrate, after all the settings have been set
  // outputCalibrationValues();
  recalibrate();
  
  timer = 0;
  dt = 0;
  every = 60;
  startTime = millis();
  lastTime = startTime;
  
  pinMode(LightningPin,OUTPUT);
  pinMode(DisturberPin,OUTPUT);
  pinMode(ThunderPin,OUTPUT);
  
  digitalWrite(LightningPin,LOW);
  digitalWrite(DisturberPin,LOW);
  digitalWrite(ThunderPin,LOW);
  
  // show lightning LEDs as test
  timerLightning = showLightning;
  digitalWrite(LightningPin,HIGH);
  delayThunder = speedSound * 1 * 1000;

}

void loop() {
  
  
  // time to output log data?
  currentTime = millis();
  dt = currentTime - lastTime;  // milliseconds since last loop()
  lastTime = currentTime;
  timer = (currentTime - startTime)/1000;  // seconds since last log
  
  
  // LED lightning
  if (timerLightning>0) {
    // this means the LED is on
    timerLightning -= dt;
    if (timerLightning <= 0) {
      // enough time has passed, turn LED off
      digitalWrite(LightningPin,LOW);
      timerLightning = 0;
    }
  }
  
  // LED distruber
  if (timerDisturber>0) {
    // this means the LED is on
    timerDisturber -= dt;
    if (timerDisturber <= 0) {
      // enough time has passed, turn LED off
      digitalWrite(DisturberPin,LOW);
      timerDisturber = 0;
    }
  }
  
  // wait for thunder
  if (delayThunder>0) {
    // this means lightning just happened 
    // (TODO: need array of counters for several strikes in short time)
    delayThunder -= dt;
    if (delayThunder <= 0) {
      timerThunder = showThunder;
      digitalWrite(ThunderPin, HIGH);
      delayThunder = 0;
    }
  }
  
  // Solenoid thunder
  if (timerThunder>0) {
    // this means the Solenoid is on
    timerThunder -= dt;
    if (timerThunder <= 0) {
      // enough time has passed, turn Solenoid off
      digitalWrite(ThunderPin,LOW);
      timerThunder = 0;
    }
  }
  
  
  
  if (timer >= every) {
    startTime = millis();
    
    if (commApp) {
      Serial.print("0"); // 0 represents log cnts
      Serial.print(",");
      Serial.print(every,DEC);
//      Serial.print(",");
//      Serial.print(cntNoiseLevel,DEC);
      Serial.print(",");
      Serial.print(cntDisturber,DEC);
      Serial.print(",");
      Serial.print(cntLightning,DEC);
      Serial.print(",");
      Serial.print(strokeDistance,DEC);
      Serial.print(",");
      Serial.print(strokeEnergy,DEC);
      Serial.print(",");
      Serial.print(pNoiseFloor,DEC);
      Serial.print(",");
      Serial.print(pSpikeRejection,DEC);
      Serial.print(",");
      Serial.print(pWatchdog,DEC);
      Serial.print(",");
      Serial.print(pCapValue,DEC);
      Serial.print(",");
      Serial.print(pIndoors,DEC);
      Serial.println();
    } else { 
      Serial.print("-");
      Serial.print("\tnoise:");
      Serial.print(cntNoiseLevel,DEC);
      Serial.print("\tdisturber: ");
      Serial.print(cntDisturber,DEC);
      Serial.print("\tlightning: ");
      Serial.print(cntLightning,DEC);
      Serial.print("\tstroke distance: ");
      Serial.println(strokeDistance,DEC);
    }
    
    cntDisturber = 0;
    cntLightning = 0;
    cntNoiseLevel = 0;
    strokeDistance = -1;
    strokeEnergy = -1;
    timer = 0;
  }
  
  // does processing want us to change any settings?
  while (Serial.available() > 0) {
    
    int incomingByte = Serial.read();
    byte mode = char(incomingByte);
    
    // apparently we need a slight delay here
    // to make sure the second byte has arrived!
    delay(50);
    
    if (mode == 'e') {
      // change the time interval
      incomingByte = Serial.read();
      every = (int) incomingByte;
      
      Serial.print("9");
      Serial.print(",");
      Serial.print(String(every));
      Serial.println();
    }
    else if (mode == 'c') {
      recalibrate();
    }
    else if (mode == 'v') {
      outputCalibrationValues();
    }
    else if (mode == 't') {
       // set thresholds
      int noisefloor = (int) Serial.read();
      int spike = (int) Serial.read();
      int watchdog = (int) Serial.read();
      
      AS3935.setNoiseFloor(noisefloor);
      AS3935.setSpikeRejection(spike);
      AS3935.setWatchdogThreshold(watchdog);
      
      updateAS3935Registers();
      
      Serial.print("9");
      Serial.print(",");
      Serial.print(pNoiseFloor);
      Serial.print(",");
      Serial.print(pSpikeRejection);
      Serial.print(",");
      Serial.println(pWatchdog);
    }
    else if (mode == 'a') {
      incomingByte = Serial.read();
      byte tune = 5;
      pCapValue = 5;
      long frequency = (long) AS3935.tuneAntenna(tune) * 1600;
      
      Serial.print("9");
      Serial.print(",");
      Serial.print(tune);
      Serial.print(",");
      Serial.print(String(frequency));
      Serial.println();
      
    }
    else if (mode == 'l') {
      // set indoors / outdoors
      pIndoors = (int) Serial.read();
      if (pIndoors == 1) {
        AS3935.setIndoors();
        Serial.print("9");
        Serial.print(",");
        Serial.print("1");
        Serial.println();
      } else {
        AS3935.setOutdoors();  
        Serial.print("9");
        Serial.print(",");
        Serial.print("0");
        Serial.println();
      }
    }
  }
  
  
  if(AS3935IrqTriggered) {
    
    AS3935IrqTriggered = 0; // reset flag
    
    delay(3);   // wait 2 ms before reading register (according to datasheet)
    
    // first step is to find out what caused interrupt
    // as soon as we read interrupt cause register, irq pin goes low
    int irqSource = AS3935.interruptSource();
    
    // returned value is bitmap field, 
    //       bit 0 - noise level too high
    //       bit 2 - disturber detected
    //       bit 3 - lightning!
    if (irqSource & 0b0001) {
      // NOISE
      if (commApp) {
        Serial.print("3");
        Serial.print(",");
        Serial.println("0");
      } else {
        Serial.println("Noise level too high, try adjusting noise floor");
      }
      cntNoiseLevel++;
    }
      
    if (irqSource & 0b0100) {
      // DISTURBER
      if (commApp) {
        Serial.print("2");
        Serial.print(",");
        Serial.println("0");
      } else {
        Serial.println("Disturber detected");
      }
      cntDisturber++;
      timerDisturber = showDisturber;
      digitalWrite(DisturberPin,HIGH);
      // lets use this to make thunder for now
      // assume 5km distance
      // delayThunder = speedSound * 5000;
    }
    
    if (irqSource & 0b0000) {
      // update to stroke distance
      strokeDistance = AS3935.lightningDistanceKm();
      if (commApp) {
        Serial.print("4");
        Serial.print(",");
        Serial.println(strokeDistance);
      } else {
        Serial.print("Distance changed to ");
        Serial.print(strokeDistance,DEC);
        Serial.println(" km.");
      }

    }
      
    if (irqSource & 0b1000) {
      // LIGHTNING STRIKE
      
      // need to find how far that lightning stroke, function returns approximate distance in kilometers,
      // where value 1 represents storm in detector's near victinity, and 63 - very distant, out of range stroke
      // everything in between is just distance in kilometers
      strokeEnergy = AS3935.lightningEnergy();
      strokeDistance = AS3935.lightningDistanceKm();
      
      if (commApp) {
        Serial.print("1");
        Serial.print(",");
        Serial.print(strokeDistance);
        Serial.print(",");
        Serial.print(strokeEnergy);
        Serial.println();
      } else {
        if (strokeDistance == 1)
          Serial.println("Storm overhead, watch out!");
        if (strokeDistance == 63)
          Serial.println("Out of range lightning detected.");
        if (strokeDistance < 63 && strokeDistance > 1)
        {
          Serial.print("Lightning detected ");
          Serial.print(strokeDistance,DEC);
          Serial.println(" kilometers away.");
        }
      }
      
      timerLightning = showLightning;
      digitalWrite(LightningPin,HIGH);
      
      // activate thunder
      delayThunder = speedSound * strokeDistance * 1000;
      
      cntLightning++;
    }
    
  }
  
}



void updateAS3935Registers() {
  pNoiseFloor = AS3935.getNoiseFloor();
  pSpikeRejection = AS3935.getSpikeRejection();
  pWatchdog = AS3935.getWatchdogThreshold();
  pMinLightning = AS3935.getMinimumLightnings();
  if (commApp) {
    
  } else {
    Serial.print("Noise floor is: ");
    Serial.println(pNoiseFloor,DEC);
    Serial.print("Spike rejection is: ");
    Serial.println(pSpikeRejection,DEC);
    Serial.print("Watchdog threshold is: ");
    Serial.println(pWatchdog,DEC); 
    Serial.print("Minimum Lightning is: ");
    Serial.println(pMinLightning,DEC);   
  }
}



// this is implementation of SPI transfer that gets passed to AS3935
// you can (hopefully) wrap any SPI implementation in this
byte SPItransfer(byte sendByte)
{
  return SPI.transfer(sendByte);
}

// this is irq handler for AS3935 interrupts, has to return void and take no arguments
// always make code in interrupt handlers fast and short
void AS3935Irq()
{
  AS3935IrqTriggered = 1;
}


// calibrate the RLC circuit
void recalibrate() {
  delay(50);
  pCapValue = AS3935.getBestTune();
  if (commApp) {
    Serial.print("9");
    Serial.print(",");
    Serial.print(String(pCapValue));
    Serial.println();
  } else {
    Serial.print("antenna calibration picks capacitor value:\t ");
    Serial.println(pCapValue);
  }
  delay(50);
}


// go through all 16 tuneCapacitor settings and 
// output the resonant frequencies that they achieve
void outputCalibrationValues() {
  delay(50);
  if (!commApp) Serial.println();
  
  for (byte i = 0; i <= 0x0F; i++) {
    
    int frequency = AS3935.tuneAntenna(i);
    // multiply with clock-divider, and 10 (because measurement is for 100ms)
    long fullFreq = (long) frequency*160;  

    if (commApp) {
      Serial.print("9");
      Serial.print(",");
      Serial.print(i);
      Serial.print(",");
      Serial.print(frequency);
      Serial.print(",");
      Serial.print(fullFreq);
      Serial.println();
    } else {
      Serial.print("tune antenna to capacitor ");
      Serial.print(i);
      Serial.print("\t gives frequency: ");
      Serial.print(frequency);
      Serial.print(" = ");
      Serial.print(fullFreq,DEC);
      Serial.println(" Hz");
      
    }
    
    delay(10);
    
  }
  
}
