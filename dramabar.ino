#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma


unsigned long expTime = 3600000;              //Time in ms for mood expiration (eg: every X ms mood goes 1 point nearer to normal)
unsigned long expButton = 10000;              //Time in ms in which all button presses after the first one get ignored
unsigned long now = 0, button_now = 0;        //Time in ms now (measured from boot), once for mood expiration, once for button timeout
unsigned long prev = 0, button_prev = 0;    //Time in ms, from last run of loop, t_prev for mood, button_prev for button timeout

bool stopAnim = true;         //Dictates, wether the button fading animation should be stopped for [expButton]ms after a button press or not

int ledPin1 = 6;    // LED connected to digital pin 9
int buttonPin1 = A5;

int ledPin2 = 3;    // LED connected to digital pin 9
int buttonPin2 = A4;



int currentpos = 7;
//int lastpos = currentpos;   //This line seems to be useless, since lastpos seems to be never used



// Don't forget to connect the ground wire to Arduino ground,
// and the +5V wire to a +5V supply

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
Adafruit_WS2801 strip = Adafruit_WS2801(15);



void setup() {


  strip.begin();

  // Update LED contents, to start they are all 'off'
  strip.show();


  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);

  init_display();
  now = millis();
}









void loop() {



  if (currentpos == 15) {
    rainbowCycle(random(10, 20));
    return;
  }
  if (currentpos == -1) {
    redFade();
    return;
  }



  // check if button was pressed
  button_now = millis();
  if (!digitalRead(buttonPin1) && tdelta(button_now, button_prev) > expButton)
  {
    if (currentpos == -1) return;
    /*strip.setPixelColor(currentpos, Color(0, 255, 0));
    currentpos--;
    strip.setPixelColor(currentpos, Color(255, 255, 255));
    strip.show();*/
    mood_dn();
    while (!digitalRead(buttonPin1)) {}
    button_prev = button_now;
    //delay(10000);
  }

  if (!digitalRead(buttonPin2) && tdelta(button_now, button_prev) > expButton)
  {
    if (currentpos == 15) return;
    /*strip.setPixelColor(currentpos, Color(255, 0, 0));
    currentpos++;
    strip.setPixelColor(currentpos, Color(255, 255, 255));
    strip.show();*/
    mood_up();
    while (!digitalRead(buttonPin2)) {}
    button_prev = button_now;
    //delay(10000);
  }
  
  now = millis();
  if (tdelta(now, prev) >= expTime && currentpos != 7) {
    prev = now;
    normalizeMood();
  }

  if (stopAnim && tdelta(button_now, button_prev) <= expButton) {
    return;
  } else {
    fadeLed(ledPin1);
    fadeLed(ledPin2);
  }

}

unsigned long tdelta (unsigned long t_now, unsigned long t_prev) {
  if (t_now > t_prev) {
    return t_now - t_prev;
  }
  return sizeof(unsigned long) - t_prev + t_now;
}

void normalizeMood () {
  if (currentpos > 7) {
    mood_dn();
  }
  else if (currentpos < 7) {
    mood_up();
  }
}

void mood_up () {
  strip.setPixelColor(currentpos, Color(255, 0, 0));
  currentpos++;
  strip.setPixelColor(currentpos, Color(255, 255, 255));
  strip.show();
}

void mood_dn () {
  strip.setPixelColor(currentpos, Color(0, 255, 0));
  currentpos--;
  strip.setPixelColor(currentpos, Color(255, 255, 255));
  strip.show();
}







void redFade() {


  for (int j = 50; j < 255; j++)
  {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Color(0, j, 0) );
    }
    strip.show();   // write all the pixels out
    if (bd(10)) {
      currentpos = 0;
      resetRed();
      return;
    }
  }

  if (bd(1000)) {
    currentpos = 0;
    resetRed();
    return;
  }



  for (int j = 255; j > 50 ; j--)
  {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Color(0, j, 0) );
    }
    strip.show();   // write all the pixels out
    if (bd(10)) {
      currentpos = 0;
      resetRed();
      return;
    }
  }





}




void   init_display()
{
  for (int i = 0; i < strip.numPixels(); ++i) {
    strip.setPixelColor(i, Color(0, 255, 0));
  }
  strip.setPixelColor(strip.numPixels()/2, Color(255, 255, 255));

  strip.show();
}






void resetGreen()
{
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Color(255, 0, 0));
  }
  strip.show();   // write all the pixels out

}




void resetRed()
{
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Color(0, 255, 0));
  }
  strip.show();   // write all the pixels out

}







void fadeLed(int ledPin)
{

  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
    // sets the value (range from 0 to 255):
    analogWrite(ledPin, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    if (bd(5)) return;
  }

  // fade out from max to min in increments of 5 points:
  for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 5) {
    // sets the value (range from 0 to 255):
    analogWrite(ledPin, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    if (bd(5)) return;
  }

}





int bd(int del)
{
  for (int i = 0; i < del; i++)
  {

    delay(1);
    if ((!digitalRead(buttonPin2))  || (!digitalRead(buttonPin1))) return 1;
  }
  return 0;


}




// Slightly different, this one makes the rainbow wheel equally distributed
// along the chain
void rainbowCycle(uint8_t wait) {
  int i, j;

  for (j = 0; j < 256 * 5; j++) {   // 5 cycles of all 25 colors in the wheel
    for (i = 0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 96-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 96 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 256 / strip.numPixels()) + j) % 256) );
    }
    strip.show();   // write all the pixels out
    if (bd(wait)) {
      currentpos = 14;
      resetGreen();
      return;
    }
  }
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
  int i;

  for (i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

/* Helper functions */

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

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85) {
    return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
