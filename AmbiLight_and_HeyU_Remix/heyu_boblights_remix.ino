#include <RCSwitch.h>
#include "SPI.h"
#include "Adafruit_WS2801.h"

#define NUM_PIXELS 20
#define DATA_PIN 8
#define CLK_PIN 9

char *switches[] = {"10101", // Housecode A
                    "00101", // Housecode B
                    "00000", // HOusecode C
                    "00000", // HOusecode D
                    "00000", // HOusecode E
                    "00000", // HOusecode F
                    "00000", // HOusecode G
                    "00000", // HOusecode H
                    "00000", // HOusecode I
                    "00000", // HOusecode J
                    "00000", // HOusecode K
                    "00000", // HOusecode L
                    "00000", // HOusecode M
                    "00000", // HOusecode N
                    "00000", // HOusecode O
                    "00000"}; // HOusecode P
                 
#define FUNC_ALL_UNITS_OFF 0
#define FUNC_ALL_LIGHTS_ON 1
#define FUNC_ON 2
#define FUNC_OFF 3
#define FUNC_DIM 4
#define FUNC_ALL_LIGHTS_OFF 5
#define FUNC_EXTENDED_CODE 6
#define FUNC_HAIL_REQ 7
#define FUNC_HAIL_ACK 8
#define FUNC_PRESET_DIM_1 9
#define FUNC_PRESET_DIM_2 10
#define FUNC_EXT_DT 11
#define FUNC_STATUS_ON 12
#define FUNC_STATUS_OFF 13
#define FUNC_STATUS_REQ 14
                 
#define STATE_LISTEN 0
#define STATE_HEYU_ONOFF 1
#define STATE_HEYU_INFO 2
#define STATE_BOBLIGHTS 3
                 
int state = STATE_LISTEN;
int nb = 0;
char buff[100];
char c = 0, chksum,hc,dc;
char housecode = 0;
char function = 0;
char devicecode = 0;
char dimamount = 0;
long lastcmd = 0;

char lookup[16] = {0x06, 0x0e, 0x02, 0x0a, 0x01, 0x09, 0x05, 0x0d, 0x07, 0x0f, 0x03, 0x0b, 0x00, 0x08, 0x04, 0x0c};

uint8_t values[3*NUM_PIXELS];
Adafruit_WS2801 strip = Adafruit_WS2801(NUM_PIXELS, DATA_PIN, CLK_PIN);
uint8_t pos = 0;
RCSwitch mySwitch = RCSwitch();

char lookup_v(char v) {
 char i = 0;
 for(i=0;i<16;i++)
   if (lookup[i] == v)
    return i;
 return 0; 
}

void setup() {

  Serial.begin(115200);
  // Transmitter is connected to Arduino Pin #10  
  mySwitch.enableTransmit(3);
  
  strip.begin();
  strip.show();
  
}

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

void loop() {
  switch(state) {
    case STATE_LISTEN:
      if (Serial.available()) {
        c = Serial.read();
        buff[nb] = c;
        nb++;
        if (nb == 1) {
          if (c == 0x8b) { // CM11 status request
           state = STATE_HEYU_INFO;
           nb = 0;
          } else if (c == 0x55) {
           state = STATE_BOBLIGHTS; 
          } else {
           state = STATE_HEYU_ONOFF; 
          }
        }
        lastcmd = millis();
      }
      break;
    case 1: // Interpret command from PC
      while (!Serial.available());
      buff[nb] = Serial.read();
      nb++;
      
      chksum = 0;
      for(c = 0;c<nb;c++) {
        chksum += buff[c]; 
      }
      chksum &= 0xff;
    
      if ((buff[0] & 0x01) == 0) { // Standard transmission (we dont deal with extended)
        dimamount = (buff[0] & 0xf8) >> 3;
        housecode = (buff[1] & 0xf0) >> 4;
        function = buff[1] & 0x0f;
        
        if (buff[0] & 0x02) { // Function code
         hc = lookup_v(housecode); // Slow function to lookup stuffz
         dc = lookup_v(devicecode)+1;
              
          switch(function) {
             case FUNC_ON: // Switch outlet off
               mySwitch.switchOn(switches[hc], dc); 
               break;
             case FUNC_OFF: // Switch outlet off
               mySwitch.switchOff(switches[hc], dc);
               break;
            default:
             break;
          } 
        } else {
          devicecode = function;
        }
      }
  
      Serial.write(chksum);
      Serial.write(0xFF);
      while (!Serial.available()) {}; // Wait for reply to our checksum
      c = Serial.read();
      Serial.write(0x55); // We answer OK, whatever happens
      Serial.write(0xFF);
      nb = 0;
      state = STATE_LISTEN;
      break;
    case STATE_HEYU_INFO: // Status not implemented
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write((uint8_t)0x00);
      Serial.write(0xFF);

      state = STATE_LISTEN;
      break;
    case STATE_BOBLIGHTS:
      while(!Serial.available());
      c = Serial.read();
    
      for (uint8_t i = 0; i < (3*NUM_PIXELS); i++) {
        while(!Serial.available());
        values[i] = Serial.read();
      }
  
      for (uint8_t i = 0; i < NUM_PIXELS; i++) {
        pos = i*3;
        strip.setPixelColor(i, Color(values[pos], values[pos+1], values[pos+2]));    
      }
      strip.show();
      state = STATE_LISTEN;
      nb = 0;
      Serial.write(0xFF);
      break;
    default:
      break;  
  }  

  if (millis() - lastcmd > 1000) {
    Serial.write(0xFF);
    lastcmd = millis(); 
  }

}
