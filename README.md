# basic_Arduino_LoRa_with_SDI-12
Sketches and project wiki for Arduino based SDI-12 data logger with basic LoRa communication.

This code utilizes an Arduino Uno and Dragino LoRa Shield v1.4 to read data from a METER 5TM soil moisture sensor and send it as a raw LoRa packet.

Check out the wiki for more information. <https://github.com/JosephThomasGit/basic_Arduino_LoRa_with_SDI-12/wiki/Getting-Started>

Files:
- basic_LoRa_SDI-12_node.ino = Simple 1 sensor (5TM from METER) read and send via LoRa
- LoRaSimpleNode_simplified_multiple_SDI_instances.ino = 4 sensor (5TM from METER) read and send via LoRa, using multiple SDI-12 instances (read the info on the sketch for more details)
