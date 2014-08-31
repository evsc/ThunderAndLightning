

import processing.serial.*;
import java.io.BufferedWriter;
import java.io.FileWriter;

Serial myPort;  // Create object from Serial class
String logFile = "lightning_log.csv";

String msgCol[] = {"mode", "intv", "dist", "light", "km", "nfloor", "spike", "watchd"};

void setup() {
  size(500,600);
  
  println(Serial.list());
  String portName = Serial.list()[32];
  myPort = new Serial(this, portName, 9600);
  
  // read bytes into a buffer until you get a linefeed (ASCII 10):
  myPort.bufferUntil('\n');

}


void draw() {
  background(0);
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
    } else if (cnts[0] == 1) {
      // receive lightning events
      print("Lightning! Storm front is at ");
      print(cnts[1]);
      println("km");
    } else if (cnts[0] == 2) {
      // receive disturber events
      // print("Man-made disturber");
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
      myPort.write('e');
      myPort.write(5);
      println("set time interval of logs to 5 seconds");
    } 
    else if (key == 't') {
      setThresholds();
    }
  }
  
  
  void setThresholds() {
    char noisefloor = 2;
    char spike = 3;
    char watchdog = 2;
    
    myPort.write('t');
    myPort.write(noisefloor);
    myPort.write(spike);
    myPort.write(watchdog);
    println("set thresholds");
  }
  
  
  /**
 * Appends text to the end of a text file located in the data directory, 
 * creates the file if it does not exist.
 * Can be used for big files with lots of rows, 
 * existing lines will not be rewritten
 */
void appendTextToFile(String filename, String text){
  String date = nf(year(),4) + nf(month(),2) + nf(day(),2);
  String time = nf(hour(),2) + nf(minute(),2) + nf(second(),2);
  text = date + "," + time + "," + text;
  File f = new File(dataPath(filename));
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
