// gsr 

#include <SoftwareWire.h>
#include "registers.h"


#define SENSOR_RATE 1000
#define LONG_THERESHOLD 1500

#define SDA_0 44
#define SCL_0 45
#define SDA_1 46 
#define SCL_1 47
#define SDA_2 48
#define SCL_2 49
#define SDA_3 50
#define SCL_3 51
#define SDA_4 52
#define SCL_4 53

// button related stuff
int buttons[3] = {8,9,10};
int current[3];
long millis_held[3];
long secs_held[3];
long prev_secs_held[3];
byte previous[3] = {HIGH,HIGH,HIGH};
unsigned long firstTime[3];

unsigned long previous_sensor = 0;
// parser related stuff
String message = "";       
bool got_message = false; 

SoftwareWire wire0(SDA_0, SCL_0);
SoftwareWire wire1(SDA_1, SCL_1);
SoftwareWire wire2(SDA_2, SCL_2);
SoftwareWire wire3(SDA_3, SCL_3);
SoftwareWire wire4(SDA_4, SCL_4);

// drv related fuckery 
// soon to be moved if working
uint8_t readRegister8(SoftwareWire * _wire, uint8_t reg) {
  uint8_t x;

  _wire->beginTransmission(DRV2605_ADDR);
  _wire->write((byte)reg);
  _wire->endTransmission();
  _wire->requestFrom((byte)DRV2605_ADDR, (byte)1);
  x = _wire->read();

  return x;
}

void writeRegister8(SoftwareWire * _wire, uint8_t reg, uint8_t val) {
  _wire->beginTransmission(DRV2605_ADDR);
  _wire->write((byte)reg);
  _wire->write((byte)val);
  _wire->endTransmission();
}

void drv_init(SoftwareWire * _wire, bool lra){
  _wire->begin();

  uint8_t id = readRegister8(_wire, DRV2605_REG_STATUS);
  //Serial.print("Status 0x"); Serial.println(id, HEX);

  writeRegister8(_wire, DRV2605_REG_MODE, 0x00); // out of standby
  writeRegister8(_wire, DRV2605_REG_RTPIN, 0x00); // no real-time-playback
  writeRegister8(_wire, DRV2605_REG_WAVESEQ1, 1); // strong click
  writeRegister8(_wire, DRV2605_REG_WAVESEQ2, 0); // end sequence
  writeRegister8(_wire, DRV2605_REG_OVERDRIVE, 0); // no overdrive
  writeRegister8(_wire, DRV2605_REG_SUSTAINPOS, 0);
  writeRegister8(_wire, DRV2605_REG_SUSTAINNEG, 0);
  writeRegister8(_wire, DRV2605_REG_BREAK, 0);
  writeRegister8(_wire, DRV2605_REG_AUDIOMAX, 0x64);

  // Set mode to internal trigger
  writeRegister8(_wire, DRV2605_REG_MODE, 0);

  // set library one 
  writeRegister8(_wire, DRV2605_REG_LIBRARY, 1);

  // turn off N_ERM_LRA
  writeRegister8(_wire, DRV2605_REG_FEEDBACK, readRegister8(_wire, DRV2605_REG_FEEDBACK) & 0x7F);
  // turn on ERM_OPEN_LOOP
  writeRegister8(_wire, DRV2605_REG_CONTROL3, readRegister8(_wire, DRV2605_REG_CONTROL3) | 0x20);

}

void drv_lib(SoftwareWire * _wire, uint8_t lib){
    writeRegister8(_wire, DRV2605_REG_LIBRARY, lib);
}

void drv_wave(SoftwareWire * _wire, uint8_t slot, uint8_t item){
    writeRegister8(_wire, DRV2605_REG_WAVESEQ1+slot, item);
}

void drv_go(SoftwareWire * _wire){
    writeRegister8(_wire, DRV2605_REG_GO, 1);
}

void drv_stop(SoftwareWire * _wire){
    writeRegister8(_wire, DRV2605_REG_GO, 0);
}


void vibrate(SoftwareWire * wire, uint8_t n, String message){
    // i=3 because of prefix 
    for(int i=3; i<n; i++)
        // i+1 to skip the comma
        drv_wave(wire, i, int(message.charAt(i + 1)));

    // close up sequence
    drv_wave(wire, n + 1 , 0);
    drv_go(wire);
    
}

// buffer incoming messages
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the message:
    message += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      got_message = true;
    }
  }
}

// poll and debounce buttons
void btn(){
	for (int i = 0; i < 3; i++){
		current[i] = digitalRead(buttons[i]);
		if (current[i] == LOW && previous[i] == HIGH && (millis() - firstTime[i]) > 200) { firstTime[i] = millis(); }
		millis_held[i] = (millis() - firstTime[i]);
		if (millis_held[i] > 100) {
			if (current[i] == HIGH && previous[i] == LOW) {
				if (millis_held[i] >= LONG_THERESHOLD) {
            Serial.print('b');
					  Serial.print(i);
            Serial.println("l");
				}
				else{
            Serial.print('b');
					  Serial.print(i);
            Serial.println("s");
				}
			}
			previous[i] = current[i];
		}
	}
}

void gsr(){
  long randNumber = random(0, 1023);
  Serial.print('g');
  Serial.println(randNumber);
}

void hr(){
  long randNumber = random(70, 130);
  Serial.print('h');
  Serial.println(randNumber);
}

void parse(String message){
    // message is for amplifier control
    if(message.charAt(0) == 'a'){
        uint8_t value = (byte)message.charAt(1);
        int mapped = map(value, 0, 255, -28, 30);

    // message is for haptic subsystem
    } else if(message.charAt(0) == 'h'){
        Serial.print('k');
        Serial.println(message);
        uint8_t which = atoi(message.charAt(1));
        uint8_t n = atoi(message.charAt(2));

        if(which == 0)
            vibrate(&wire0, n, message);

        else if(which == 1)
            vibrate(&wire1, n, message);

        else if(which == 2)
            vibrate(&wire2, n, message);


    }
}

void setup() {
  Serial.begin(112500);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);

  drv_init(&wire0, false);
  drv_init(&wire1, false);
  drv_init(&wire2, false);
  randomSeed(analogRead(0));
}



void loop(){
    btn();

    unsigned long currentMillis = millis();

   // time to toggle LED on Pin 12?
   if ((unsigned long)(currentMillis - previous_sensor) >= SENSOR_RATE) {
      hr();
      gsr();
      previous_sensor = currentMillis;
   }

    if(got_message){
        parse(message);
        message = "";
        got_message = false;
    }      
}
