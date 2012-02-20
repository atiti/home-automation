#include "SPI.h"
#include "WS2801.h"

#define NUM_PIXELS 20
#define DATA_PIN 12
#define CLK_PIN 11


uint8_t values[3*NUM_PIXELS];
WS2801 strip = WS2801(NUM_PIXELS, DATA_PIN, CLK_PIN);
uint8_t pos = 0;


void setup()
{
  Serial.begin(57600);
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
  WaitForPrefix();

  for (uint8_t i = 0; i < (3*NUM_PIXELS); i++)
  {
    while(!Serial.available());
    values[i] = Serial.read();
  }
  
  for (uint8_t i = 0; i < NUM_PIXELS; i++) {
    pos = i*3;
    strip.setPixelColor(i, Color(values[pos], values[pos+1], values[pos+2]));    
  }
  strip.show();
}

//boblightd needs to send 0x55 0xAA before sending the channel bytes
void WaitForPrefix()
{
  uint8_t first = 0, second = 0;
  while (second != 0x55 || first != 0xAA)
  {
    while (!Serial.available());
    second = first;
    first = Serial.read();
  }
}
