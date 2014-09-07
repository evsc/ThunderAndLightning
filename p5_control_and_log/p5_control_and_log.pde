

import processing.serial.*;
import java.io.BufferedWriter;
import java.io.FileWriter;

Serial myPort;  // Create object from Serial class
String logFile = "lightning_log.csv";
String msgCol[] = {"mode", "intv", "dist", "light", "km", "energy", "nfloor", "spike", "watchd", "tune", "indoors"};

PFont f1;
PFont f2;
PFont f3;

String date = "";
String time = "";
int interval = -1;
int lightningCnt = -1;
int disturberCnt = -1;
int distance = -1;
long energy = -1;
int pNoiseFloor = -1;
int pSpikeRejection = -1;
int pWatchdog = -1;
int pTuneCapacitor = -1;
int indoors = 0;

int historyLightning[];
int historyDisturber[];

boolean lightningEvent = false;
boolean disturberEvent = false;

int setInterval = 10;
int setNoiseFloor = 1;
int setSpike = 1;
int setWatchdog = 1;

void setup() {
  size(500,600);
  
  println(Serial.list());
  String portName = Serial.list()[32];
  myPort = new Serial(this, portName, 9600);
  
  // read bytes into a buffer until you get a linefeed (ASCII 10):
  myPort.bufferUntil('\n');
  
  //printArray(PFont.list());
  f1 = createFont("Ubuntu Monospace", 16);
  f2 = createFont("Ubuntu Monospace", 12);
  f3 = createFont("Ubuntu Monospace", 32);
  textFont(f1);
  
  // save only an hour of material for now
  historyLightning = new int[60];
  historyDisturber = new int[60];
  for (int i=0; i<60; i++) {
    historyLightning[i] = 0;
    historyDisturber[i] = 0;
  }
  
}


void draw() {
  background(0);
  noStroke();
  fill(100);
  rect(0,0,width,40);
  fill(255);
  textFont(f1);
  text("AS3935 Lightning Sensor", 20, 25);
  
  int y = 100;
  fill(150);
  textFont(f2);
  text("DATE", 20,y);
  text("TIME", 170,y);
  text("INTERVAL", 320,y);
  y+= 70;
  text("LIGHTNING CNT", 20,y);
  text("STORM HEAD DISTANCE", 150,y);
  text("DISTURBER CNT", 320,y);
  y+=60;
  text("NOISEFLOOR", 20,y);
  text("SPIKEREJECTION", 120,y);
  text("WATCHDOG", 250,y);
  text("TUNECAPACITOR", 350,y);
  
  y = 80;
  fill(255);
  textFont(f1);
  text(date, 20,y);
  text(time, 170,y);
  text(interval + " sec", 320,y);
  fill(255,0,0);
  textFont(f2);
  text(setInterval + " [io] >e" , 390,y);
  fill(255);
  textFont(f3);
  y+= 70;
  text(lightningCnt, 20,y);
  text(distance + " km", 150,y);
  text(disturberCnt, 320,y);;
  y+= 60;
  textFont(f1);
  text(pNoiseFloor, 20,y);
  text(pSpikeRejection, 120,y);
  text(pWatchdog, 250,y);
  text(pTuneCapacitor, 350,y);
  fill(255,0,0);
  textFont(f2);
  text(setNoiseFloor + " [nm]>t", 50,y);
  text(setSpike + " [sd]>t", 150,y);
  text(setWatchdog + " [qw]>t", 280,y);
  fill(255);
  y+= 60;
  textFont(f1);
  if (indoors==1) text("indoors", 20,y);
  else text("outdoors", 20, y);
  
  // draw history
  fill(200);
  if (lightningEvent) {
    lightningEvent = false;
    fill(255,0,0);
  }
  rect(0,300,width,100);
  fill(150);
  textFont(f2);
  text("LIGHTNING COUNT HISTORY", 20,420);
  stroke(0);
  noFill();
  beginShape();
  int x = 20;
  float m = (width - (2*x) ) / (float) historyLightning.length;
  for (int i=0; i<60; i++) {
    float v = historyLightning[i]*10;
    vertex(x + i*m, 400 - v);
  }
  endShape();
  
  fill(200);
  if (disturberEvent) {
    disturberEvent = false;
    fill(0,0,255);
  }
  rect(0,450,width,100);
  fill(150);
  textFont(f2);
  text("DISTURBER COUNT HISTORY", 20,570);
  stroke(0);
  noFill();
  beginShape();
  x = 20;
  m = (width - (2*x) ) / (float) historyDisturber.length;
  for (int i=0; i<60; i++) {
    float v = historyDisturber[i] * 3;
    vertex(x + i*m, 550 - v);
  }
  endShape();
  
}


// serialEvent  method is run automatically by the Processing applet
// whenever the buffer reaches the  byte value set in the bufferUntil() 
// method in the setup():

