/*
This is a sketch to show how to use input data from a Tinkerit shield to curl -k -d up to Firebase for realtime sensor data.
This also creates a Yun Bridge server and client in order to debug locally
 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h> 
#include <TinkerKit.h>

String startString;
long hits = 0;
int F;
int brightnessVal;
TKThermistor therm(I0);       // creating the object 'therm' that belongs to the 'TKThermistor' class
TKLightSensor ldr(I1);	//create the "ldr" object on port I1
int pirState = LOW;
int inputPin = 2;
int val = 0;
// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;

void setup() {
  Serial.begin(9600);
  // Bridge startup
  pinMode(13,OUTPUT);
  pinMode(inputPin,INPUT);
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
 String tempstring = String(F);
 String brightnessstring = String(brightnessVal);
      
      F = therm.readFahrenheit();  	// Reading the temperature in Fahrenheit degrees and store in the F variable
      brightnessVal = ldr.read(); 
      val = digitalRead(inputPin);  // read PIR pin value
        if (val == HIGH) {            // check if the input is HIGH
    digitalWrite(ledPin, HIGH);  // turn LED ON
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
        Process p;        // Create a process and call it "p"
    p.runShellCommand("curl -k -X PATCH https://yun.firebaseio.com/Huddles/one/.json -d '{ \"motion\" : \"Detected!\" }'");  
      
      // We only want to print on the output change, not state
      pirState = HIGH;
    }
  } else {
    digitalWrite(ledPin, LOW); // turn LED OFF
    if (pirState == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
  Process p;        // Create a process and call it "p"
    p.runShellCommand("curl -k -X PATCH https://yun.firebaseio.com/Huddles/one/.json -d '{ \"motion\" : \"Not Detected!\" }'");  
            pirState = LOW;
    }
  }

  Process p;        // Create a process and call it "p"
    p.runShellCommand("curl -k -X PATCH https://yun.firebaseio.com/Huddles/one/.json -d '{ \"temp\" : \" "+tempstring+" \", \"brightness\" :  \" "+brightnessstring+" \"}'");  
 delay(500);

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
    //  F = therm.readFahrenheit();  	// Reading the temperature in Fahrenheit degrees and store in the F variable
      //brightnessVal = ldr.read();            
      
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
 // String tempstring = String(F);
 // String brightnessstring = String(brightnessVal);

 // Process p;        // Create a process and call it "p"
//p.runShellCommand("curl -k -X PATCH https://yun.firebaseio.com/Huddles/1/.json -d '{ \"temp\" : \" " + tempstring + " \", \"brightness\" :  \" " + brightnessstring + " \"}'");  
// p.runShellCommand("curl -u " + username + ":" + password + " \"https://mail.google.com/mail/feed/atom/" + label + "\" -k --silent |grep -o \"<fullcount>[0-9]*</fullcount>\" |grep -o \"[0-9]*\"");

//p.runShellCommand("curl -k -X POST https://yun.firebaseio.com/Names/.json -d '{ \"first\" : \"firstname\",\"last\" : \"lastname\"}'");  

}

  
  



