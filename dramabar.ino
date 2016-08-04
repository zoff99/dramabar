#include "Adafruit_WS2801.h"
#include "limits.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma

#define REDFADE_SPEED 10
#define BUTTONFADE_SPEED 10
#define RAINBOW_SPEED 10

#define METALED true        //corrects the issue that the dramabar seems to have GRB instead of RGB LEDs

#define DEBUG false       //defines if the debug version should be used (shortened cooldowns so that everything happens a bit faster for testing purposes)
#if DEBUG
unsigned long expTime = 5000;
unsigned long expButton = 1000;
#else
unsigned long expTime = 3600000;              //Time in ms for mood expiration (eg: every X ms mood goes 1 point nearer to normal)
unsigned long expButton = 10000;              //Time in ms in which all button presses after the first one get ignored
#endif

unsigned long now = 0;        //Time in ms now (measured from boot)
unsigned long prev_normalize = 0, button_prev = 0;    //Time in ms, from last run of loop, t_prev_normalize for mood, button_prev_normalize for button timeout

bool stopAnim = false;         //Dictates, wether the button fading animation should be stopped for [expButton]ms after a button press or not

int ledPin1 = 6;    // LED connected to digital pin 9
int buttonPin1 = A5;

int ledPin2 = 3;    // LED connected to digital pin 9
int buttonPin2 = A4;

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

  //attachInterrupt(digitalPinToInterrupt(buttonPin1), buttonPressed, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(buttonPin2), buttonPressed, CHANGE);

  init_display();
  now = millis();

  Serial.begin(9600);
}

void loop() {
  static int8_t currentPos = 7;
  static bool drama = false;

  if (currentPos == 15) {
    rainbowCycle(random(10, 20));
  }
  if (currentPos == -1) {
    redFade(0);
  }

  // check if button was pressed
  now = millis();
  if (!digitalRead(buttonPin2) && tdelta(now, button_prev) > expButton) {
    button_prev = now;
    //prev_normalize += expTime / 6;
    if (currentPos == 15) return;
    currentPos = mood_up(currentPos);
  }
  else if (!digitalRead(buttonPin1) && tdelta(now, button_prev) > expButton) {
    button_prev = now;
    //prev_normalize += expTime / 6;
    if (currentPos == -1) return;
    currentPos = mood_dn(currentPos);
  }


  now = millis();
  if (tdelta(now, prev_normalize) >= expTime && currentPos != 7) {
    prev_normalize = now;
    currentPos = normalizeMood(currentPos);
  }

  if (drama) {
    redFade(currentPos + 1);
  }

  if (stopAnim && tdelta(now, button_prev) <= expButton) {
    return;
  } else {
    fadeLed(ledPin1);
    fadeLed(ledPin2);
  }

}

//Time functions
unsigned long tdelta (unsigned long t_now, unsigned long t_prev) {
  if (t_now > t_prev) {
    return t_now - t_prev;
  }
  return ULONG_MAX - t_prev + t_now;
}
//Time functions end

//Mood Functions
int8_t normalizeMood (int8_t currentPos) {
  Serial.println("normalized mood");
  if (currentPos > 7) {
    currentPos = mood_dn(currentPos);
  }
  else if (currentPos < 7) {
    currentPos = mood_up(currentPos);
  }
  return currentPos;
}

int8_t mood_up (int8_t currentPos) {
  if (currentPos == -1) {
    resetRed();
  }
  strip.setPixelColor(currentPos, Color(0, 255, 0));
  ++currentPos;
  strip.setPixelColor(currentPos, Color(255, 255, 255));
  strip.show();
  return currentPos;
}

int8_t mood_dn (int8_t currentPos) {
  if (currentPos == 15) {
    resetGreen();
  }
  strip.setPixelColor(currentPos, Color(255, 0, 0));
  --currentPos;
  strip.setPixelColor(currentPos, Color(255, 255, 255));
  strip.show();
  return currentPos;
}
//Mood Functions end

//LED functions
void init_display() {
  for (int i = strip.numPixels() / 2; i < strip.numPixels(); ++i) {
    strip.setPixelColor(i, Color(255, 0, 0));
  }
  for (int i = 0; i < strip.numPixels() / 2; ++i) {
    strip.setPixelColor(i, Color(0, 255, 0));
  }
  strip.setPixelColor(strip.numPixels() / 2, Color(255, 255, 255));

  strip.show();
}

void resetGreen() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Color(0, 255, 0));
  }
  strip.show();   // write all the pixels out
}

void resetRed() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Color(255, 0, 0));
  }
  strip.show();   // write all the pixels out
}

//LED FX functions
//when startpos = 0 everything fades red, other startpositions for dramamode (where only the red mood part blinks)
void redFade (int8_t currentPos) {
  static unsigned long prev_fade = 0;
  static bool up = true;
  static int j = 30;

  now = millis();
  if (tdelta(now, prev_fade) < REDFADE_SPEED) {
    return;
  }

  prev_fade = now;
  if (up) {
    for (int i = currentPos; i < strip.numPixels(); ++i) {
      strip.setPixelColor(i, Color(j, 0, 0));
    }
    ++j;
  } else {
    for (int i = currentPos; i < strip.numPixels(); ++i) {
      strip.setPixelColor(i, Color(j, 0, 0));
    }
    --j;
  }
  strip.show();

  if (j >= 255) {
    up = false;
  }
  else if (j <= 30) {
    up = true;
  }
}

//fades the buttons
void fadeLed(uint8_t ledPin) {
  static unsigned long prev_fade = 0;
  static uint8_t fadeValue = 0;
  static bool fadeIn = true;

  if (tdelta(now, prev_fade) < BUTTONFADE_SPEED) {
    return;
  }

  if (fadeIn) {
    analogWrite(ledPin, fadeValue);
    ++prev_fade;
  } else {
    analogWrite(ledPin, fadeValue);
    --prev_fade;
  }
}


// Slightly different, this one makes the rainbow wheel equally distributed
// along the chain
void rainbowCycle(uint8_t wait) {
  static unsigned long prev_bow = 0;
  static unsigned long rainbow_pos = 0;

  now = millis();
  if (tdelta(now, prev_bow) < RAINBOW_SPEED) {
    return;
  }

  prev_bow = now;
  for (int i = 0; i < strip.numPixels(); i++) {
    /*tricky math! we use each pixel as a fraction of the full 96-color wheel
      (thats the i / strip.numPixels() part)
      Then add in j which makes the colors go around per pixel
      the % 96 is to make the wheel cycle around*/
    strip.setPixelColor(i, Wheel( ((i * 256 / strip.numPixels()) + rainbow_pos) % 256) );
  }
  strip.show();   // write all the pixels out
  ++rainbow_pos;
  rainbow_pos %= 256;
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
//LED FX end

// Helper functions
// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;

#if METALED
  c = g;
  c <<= 8;
  c |= r;
  c <<= 8;
  c |= b;
#else
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
#endif

  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(uint8_t WheelPos) {
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