void serialEvent(Serial myPort) { 
  
  // read the serial buffer:
  String myString = myPort.readStringUntil('\n');
  // if you got any bytes other than the linefeed:
    myString = trim(myString);
 
    // split the string at the commas
    // and convert the sections into integers:
    int cnts[] = int(split(myString, ','));

    if (cnts[0] == 0) {
      
      // print out the values you got:
      for (int valNum = 1; valNum < cnts.length; valNum++) {
        print("[" + msgCol[valNum] + "] " + cnts[valNum] + "\t"); 
      }
      println();
      appendTextToFile(logFile, myString);
      
      interval = cnts[1];
      disturberCnt = cnts[2];
      lightningCnt = cnts[3];
      distance = cnts[4];
      energy = cnts[5];
      pNoiseFloor = cnts[6];
      pSpikeRejection = cnts[7];
      pWatchdog = cnts[8];
      pTuneCapacitor = cnts[9];
      
      addToLightningHistory(lightningCnt);
      addToDisturberHistory(disturberCnt);
      
    } else if (cnts[0] == 1) {
      // receive lightning events
      print("Lightning! Storm front is at ");
      print(cnts[1]);
      println("km");
      print("Energy = ");
      print(cnts[2]);
      println();
      lightningEvent = true;
    } else if (cnts[0] == 2) {
      // receive disturber events
      // print("Man-made disturber");
      disturberEvent = true;
    } else if (cnts[0] == 3) {
      println("noise level too high");
    } else if (cnts[0] == 4) {
      print("head storm distance changed to ");
      print(cnts[1]);
      println("km");
    } else {
      // other communication
      for (int valNum = 0; valNum < cnts.length; valNum++) {
        print("[" + valNum + "] " + cnts[valNum] + "\t"); 
      }
      println();
    }
  }
  
  
  void keyReleased() {
    if (key == 'e') {
      int a = setInterval;
      char b = (char) a;
      myPort.write('e');
      myPort.write(b);
      print("COMMAND\tset time interval of logs to ");
      print(a);
      print(" seconds, value = ");
      println(b);
    } 
    else if (key == 'a') {
      // tune antenna
      myPort.write('a');
    }
    else if (key == 't') {
      setThresholds();
    }
    else if (key == 'c') {
      myPort.write('c');
      println("COMMAND\trecalibrate");
    } 
    else if (key == 'v') {
      myPort.write('v');
      println("COMMAND\toutput antenna frequencies");
    } 
    else if (key == 'i') {
      setInterval+=5;
    }
    else if (key == 'o') {
      setInterval-=5;
      if (setInterval < 5) setInterval = 5;
    }
    else if (key == 'l') {
      indoors = (indoors==0) ? 1 : 0;
      myPort.write('l');
      char c = (char) indoors;
      myPort.write(c);
     println("COMMAND\ttoggle indoor/outdoor  (0=outdoor, 1=indoor)"); 
    }
    else if (key == 'n') {
      setNoiseFloor+=1;
      if(setNoiseFloor>7) setNoiseFloor=7;
    }
    else if (key == 'm') {
      setNoiseFloor-=1;
      if (setNoiseFloor < 0) setNoiseFloor = 0;
    }
    else if (key == 's') {
      setSpike+=1;
      if(setSpike>7) setSpike=7;
    }
    else if (key == 'd') {
      setSpike-=1;
      if (setSpike < 0) setSpike = 0;
    }
    else if (key == 'w') {
      setWatchdog+=1;
      if(setWatchdog>7) setWatchdog=7;
    }
    else if (key == 'q') {
      setWatchdog-=1;
      if (setWatchdog < 0) setWatchdog = 0;
    }
    else if (key == 'p') {
      saveFrame("screenshot/lightning_"+date+"_"+time+".png");
    }
  }
  
  
  void setThresholds() {
    char noisefloor = (char) setNoiseFloor;
    char spike = (char) setSpike;
    char watchdog = (char) setWatchdog;
    
    myPort.write('t');
    myPort.write(noisefloor);
    myPort.write(spike);
    myPort.write(watchdog);
    print("COMMAND\tset thresholds ");
    print(int(noisefloor));
    print(" ");
    print(int(spike));
    print(" ");
    println(int(watchdog));
  }
  
  
  /**
 * Appends text to the end of a text file located in the data directory, 
 * creates the file if it does not exist.
 * Can be used for big files with lots of rows, 
 * existing lines will not be rewritten
 */
void appendTextToFile(String filename, String text){
  date = nf(year(),4) + nf(month(),2) + nf(day(),2);
  time = nf(hour(),2) + nf(minute(),2) + nf(second(),2);
  text = date + "," + time + "," + text;
  File f = new File(dataPath(filename));
  
  if (minute() == 0) {
    saveFrame("screenshot/lightning_"+date+"_"+time+".png");
  }
  
  if(!f.exists()){
    createFile(f);
  }
  try {
    PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter(f, true)));
    out.println(text);
    out.close();
  }catch (IOException e){
      e.printStackTrace();
  }
}

/**
 * Creates a new file including all subfolders
 */
void createFile(File f){
  File parentDir = f.getParentFile();
  try{
    parentDir.mkdirs(); 
    f.createNewFile();
  }catch(Exception e){
    e.printStackTrace();
  }
}  


void addToLightningHistory(int v) {
  for (int i=0; i<historyLightning.length-1; i++) {
    historyLightning[i] = historyLightning[i+1];
  }
  historyLightning[historyLightning.length-1] = v;
}



void addToDisturberHistory(int v) {
  for (int i=0; i<historyDisturber.length-1; i++) {
    historyDisturber[i] = historyDisturber[i+1];
  }
  historyDisturber[historyDisturber.length-1] = v;
}
