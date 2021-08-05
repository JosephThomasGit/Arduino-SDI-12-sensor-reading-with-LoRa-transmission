/* LoRa Simple Node with Multiple SDI-12 Instances
 * 
 * This sketch collects data from 4 5TM soil moisture sensors, via multiple SDI-12 instances, and sends the data as a basic raw LoRa packet
 * to a primitive gateway (Raspberry Pi) for further processing.
 * 
 * The purpose of the multiple instances of SDI-12 (despite the fact that SDI-12 can allow for connections of up to 64 devices on one data line)
 * is that this data logger is meant to exist in tandem with the current EM50 data logger and not interupt its activity. To insure that this occurs,
 * multiple instances of SDI-12 allow for the EM50 data logger to read the soil moisture data either as TTL (serial) or SDI-12, and allows the EM50
 * data logger to recognize only one sensor per 3.5 mm output cable. This should reduce confusion on the EM50 end, which is not as customizable as
 * the Arduino.
 * 
 * Due to pin interupt issues with both the SDI-12 library and the LoRa transciever, the use of the LoRa.h simple, unaddressed packet sender was 
 * necessary. One issue with this method of LoRa radio transmission is the potential for two Arduino data loggers (there will be 6 total at this
 * site) could possibly transmit at the same time, resulting in interference and corruption of data, therefore a transmission delay of ~0.5-1 minute 
 * is utilized (setup in the field using the Arduino reset button and a timer). The time total caclulated run time of this script (including LoRa air
 * time) is ~7 seconds. 
 * 
 * LoRa Radio Transmission Settings and Analysis:
 * Default settings of the LoRa.h library are a spreading factor of: 7, a bandwidth of: 125E3, and a set frequency of 914.9E6. Preamble length is 
 * by default 8, and when combined with the maximum charaters for the data from all 4 sensors (55), a total LoRa payload size of 63 bytes. A total 
 * time on air of 138.5 ms is achieved at these parameters. Across all 6 data logger sites, a total time on air per field site-wide measurement is
 * 831 ms. To abide by the LoRa 1% max duty use, and considering data collection and sending at all 6 data logger sites as a single event, the max
 * number of site-wide measurements that can be taken per hour are: 43, or one every 00:01:40.
 * 
 * Syncronicity with EM50 data loggers:
 * If both the data logger coded here and the EM50 data logger that already exist in the field site attempt to collect sensor data at the same time,
 * the data will be corrupted and not useful. Current settings for the EM50 data loggers are a measurement interval of 15 minutes, beginning at the top
 * of each hour. Minimum sensor measurement time is 200 ms, but it is safe to assume that at a total of at least 5 seconds is used to collect data from
 * all 4 sensors at each EM50 data logger site. To maximize the amount of data being collected by the data logger coded here, and reduce the potential for
 * data line interference, a sample interval of 10 minutes will be used. As long as each Arduino is reset and the millis() timer begins on a real clock minute
 * value of 1,2,3,4,6,7,8,9 [the EM50 will request data on 0 and 5 minute values: 00:00:00 / 00:15:00 / 00:30:00 / 00:45:00] there should be no interference. 
 * The estimate run time of this script is 6 seconds (this can be reduced though), and therefore the data line should be clear by the time the EM50 logger 
 * requests data.
 * 
 * Reception by "gateway":
 * To call the LoRa end tranciever, which is an Arduino with a Dragion LoRa radio, connected to a Raspberry Pi a gateway is not accurate, as the "gateway"
 * does not function like a traditional LoRa gateway. The reason that a normal packet forwarding gateway, with multiple channels, was not used, was because some
 * data analysis will occur on the Raspberry Pi before the data is sent to the cloud via MQTT (more details on this on the "gateway" sketch). More importantly
 * for this specific node sketch, since the radio packets are sent raw on a public access freqency, field site numbers are used to code the data and allow the
 * gateway to filter random radio transmissions and accept only those sent by the nodes. The gateway is specifically looking for a site number of 1,2,3,4,5,or 6
 * in the first position of the string being transmitted, and followed immediately after by a comma. It is important to change the site number for each site to prevent 
 * a mix up of data.
 * 
 * 
 * created:  2021
 * by:       Joseph Thomas
 * position: Physical Science Technician, United States Geological Survey, Flagstaff Arizona
 *
 */

#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <SDI12.h>

#define SITE_NUMBER 1        // MAKE SURE THE CHANGE THIS FOR EACH FIELD SITE

#define SDI_Sensor_1 A1 
#define SDI_Sensor_2 A2
#define SDI_Sensor_3 A3
#define SDI_Sensor_4 A4

String myCommand = "";
String sdiResponse_1 = "";
String sdiResponse_2 = "";
String sdiResponse_3 = "";
String sdiResponse_4 = "";
String payload = "";

const long frequency = 914.9E6;  // LoRa Frequency 914.9 for US uplink Mhz

const int csPin = 10;          // LoRa radio chip select
const int resetPin = 9;        // LoRa radio reset
const int irqPin = 2;          // change for your board; must be a hardware interrupt pin

unsigned long awake_interval = 300000; // Measure every 5 minutes
unsigned long previousMillis = 0; // millis() returns unsigned long

SDI12 SDI12_1(SDI_Sensor_1);
SDI12 SDI12_2(SDI_Sensor_2);
SDI12 SDI12_3(SDI_Sensor_3);
SDI12 SDI12_4(SDI_Sensor_4);

