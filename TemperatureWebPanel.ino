/*
  Temperature web interface
 
 This example shows how to serve data from an analog input  
 via the Arduino Yún's built-in webserver using the Bridge library.
 	
 The circuit:
 * TMP36 temperature sensor on analog pin A1
 * SD card attached to SD card slot of the Arduino Yún
 
 Prepare your SD card with an empty folder in the SD root 
 named "arduino" and a subfolder of that named "www". 
 This will ensure that the Yún will create a link 
 to the SD to the "/mnt/sd" path.
 
 In this sketch folder is a basic webpage and a copy of zepto.js, a 
 minimized version of jQuery.1`  When you upload your sketch, these files
 will be placed in the /arduino/www/TemperatureWebPanel folder on your SD card.
 
 You can then go to http://arduino.local/sd/TemperatureWebPanel
 to see the output of this sketch.
 
 You can remove the SD card while the Linux and the 
 sketch are running but be careful not to remove it while
 the system is writing to it.
 
 created  6 July 2013
 by Tom Igoe
 
 This example code is in the public domain.
 
 http://arduino.cc/en/Tutorial/TemperatureWebPanel
 
 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h> 
#include <TinkerKit.h>

String startString;
long hits = 0;
int F;
TKThermistor therm(I0);       // creating the object 'therm' that belongs to the 'TKThermistor' class
TKLightSensor ldr(I1);	//create the "ldr" object on port I1
String brightnessVal;
String temp;  
String newtemp;
// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;

void setup() {
  Serial.begin(9600);
  // Bridge startup
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
  // get the time that this sketch started:
  Process startTime;
  startTime.runShellCommand("date");
  while(startTime.available()) {
    char c = startTime.read();
    startString += c;
  }
}

void loop() {
  // Setup the Tinkerit Kit here


  // Get clients coming from server
  YunClient client = server.accept();
  // There is a new client?
  if (client) {
    // read the command
    String command = client.readString();
    command.trim();        //kill whitespace
    Serial.println(command);
    // is "temperature" command?
    if (command == "temperature") {

      // get the time from the server:
      Process time;
      time.runShellCommand("date");
      String timeString = "";
      while(time.available()) {
        char c = time.read();
        timeString += c;
      }

      Serial.println(timeString);
      F = therm.readFahrenheit();  	// Reading the temperature in Fahrenheit degrees and store in the F variable
      int brightnessVal = ldr.read();            
      
      // print the temperature:
      client.print("Current time on the Yún: ");
      client.println(timeString);
      client.print("<br>Current temperature: ");
      client.println(F);             // debug value
      client.print(" degrees F");
      client.print("It's this bright: ");
      client.print(brightnessVal);
      client.print("<br>This sketch has been running since ");
      client.print(startString);
      client.print("<br>Hits so far: ");
      client.print(hits);
      client.print("GET /firebaseTest.php");//The other important bit
      client.print(brightnessVal);//This is the variable you'll be passing
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println("User-Agent: Arduino");
      client.println();

    }

    // Close connection and free resources.
    client.stop();
    hits++;
  }

  delay(50); // Poll every 50ms
  String tempstring = String(F);
  String brightnessstring = String(brightnessVal);

  Process p;        // Create a process and call it "p"
p.runShellCommand("curl -k -X PATCH https://yun.firebaseio.com/Sensors/Huddle_1/.json -d '{ \"temp\" : \" " + tempstring + " \", \"brightness\" :  \" " + brightnessstring + " \"}'");  
// p.runShellCommand("curl -u " + username + ":" + password + " \"https://mail.google.com/mail/feed/atom/" + label + "\" -k --silent |grep -o \"<fullcount>[0-9]*</fullcount>\" |grep -o \"[0-9]*\"");

//p.runShellCommand("curl -k -X POST https://yun.firebaseio.com/Names/.json -d '{ \"first\" : \"firstname\",\"last\" : \"lastname\"}'");  

}

  




