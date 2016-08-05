#include "Adafruit_WS2801.h"
#include "limits.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma

//SETTINGS BEGIN
#define BUTTONS_ANIMATE 0     //whether the buttons should animate in EVER. Leave this to 0 until you somehow fix the animations, because they are SH*T right now
#define STOPANIM 1            //Dictates, whether the button fading animation should be stopped for [expButton]ms after a button press or not


#define REDFADE_SPEED 10
#define BUTTONFADE_SPEED 50    //this value seems to have to be higher than the others, I think this is because the PWM output of the microcontroller doesn't update that often
#define RAINBOW_SPEED 10

#define BUTTON_LIGHT_BORDER_UP 255    //intensity of the button light (upper border). possible values: 0-255
#define BUTTON_LIGHT_BORDER_DOWN 50   //intensity of the button light (lower border). possible values: 0-255. ATTENTION: UPPER BORDER __MUST__ BE HIGHER THAN LOWER BORDER
#define BUTTON_OFF_INACT 0            //Dictates, whether the button light should be off during it being inactive, or only off during the button being pushed

#define REDFADE_BORDER_UP 255         //see BUTTON_LIGHT_BORDER_UP for more information
#define REDFADE_BORDER_DOWN 30        //see BUTTON_LIGHT_BORDER_DOWN for more information

#define METALED true        //corrects the issue that the dramabar seems to have GRB instead of RGB LEDs

#define DEBUG false         //defines if the debug version should be used (shortened cooldowns so that everything happens a bit faster for testing purposes)
#if DEBUG
unsigned long expTime = 5000;                 //see further down for explanation
unsigned long expButton = 1000;               //see further down for explanation
#else
unsigned long expTime = 2700000;               //Time in ms for mood expiration (eg: every X ms mood goes 1 point nearer to normal)
unsigned long expButton = 10000;              //Time in ms in which all button presses after the first one get ignored
#endif

unsigned long now = 0;                                //Time in ms now (measured from boot). Used for all the timeouts and waiting stuff
unsigned long prev_normalize = 0, button_prev = 0;    //Time in ms, from last run of loop, t_prev_normalize for mood, button_prev_normalize for button timeout


//forward declaration
uint32_t Color(byte r, byte g, byte b);

//defines the colors with wich the mood bar should work. ATTENTION! The redFade function does NOT change with these settings, it will still fade red and NOT fade the defined colorSad
uint32_t colorHappy = Color(0, 255, 0);
uint32_t colorSad = Color(255, 0, 0);
uint32_t colorCursor = Color(255, 255, 255);
//SETTINGS END



int ledPin1 = 6;    // LED connected to digital pin 6
int buttonPin1 = A5;

int ledPin2 = 3;    // LED connected to digital pin 3
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

  pinMode(ledPin1, OUTPUT);
  digitalWrite(ledPin1, HIGH);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin2, HIGH);

  init_display();
  now = millis();
}

void loop() {
  //Initialize variables
  static int8_t currentPos = 7;
  static bool drama = false;  //drama == true makes the red part of the mood bar blink

  //For the extreme moods
  if (currentPos == 15) {
    rainbowCycle(random(10, 20));
  }
  if (currentPos == -1) {
    redFade(0);
  }

  // check if button was pressed and if there was enough delay since the last button press
  now = millis();
  if (!digitalRead(buttonPin2) && tdelta(now, button_prev) > expButton) {
    button_prev = now;
    //resets some of the time for the mood normalization, so that it won't normalize shortly after a manual mood change (expTime / 9 equals to 5 min for the default 45min expiration time)
    //the  if  checks, that at least half of the expiration timer has to be expired so that it won't trigger the "pseudo overflow bug" (see at tdelta for more information)
    if (tdelta(now, prev_normalize) > expTime/2) {
      prev_normalize += expTime / 9;
    }
    if (currentPos == 15) return;
    currentPos = mood_up(currentPos);
    //Turns the button light off while it is being pushed
    digitalWrite(ledPin2, LOW);
  }
  else if (!digitalRead(buttonPin1) && tdelta(now, button_prev) > expButton) {
    button_prev = now;
    //resets some of the time for the mood normalization, so that it won't normalize shortly after a manual mood change (expTime / 9 equals to 5 min for the default 45min expiration time)
    //the  if  checks, that at least half of the expiration timer has to be expired so that it won't trigger the "pseudo overflow bug" (see at tdelta for more information)
    if (tdelta(now, prev_normalize) > expTime/2) {
      prev_normalize += expTime / 9;
    }
    if (currentPos == -1) return;
    currentPos = mood_dn(currentPos);
    //Turns the button light off while it is being pushed
    digitalWrite(ledPin1, LOW);
  }

  //resets mood normalization, so that it won't normalize instantly after leaving the neutral state (currentPos == 7)
  if (currentPos == 7) {
    prev_normalize = now;
  }

  //normalizes the mood if enough time has passed after the last normalization (default 45min, this can be delayed by pressing buttons, since this resets this time by expTime/9, which is 5min for 45min expTime
  now = millis();
  if (tdelta(now, prev_normalize) >= expTime && currentPos != 7) {
    prev_normalize = now;
    currentPos = normalizeMood(currentPos);
  }

  //if drama is true, the red part of the mood bar will start to blink - this won't be visible when mood is -1 or 15, since the occuring animations at those points overrule the blinking
  if (drama) {
    redFade(currentPos + 1);
  }

  //animates the buttons, STOPANIM dictates whether the fading should stop during the (default) 10sec button inactivity after a button press
  if ((STOPANIM && tdelta(now, button_prev) <= expButton) || !BUTTONS_ANIMATE) {
    return;
  } else {
    fadeLed(ledPin1);
    fadeLed(ledPin2);
  }

  //Sets the button lights to on while not being pressed, or while not being pressed and not inactive (dictated by BUTTON_OFF_INACT)
  if (digitalRead(buttonPin2) && (tdelta(now, button_prev) > expButton || !BUTTON_OFF_INACT)) {
    digitalWrite(ledPin2, HIGH);
  }
  if (digitalRead(buttonPin1) && (tdelta(now, button_prev) > expButton || !BUTTON_OFF_INACT)) {
    digitalWrite(ledPin1, HIGH);
  }
}

