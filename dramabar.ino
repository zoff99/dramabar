#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
#include "limits.h"

//SETTINGS BEGIN
bool buttonsAnimate = 0;      //whether the buttons should animate in EVER. Leave this to 0 until you somehow fix the animations, because they are SH*T right now
bool stopAnim = 1;            //Dictates, whether the button fading animation should be stopped for [t_ButtonExp]ms after a button press or not

uint16_t s_Redfade = 10;
uint16_t s_Rainbow = 10;
uint16_t s_Button = 30;    //this value seems to have to be higher than the others, I think this is because the PWM output of the microcontroller doesn't update that often

uint8_t l_Button_UP = 255;    //intensity of the button light (upper border). possible values: 0-255
uint8_t l_Button_LOW = 50;    //intensity of the button light (lower border). possible values: 0-255. ATTENTION: UPPER BORDER __MUST__ BE HIGHER THAN LOWER BORDER
bool buttonOffInact = 1;      //Dictates, whether the button light should be off during it being inactive, or only off during the button being pushed

uint8_t l_Redfade_UP = 255;        //see l_Button_UP for more information
uint8_t l_Redfade_LOW = 30;        //see l_Button_LOW for more information

#define METALED true        //corrects the issue that the dramabar seems to have GRB instead of RGB LEDs

unsigned long t_Exp = 0;                 //see further down for explanation
unsigned long t_ButtonExp = 0;           //see further down for explanation

bool debug = false;         //defines if the debug version should be used (shortened cooldowns so that everything happens a bit faster for testing purposes)

unsigned long now = 0;                                //Time in ms now (measured from boot). Used for all the timeouts and waiting stuff
unsigned long prev_normalize = 0, button_prev = 0;    //Time in ms, from last run of loop, t_prev_normalize for mood, button_prev_normalize for button timeout


//forward declaration
uint32_t Color(byte r, byte g, byte b);

//defines the colors with wich the mood bar should work. ATTENTION! The redFade function does NOT change with these settings, it will still fade red and NOT fade the defined c_Sad
uint32_t c_Happy = Color(0, 255, 0);
uint32_t c_Sad = Color(255, 0, 0);
uint32_t c_Cursor = Color(255, 255, 255);
//SETTINGS END



int ledPin1 = 6;    // LED connected to digital pin 6
int buttonPin1 = A5;

int ledPin2 = 3;    // LED connected to digital pin 3
int buttonPin2 = A4;

// Don't forget to connect the ground wire to Arduino ground,
// and the +5V wire to a +5V supply

// Set the first variable to the NUMBER of pixels. 25 = 25 pixels in a row
Adafruit_WS2801 strip = Adafruit_WS2801(15);

enum settingTypes {
  _color, _number, _bool, _other
};

