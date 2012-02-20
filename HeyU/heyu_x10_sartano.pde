#include <RCSwitch.h>

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
                 
int state = 0;
int nb = 0;
char buff[100];
char c = 0, chksum,hc,dc;
char housecode = 0;
char function = 0;
char devicecode = 0;
char dimamount = 0;

char lookup[16] = {0x06, 0x0e, 0x02, 0x0a, 0x01, 0x09, 0x05, 0x0d, 0x07, 0x0f, 0x03, 0x0b, 0x00, 0x08, 0x04, 0x0c};

RCSwitch mySwitch = RCSwitch();

char lookup_v(char v) {
 char i = 0;
 for(i=0;i<16;i++)
   if (lookup[i] == v)
    return i;
 return 0; 
}

void setup() {

  Serial.begin(9600);
  // Transmitter is connected to Arduino Pin #10  
  mySwitch.enableTransmit(7);
  // Optional set pulse length.
  mySwitch.setPulseLength(320);
    // Optional set number of transmission repetitions.
  mySwitch.setRepeatTransmit(15);
  
}

void loop() {
  switch(state) {
    case 0:
      if (Serial.available()) {
        c = Serial.read();
        buff[nb] = c;
        nb++;
        if (nb == 2) {
          state = 1;
        } else if (nb == 1) {
          if (c == 0x8b) { // CM11 status request
           state = 2;
           nb = 0;
          }
        }
      }
      break;
    case 1: // Interpret command from PC
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
  
      Serial.print(chksum, BYTE);
      while (!Serial.available()) {}; // Wait for reply to our checksum
      c = Serial.read();
      if (c == 0x00) {
        Serial.print(0x55, BYTE); // We answer OK, whatever happens
      }
      nb = 0;
      state = 0;
      break;
    case 2: // Status not implemented
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      Serial.print(0x00, BYTE);
      state = 0;
      break;
    default:
      break;  
  }  
}