//Time functions
/* tdelta calculates the delta time between now and previous (t_now, t_prev). It has a overflow prevention, since the millis() variable (ulong) overflows every ~50 days.
 * In that case the calculation gets changed so that the overflow doesn't change the elapsed time. The correction gets called if the PREVIOUS time is higher than NOW.
 * Because of that there is a chance of a "pseudo overflow bug", in which the function thinks, that an overflow happened, but it didn't (only happens when either now or
 * prev gets changed manually, which is the case if a button press occurs, since the button press changes the prev_normalize value, so that the next normalization happens
 * later. But when prev's value is too close to now's value, this can trigger the bug. I prevented this from happening via an  if  but if some values get changed it could happen again*/
unsigned long tdelta (unsigned long t_now, unsigned long t_prev) {
  if (t_now >= t_prev) {
    return t_now - t_prev;
  }
  return ULONG_MAX - t_prev + t_now;
}
//Time functions end

//Mood Functions
//normalizes the mood
int8_t normalizeMood (int8_t currentPos) {
  if (currentPos > 7) {
    currentPos = mood_dn(currentPos);
  }
  else if (currentPos < 7) {
    currentPos = mood_up(currentPos);
  }
  return currentPos;
}

//increases the mood bar by one
int8_t mood_up (int8_t currentPos) {
  if (currentPos == -1) {
    resetSadColor();
  }
  strip.setPixelColor(currentPos, colorHappy);
  ++currentPos;
  strip.setPixelColor(currentPos, colorCursor);
  strip.show();
  return currentPos;
}

//decreases the mood bar by one
int8_t mood_dn (int8_t currentPos) {
  if (currentPos == 15) {
    resetHappyColor();
  }
  strip.setPixelColor(currentPos, colorSad);
  --currentPos;
  strip.setPixelColor(currentPos, colorCursor);
  strip.show();
  return currentPos;
}
//Mood Functions end

//LED functions
//initializes the display
void init_display() {
  //changes the LED color from all LEDs from middle to top to defined sad color (sad part of neutral mood)
  for (int i = strip.numPixels() / 2; i < strip.numPixels(); ++i) {
    strip.setPixelColor(i, colorSad);
  }
  //changes the LED color from all LEDs from beginning to middle to happy color (happy part of neutral mood)
  for (int i = 0; i < strip.numPixels() / 2; ++i) {
    strip.setPixelColor(i, colorHappy);
  }
  //sets the LED in the middle for the cursor in the defined cursor color
  strip.setPixelColor(strip.numPixels() / 2, colorCursor);

  strip.show();
}

//sets all pixels to the defined happy color
void resetHappyColor() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, colorHappy);
  }
  strip.show();   // write all the pixels out
}

//sets all pixels to the defined sad color
void resetSadColor() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, colorSad);
  }
  strip.show();   // write all the pixels out
}

//LED FX functions
//when startpos = 0 everything fades red (which is used for doomsday mood which is currentPos == -1), other startpositions for dramamode (where only the red mood part blinks)
void redFade (int8_t currentPos) {
  static unsigned long prev_fade = 0;
  static bool up = true;
  static int j = 30;

  now = millis();
  if (tdelta(now, prev_fade) < REDFADE_SPEED) {
    return;
  }

  prev_fade = now;
  if (up) { //fade in
    for (int i = currentPos; i < strip.numPixels(); ++i) {
      strip.setPixelColor(i, Color(j, 0, 0));
    }
    ++j;
  } else {  //fade out
    for (int i = currentPos; i < strip.numPixels(); ++i) {
      strip.setPixelColor(i, Color(j, 0, 0));
    }
    --j;
  }
  strip.show();

  //changes direction of fading on all of the borders
  if (j >= REDFADE_BORDER_UP) {
    up = false; 
  }
  else if (j <= REDFADE_BORDER_DOWN) {
    up = true;
  }
}

//fades the buttons   ---CURRENTLY VERY SHITTY! Fading is very laggy
void fadeLed(uint8_t ledPin) {
  static unsigned long prev_fade = 0;
  static uint8_t fadeValue = BUTTON_LIGHT_BORDER_DOWN;
  static bool fadeIn = true;

  now = millis();
  if (tdelta(now, prev_fade) < BUTTONFADE_SPEED) {
    return;
  }
  
  prev_fade = now;
  if (fadeIn) { //fade in 
    analogWrite(ledPin, fadeValue);
    fadeValue += 3;
  } else {      //fade out
    analogWrite(ledPin, fadeValue);
    fadeValue -= 3;
  }

  //changes direction of fading on the borders
  if (fadeValue >= BUTTON_LIGHT_BORDER_UP) {
    fadeIn = false;
  }
  else if (fadeValue <= BUTTON_LIGHT_BORDER_DOWN) {
    fadeIn = true;
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
      Then add in rainbow_pos which makes the colors go around per pixel
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
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}
//LED FX end

// Helper functions
// Create a 24 bit color value from R,G,B, defined METALED value defines if normal RGB mode should be used for that, or if GRB should be used (the dramabar in metalab apparently has GRB LEDs
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
uint32_t Wheel(uint8_t WheelPos)
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
