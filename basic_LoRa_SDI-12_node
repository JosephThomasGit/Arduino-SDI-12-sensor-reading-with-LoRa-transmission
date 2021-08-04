// A sketch that collects data from a 5TM soil moisture sensor, via SDI-12, and transmits the data as a basic LoRa radio packet
// Written by Joseph Thomas - http://github.com/JosephThomasGit/

#include <SPI.h>              // SPI library for data transfer to the LoRa radio
#include <LoRa.h>             // Basic LoRa library for simple packet transmission
#include <SDI12.h>            // SDI-12 sensor reading library

#define DATA 4 // The SDI-12 bus pin. On the Uno this can be any pin, except those used by the LoRa radio

String myCommand = ""; // Allocate space for the SDI-12 command string
String sdiResponse = ""; // Allocate space for the sensor response string

const long frequency = 915E6;  // LoRa Frequency (check for frequencies in your region and for your radio, reference the LoRaAlliance website)

const int csPin = 10;          // LoRa radio chip select (for the Dragion LoRa shield v1.4 the chip select pin is 10)
const int resetPin = 9;        // LoRa radio reset
const int irqPin = 2;          // ardware interrupt pin (optional: only needed is recieving messages from another radio)

unsigned long awake_interval = 300000; // Take a measurement and send it every 5 minutes (adjust as needed, but obey LoRa use rules)
unsigned long previousMillis = 0; // setup previousMillis millis()

SDI12 mySDI12(DATA);

void setup() {
  Serial.begin(9600);                   // Start the serial port

  LoRa.setPins(csPin, resetPin, irqPin); // Define the LoRa radio pins
  
  mySDI12.begin(); // Start the SDI-12 bus

  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed.");
    while (true);                       // Check to make sure that the LoRa radio is working, if not say that it failed and hang the code.
  }

  Serial.println("LoRa node init succeeded.");

  LoRa.onReceive(onReceive); // Setup the LoRa commands, defined below.
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  Serial.print("Starting 5TM measurement every ");
  Serial.print(awake_interval/60000);
  Serial.println(" minutes.");
}

void loop() {
//////Set Up millis time keeping///////
unsigned long currentMillis = millis(); // Current millis time

if((unsigned long)(currentMillis - previousMillis >= awake_interval)){ //
If the time passed is greater than or equal to awake_interval, take a measurement 

//////Sensor Measurement///////
  myCommand = ""; // Clear the command string
  myCommand += "1M!"; // The SDI-12 command for a 5TM sensor is [sensor_number][M][!], or in this example for a sensor addressed as "1" the command is "1M!" (You may need to address your sensor, use the example in the SDI-12 library to do this, otherwise the default sensor address is 0)
  Serial.println(myCommand);
  mySDI12.sendCommand(myCommand); // Send the command to the sensor.
  delay(500); // Give the sensor a second (this is necessary, 200 ms is recommended but I made it 500 ms to be cautious)
  mySDI12.flush(); // Clear the SDI-12 buffer

  myCommand = ""; // Clear the command string again
  myCommand += "1D0!"; // Send the data request command to sensor addressed at 1, the command for a 5TM sensor is [sensor_number][D0][!], similar as above.
  Serial.println(myCommand);
  mySDI12.sendCommand(myCommand); // Send the data request command
  delay(500); // Wait a second (necessary, similar as above)
  while (mySDI12.available()) { // Use this while loop to read the multiple output values from the 5TM sensor (sensor number / Ea / temperature)
      char c = mySDI12.read(); // Convert the output from the sensor to a char
      sdiResponse += c; // Add the char to the sdiResponse to make it easier to manipulate and send via LoRa
      delay(10);
    }
  sdiResponse.replace("\n",""); // These next few commands simply clean up the sensor output, adjust as needed.
  sdiResponse.replace("\r","");
  sdiResponse.replace("+",",");
  sdiResponse.remove(0,6);
  Serial.println(sdiResponse); // Print the sensor data that was just collected.
  myCommand = ""; 
  mySDI12.flush(); // Flush the SDI-12 buffer.
  mySDI12.clearBuffer(); // Clear it again, just to make sure (probably not necessary).
  delay(500);

////LoRa send////   
 LoRa_sendMessage(sdiResponse); // Send the sensor data via LoRa
 Serial.println("Message sent!"); 
 sdiResponse = ""; // Clear the data string
 previousMillis = millis(); // Adjust the time
}
  }


/////LoRa Functions//////

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  LoRa.disableInvertIQ();               // normal mode
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it
}

void onTxDone() {
  Serial.println("TxDone");
  LoRa_rxMode();
}

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}