void setup() {
  Serial.begin(115200);
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

unsigned long prev_debug = 0;

void loop() {
  //Initialize variables
  static int8_t currentPos = 7;
  static bool drama = false;  //drama == true makes the red part of the mood bar blink

  if (debug) {
    t_Exp = 5000;                 //see further down for explanation
    t_ButtonExp = 1000;               //see further down for explanation
  } else {
    t_Exp = 3600000;               //Time in ms for mood expiration (eg: every X ms mood goes 1 point closer to normal)
    t_ButtonExp = 10000;              //Time in ms in which all button presses after the first one get ignored
  }

  /*now = millis();
  if (tdelta(now, prev_debug) > 10000) {
    Serial.println("C loop: ");
    Serial.println(c_Happy);
    Serial.println(c_Sad);
    Serial.println(c_Cursor);
    prev_debug = now;
  }*/
  
  if (readSettings()) {
    /*Serial.println("C before refresh:");
    Serial.println(c_Happy);
    Serial.println(c_Sad);
    Serial.println(c_Cursor);*/
    refresh_display(currentPos);
  }
  
  //For the extreme moods
  if (currentPos == 15) {
    rainbowCycle(random(10, 20));
  }
  if (currentPos == -1) {
    redFade(0);
  }

  // check if button was pressed and if there was enough delay since the last button press
  now = millis();
  if (!digitalRead(buttonPin2) && tdelta(now, button_prev) > t_ButtonExp) {
    button_prev = now;
    //resets some of the time for the mood normalization, so that it won't normalize shortly after a manual mood change (expTime / 9 equals to 5 min for the default 45min expiration time)
    //the  if  checks, that at least half of the expiration timer has to be expired so that it won't trigger the "pseudo overflow bug" (see at tdelta for more information)
    if (tdelta(now, prev_normalize) > t_Exp/2) {
      prev_normalize += t_Exp / 12;
    }
    if (currentPos == 15) return;
    currentPos = mood_up(currentPos);
    //Turns the button light off while it is being pushed
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
  }
  else if (!digitalRead(buttonPin1) && tdelta(now, button_prev) > t_ButtonExp) {
    button_prev = now;
    //resets some of the time for the mood normalization, so that it won't normalize shortly after a manual mood change (expTime / 9 equals to 5 min for the default 45min expiration time)
    //the  if  checks, that at least half of the expiration timer has to be expired so that it won't trigger the "pseudo overflow bug" (see at tdelta for more information)
    if (tdelta(now, prev_normalize) > t_Exp/2) {
      prev_normalize += t_Exp / 12;
    }
    if (currentPos == -1) return;
    currentPos = mood_dn(currentPos);
    //Turns the button light off while it is being pushed
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
  }

  //resets mood normalization, so that it won't normalize instantly after leaving the neutral state (currentPos == 7)
  if (currentPos == 7) {
    prev_normalize = now;
  }

  //normalizes the mood if enough time has passed after the last normalization (default 45min, this can be delayed by pressing buttons, since this resets this time by expTime/9, which is 5min for 45min expTime
  now = millis();
  if (tdelta(now, prev_normalize) >= t_Exp && currentPos != 7) {
    prev_normalize = now;
    currentPos = normalizeMood(currentPos);
  }

  //if drama is true, the red part of the mood bar will start to blink - this won't be visible when mood is -1 or 15, since the occuring animations at those points overrule the blinking
  if (drama) {
    redFade(currentPos + 1);
  }

  //Sets the button lights to on while not being pressed, or while not being pressed and not inactive (dictated by buttonOffInact)
  if (digitalRead(buttonPin2) && (tdelta(now, button_prev) > t_ButtonExp || !buttonOffInact)) {
    digitalWrite(ledPin2, HIGH);
    digitalWrite(ledPin1, HIGH);
  }
  if (digitalRead(buttonPin1) && (tdelta(now, button_prev) > t_ButtonExp || !buttonOffInact)) {
    digitalWrite(ledPin2, HIGH);
    digitalWrite(ledPin1, HIGH);
  }

  //animates the buttons, stopAnim dictates whether the fading should stop during the (default) 10sec button inactivity after a button press
  if ((stopAnim && tdelta(now, button_prev) <= t_ButtonExp) || !buttonsAnimate) {
  } else {
    fadeLed(ledPin1);
    fadeLed(ledPin2);
  }
}

//Settings functions
String findValueStr (String str) {  
  uint8_t posEquals = 0;

  posEquals = str.indexOf('=');
  if (str[posEquals + 1] == '%') {
    return str.substring(posEquals + 4);
  }
  return str.substring(posEquals + 1);
}

String findName (String str) {
  return str.substring(0, str.indexOf('='));
}

uint8_t getSettingsType (String key) {
  if (key[0] == 'c') return _color;
  else if (key[0] == 's' || key[0] == 'l' || key[0] == 't') return _number;
  else if (key[0] == 'b') return _bool;
  return _other;
}

uint8_t convertToNumber (char strValue) {
  if (isDigit(strValue)) {
    return strValue - '0';
  }
  switch (strValue) {
    case 'A': case 'a':
      return 10;
      break;
    case 'B': case 'b':
      return 11;
      break;
    case 'C': case 'c':
      return 12;
      break;
    case 'D': case 'd':
      return 13;
      break;
    case 'E': case 'e':
      return 14;
      break;
    case 'F': case 'f':
      return 15;
      break;
    default:
      return strValue - '0';
      break;
  }
}

uint32_t power(uint16_t base, uint8_t exponent) {
  uint32_t result = 1;
  for (; exponent > 0; --exponent) {
    result *= base;
  }
  return result;
}

uint32_t atoh (String strValue) {
  uint8_t digits[] = {
    0, 0, 0, 0, 0, 0
  };
  uint32_t value = 0;

  for (int8_t i = strValue.length() - 1, n = 0; i >= 0; --i, ++n) {
    digits[n] = convertToNumber(strValue[i]);
  }
  
  for (uint8_t i = 0; i < 6; ++i) {
    //Serial.print("val in atoh: "); Serial.println(value);
    value += digits[i] * power(16, i);
    /*Serial.print("digits[i]: "); Serial.println(digits[i]);
    Serial.print("i: "); Serial.println(i);
    Serial.print("pow: "); Serial.println(power(16, i));*/
  }
  return value;
}

uint32_t getValue (String strValue, uint8_t type) {
  switch (type) {
    case _color:
      return atoh(strValue);
      break;
    case _number:
      return strValue.toInt();
      break;
    case _bool:
      if (strValue == "true") return 1;
      if (strValue == "false") return 0;
      break;
    default:
    return 0;
      break;
  }
}

uint32_t correctColor (uint32_t color) {
  if (METALED) {
    uint8_t r, g, b;
    r = color >> 16;
    g = color >> 8;
    b = color;
    color = Color(r, g, b);
    return color;
  } else {
    return color;
  }
}

void setSettings (String key, uint32_t value) {
  buttonsAnimate = false;
  stopAnim = false;
  buttonOffInact = false;
  debug = false;

  /*Serial.print("key: "); Serial.println(key);
  Serial.print("val: "); Serial.println(value);*/

  if (key == "cH") c_Happy = correctColor(value);
  else if (key == "cS") c_Sad = correctColor(value);
  else if (key == "cC") c_Cursor = correctColor(value);
  else if (key == "sRf") s_Redfade = value;
  else if (key == "sRb") s_Rainbow = value;
  else if (key == "sB") s_Button = value;
  else if (key == "lRU") l_Redfade_UP = value;
  else if (key == "lRL") l_Redfade_LOW = value;
  else if (key == "lBU") l_Button_UP = value;
  else if (key == "lBL") l_Button_LOW = value;
  else if (key == "bB") buttonsAnimate = value;
  else if (key == "bSB") stopAnim = value;
  else if (key == "bBIn") buttonOffInact = value;
  else if (key == "tExp") t_Exp = value * 60*1000;  //t_Exp  is entered in minutes, but is used in ms, 60 * 1000 is conversion
  else if (key == "tBExp") t_ButtonExp = value*1000; //t_ButtonExp is entered in seconds but used in ms, 1000 is conversion
  else if (key == "bDe") debug = value;
}

void parseSettings (String settingsData) {
  uint16_t oldPos = 0, newPos = 0, lastPos = 0;
  String subString = "", strValue = "", settingName = "";

  lastPos = settingsData.lastIndexOf('&');
  while (lastPos > oldPos) {  //This won't see the last setting, but since this will only be "save=Save+Settings" it's ok
    newPos = settingsData.indexOf('&', oldPos);
    subString = settingsData.substring(oldPos, newPos);
    oldPos = newPos + 1;

    strValue = findValueStr(subString);
    settingName = findName(subString);
    setSettings(settingName, getValue(strValue, getSettingsType(settingName)));
  }
}

bool readSettings() {
  String data = "";
  if (Serial.available()) {
    data = Serial.readStringUntil('\0');
    //Serial.print("read string until 0 "); Serial.println(data);
  } else {
    return false;
  }
  if (data.indexOf("already freed") != -1) {
    //Serial.print("already freed, discarding data: "); Serial.println(data);
    data = "";
    return false;
  }
  else if (data.lastIndexOf("powered by Lua 5.1.4 on SDK 1.5.4.1(39cb9a32)") != -1) {
    //Serial.println("Firmware overhead");
    data = "";
    return false;
  }
  //Serial.print("Data Ada: "); Serial.println(data);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  parseSettings(data);
  digitalWrite(ledPin1, HIGH);
  digitalWrite(ledPin2, HIGH);
  return true;
}
//Settings functions end

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
  strip.setPixelColor(currentPos, c_Happy);
  ++currentPos;
  strip.setPixelColor(currentPos, c_Cursor);
  strip.show();
  return currentPos;
}

