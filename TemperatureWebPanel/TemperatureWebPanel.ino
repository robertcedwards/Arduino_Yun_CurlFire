/*
This is a sketch to show how to use input data from a Tinkerit shield to curl -k -d up to Firebase for realtime sensor data.
This also creates a Yun Bridge server and client in order to debug locally
 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h> 
#include <TinkerKit.h>
//#include <SoftwareSerial.h>
unsigned long bitHolder1 = 0;
unsigned long bitHolder2 = 0;
unsigned int bitCount = 0;
unsigned long cardChunk1 = 0;
unsigned long cardChunk2 = 0;
int parity = 0;
long previousMillis = 0; // will store last time LCD was updated
long interval = 3000; // interval at which to reset display (milliseconds)

// Begin Code for DATA0 and DATA1 Interrupts to get the binary data from the card
// For whatever reason, when wired to the micro DATA1 and DATA0 need to be reversed
void DATA1(void) {  
    bitCount++;
    if(bitCount < 23) {
      bitHolder1 = bitHolder1 << 1;
    }
    else {
      bitHolder2 = bitHolder2 << 1;
    }
}

void DATA0(void) {
   bitCount++;
   if(bitCount < 23) {
      bitHolder1 = bitHolder1 << 1;
      bitHolder1 |= 1;
   }
   else {
     bitHolder2 = bitHolder2 << 1;
     bitHolder2 |= 1;
   }
}
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
  Serial.begin(57600);
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
  clearinterrupts();
  
  attachInterrupt(0, DATA0, RISING);  // DATA0 (green) on HID reader, Pin 2 on Arduino
  attachInterrupt(1, DATA1, RISING);  // DATA1 (white) on HID reader, Pin 3 on Arduino
  delay(10);


  // Turn off onboard LED to indicate ready state
  digitalWrite(17, LOW);
    
  Serial.println("Huddle_1");
  
  //Reset and get ready for a card
  bitCount = 0; bitHolder1 = 0; bitHolder2 = 0;

}

void loop() {
  
  if (millis() - previousMillis > interval) {
      bitCount = 0; bitHolder1 = 0; bitHolder2 = 0; //in case something went wrong, clear the buffers
      previousMillis = millis(); // remember the last time we blinked the LED
//      clearLCD();
//      LCD.print("Present Card or Enter Pin");
      
    }
    
    // Check if bits received are a valid length to be a card
    if (bitCount >= 26) {
        delay(100);
        
        // Call function to get our two chunks of card values
        getCardValues();
        
        Serial.print("Card ID: ");
        Serial.print(cardChunk1, HEX);
        Serial.println(cardChunk2, HEX);

        //Reset and get ready for another card
        bitCount = 0; bitHolder1 = 0; bitHolder2 = 0;
        previousMillis = millis();
        delay(100); // Extend this if you want to see the card value on the LCD screen longer
    }
  
  
  // Setup the Tinkerit Kit here
 String tempstring = String(F);
 String brightnessstring = String(brightnessVal);
      
      F = therm.readFahrenheit();  	// Reading the temperature in Fahrenheit degrees and store in the F variable
      brightnessVal = ldr.read(); 
      val = digitalRead(inputPin);  // read PIR pin value
        if (val == HIGH) {            // check if the input is HIGH
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
        Process p;        // Create a process and call it "p"
    p.runShellCommand("curl -k -X PATCH https://yun.firebaseio.com/Huddles/one/.json -d '{ \"motion\" : \"Detected!\" }'");  
      
      // We only want to print on the output change, not state
      pirState = HIGH;
    }
  } else {
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

}

// Function to clear interrupts and prepare them for use
void clearinterrupts () {
  // the interrupt in the Atmel processor messes up the first negative pulse as the inputs are already high,
  // so this gives a pulse to each reader input line to get the interrupts working properly.
  // Then clear out the reader variables.
  // The readers are open collector sitting normally at a one so this is OK
  for(int i = 2; i < 4; i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH); // enable internal pull up causing a one
    digitalWrite(i, LOW); // disable internal pull up causing zero and thus an interrupt
    pinMode(i, INPUT);
    digitalWrite(i, HIGH); // enable internal pull up
  }
  delay(10);
}
void getCardValues() {
  switch (bitCount) {
    case 26:
        // Example of full card value
        // |>   preamble   <| |>   Actual card value   <|
        // 000000100000000001 11 111000100000100100111000
        // |> write to chunk1 <| |>  write to chunk2   <|
        
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 2){
            bitWrite(cardChunk1, i, 1); // Write preamble 1's to the 13th and 2nd bits
          }
          else if(i > 2) {
            bitWrite(cardChunk1, i, 0); // Write preamble 0's to all other bits above 1
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 20)); // Write remaining bits to cardChunk1 from bitHolder1
          }
          if(i < 20) {
            bitWrite(cardChunk2, i + 4, bitRead(bitHolder1, i)); // Write the remaining bits of bitHolder1 to cardChunk2
          }
          if(i < 4) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i)); // Write the remaining bit of cardChunk2 with bitHolder2 bits
          }
        }
        break;

    case 27:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 3){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 3) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 19));
          }
          if(i < 19) {
            bitWrite(cardChunk2, i + 5, bitRead(bitHolder1, i));
          }
          if(i < 5) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 28:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 4){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 4) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 18));
          }
          if(i < 18) {
            bitWrite(cardChunk2, i + 6, bitRead(bitHolder1, i));
          }
          if(i < 6) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 29:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 5){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 5) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 17));
          }
          if(i < 17) {
            bitWrite(cardChunk2, i + 7, bitRead(bitHolder1, i));
          }
          if(i < 7) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 30:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 6){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 6) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 16));
          }
          if(i < 16) {
            bitWrite(cardChunk2, i + 8, bitRead(bitHolder1, i));
          }
          if(i < 8) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 31:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 7){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 7) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 15));
          }
          if(i < 15) {
            bitWrite(cardChunk2, i + 9, bitRead(bitHolder1, i));
          }
          if(i < 9) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 32:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 8){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 8) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 14));
          }
          if(i < 14) {
            bitWrite(cardChunk2, i + 10, bitRead(bitHolder1, i));
          }
          if(i < 10) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 33:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 9){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 9) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 13));
          }
          if(i < 13) {
            bitWrite(cardChunk2, i + 11, bitRead(bitHolder1, i));
          }
          if(i < 11) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 34:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 10){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 10) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 12));
          }
          if(i < 12) {
            bitWrite(cardChunk2, i + 12, bitRead(bitHolder1, i));
          }
          if(i < 12) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 35:        
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 11){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 11) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 11));
          }
          if(i < 11) {
            bitWrite(cardChunk2, i + 13, bitRead(bitHolder1, i));
          }
          if(i < 13) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 36:
       for(int i = 19; i >= 0; i--) {
          if(i == 13 || i == 12){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 12) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 10));
          }
          if(i < 10) {
            bitWrite(cardChunk2, i + 14, bitRead(bitHolder1, i));
          }
          if(i < 14) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 37:
       for(int i = 19; i >= 0; i--) {
          if(i == 13){
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 9));
          }
          if(i < 9) {
            bitWrite(cardChunk2, i + 15, bitRead(bitHolder1, i));
          }
          if(i < 15) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;
  }
  return;
}

  
  