void setup() {
  Serial.begin(9600);                   // initialize serial

  LoRa.setPins(csPin, resetPin, irqPin);
  
  SDI12_1.begin();
  SDI12_2.begin();
  SDI12_3.begin();
  SDI12_4.begin();

  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa node init succeeded.");

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  Serial.print("Starting measurements at site ");
  Serial.print(SITE_NUMBER);
  Serial.print(" every ");
  Serial.print(awake_interval/60000);
  Serial.println(" minutes."); 
}

void loop() {
//////Set Up millis time keeping///////
unsigned long currentMillis = millis(); //grab current millis time


if((unsigned long)(currentMillis - previousMillis >= awake_interval)){ //If the time passed is greater than or equal to awake_interval, take a measurement 
//////Clean up the SDI line before measuring/////// 
  SDI12_1.setActive();
  SDI12_1.flush();
  SDI12_1.clearBuffer();
//////Sensor 1///////
  myCommand = "";
  myCommand += "1M!";
  Serial.println(myCommand);
  SDI12_1.sendCommand(myCommand);
  delay(500);
  SDI12_1.flush();

  myCommand = "";
  myCommand += "1D0!";
  Serial.println(myCommand);
  SDI12_1.sendCommand(myCommand);
  delay(500);
  while (SDI12_1.available()) {
      char c = SDI12_1.read();
      sdiResponse_1 += c;
      delay(10);
    }
  sdiResponse_1.replace("\n","");
  sdiResponse_1.replace("\r","");
  sdiResponse_1.replace("+",",");
  sdiResponse_1.remove(0,6);
  Serial.println(sdiResponse_1);
  myCommand = "";
  SDI12_1.flush();
  SDI12_1.clearBuffer();
  SDI12_1.forceHold();
  delay(500);

//////Sensor 2///////
  SDI12_2.setActive();
  myCommand = "";
  myCommand += "2M!";
  Serial.println(myCommand);
  SDI12_2.sendCommand(myCommand);
  delay(500);
  SDI12_2.flush();

  myCommand = "";
  myCommand += "2D0!";
  Serial.println(myCommand);
  SDI12_2.sendCommand(myCommand);
  delay(500);
  while (SDI12_2.available()) {
      char c = SDI12_2.read();
      sdiResponse_2 += c;
      delay(10);
    }
  sdiResponse_2.replace("\n","");
  sdiResponse_2.replace("\r","");
  sdiResponse_2.replace("+",",");
  sdiResponse_2.remove(0,6);
  Serial.println(sdiResponse_2);
  myCommand = "";
  SDI12_2.flush();
  SDI12_2.clearBuffer();
  SDI12_2.forceHold();
  delay(500);

//////Sensor 3///////
  SDI12_3.setActive();
  myCommand = "";
  myCommand += "3M!";
  Serial.println(myCommand);
  SDI12_3.sendCommand(myCommand);
  delay(500);
  SDI12_3.flush();

  myCommand = "";
  myCommand += "3D0!";
  Serial.println(myCommand);
  SDI12_3.sendCommand(myCommand);
  delay(500);
  while (SDI12_3.available()) {
      char c = SDI12_3.read();
      sdiResponse_3 += c;
      delay(10);
    }
  sdiResponse_3.replace("\n","");
  sdiResponse_3.replace("\r","");
  sdiResponse_3.replace("+",",");
  sdiResponse_3.remove(0,6);
  Serial.println(sdiResponse_3);
  myCommand = "";
  SDI12_3.flush();
  SDI12_3.clearBuffer();
  SDI12_3.forceHold();
  delay(500);

//////Sensor 4///////
  SDI12_4.setActive();
  myCommand = "";
  myCommand += "4M!";
  Serial.println(myCommand);
  SDI12_4.sendCommand(myCommand);
  delay(500);
  SDI12_4.flush();

  myCommand = "";
  myCommand += "4D0!";
  Serial.println(myCommand);
  SDI12_4.sendCommand(myCommand);
  delay(500);
  while (SDI12_4.available()) {
      char c = SDI12_4.read();
      sdiResponse_4 += c;
      delay(10);
    }
  sdiResponse_4.replace("\n","");
  sdiResponse_4.replace("\r","");
  sdiResponse_4.replace("+",",");
  sdiResponse_4.remove(0,6);
  Serial.println(sdiResponse_4);
  myCommand = "";
  SDI12_4.flush();
  SDI12_4.clearBuffer();
  SDI12_4.forceHold();
  delay(500);


////LoRa send////   
 payload += SITE_NUMBER;
 payload += ",";
 payload += sdiResponse_1;
 payload += ",";
 payload += sdiResponse_2;
 payload += ",";
 payload += sdiResponse_3;
 payload += ",";
 payload += sdiResponse_4;
 Serial.println(payload);
 LoRa_sendMessage(payload); // send a message
 Serial.println("Message sent!");
 sdiResponse_1 = "";
 sdiResponse_2 = "";
 sdiResponse_3 = "";
 sdiResponse_4 = "";
 payload = "";
 previousMillis = millis();
}
  }


/////LoRa Functions//////
void LoRa_rxMode(){
  LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.receive();                       // set receive mode
}

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

void onReceive(int packetSize) {
  String message = "";
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  Serial.print("Node Receive: ");
  Serial.println(message);
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