//decreases the mood bar by one
int8_t mood_dn (int8_t currentPos) {
  if (currentPos == 15) {
    resetHappyColor();
  }
  strip.setPixelColor(currentPos, c_Sad);
  --currentPos;
  strip.setPixelColor(currentPos, c_Cursor);
  strip.show();
  return currentPos;
}
//Mood Functions end

//LED functions
void refresh_display(int8_t currentPos) {
  if (currentPos == -1 || currentPos == 15) return;
  
  for (uint8_t i = 0; i < currentPos; ++i) {
    strip.setPixelColor(i, c_Happy);
  }
  for (uint8_t i = strip.numPixels(); i > currentPos; --i) {
    strip.setPixelColor(i, c_Sad);
  }
  strip.setPixelColor(currentPos, c_Cursor);
}

//initializes the display
void init_display() {
  //changes the LED color from all LEDs from middle to top to defined sad color (sad part of neutral mood)
  for (int i = strip.numPixels() / 2; i < strip.numPixels(); ++i) {
    strip.setPixelColor(i, c_Sad);
  }
  //changes the LED color from all LEDs from beginning to middle to happy color (happy part of neutral mood)
  for (int i = 0; i < strip.numPixels() / 2; ++i) {
    strip.setPixelColor(i, c_Happy);
  }
  //sets the LED in the middle for the cursor in the defined cursor color
  strip.setPixelColor(strip.numPixels() / 2, c_Cursor);

  strip.show();
}

//sets all pixels to the defined happy color
void resetHappyColor() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c_Happy);
  }
  strip.show();   // write all the pixels out
}

//sets all pixels to the defined sad color
void resetSadColor() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c_Sad);
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
  if (tdelta(now, prev_fade) < s_Redfade) {
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
  if (j >= l_Redfade_UP) {
    up = false; 
  }
  else if (j <= l_Redfade_LOW) {
    up = true;
  }
}

//fades the buttons   ---CURRENTLY VERY SHITTY! Fading is very laggy
void fadeLed(uint8_t ledPin) {
  static unsigned long prev_fade = 0;
  static uint8_t fadeValue = l_Button_LOW;
  static bool fadeIn = true;

  now = millis();
  if (tdelta(now, prev_fade) < s_Button) {
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
  if (fadeValue >= l_Button_UP) {
    fadeIn = false;
  }
  else if (fadeValue <= l_Button_LOW) {
    fadeIn = true;
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed
// along the chain
void rainbowCycle(uint8_t wait) {
  static unsigned long prev_bow = 0;
  static unsigned long rainbow_pos = 0;

  now = millis();
  if (tdelta(now, prev_bow) < s_Rainbow) {
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
