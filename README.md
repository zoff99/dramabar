# dramabar


There is a big "progressbar" style thing in the mainroom.
It currently hangs next to the lounge entrance.
Inside are 
 - 15 pcs WS2801 pixels
 - an Adafruit pro Trinket 5V that is only programmable over FTDI 
 - and 2 buttons

One can raise or lower the current moodlevel by hitting the + or - button respectively.
If it reaches 15 it turns all rainbowy and the bar goes doomsday red gloomy on -1.

TODO Adafruit code:
- Code clean up
- change the time logic from "previous time triggered" and "now" to "now" and "time for next trigger" because it solves some problems with counter manipulation

TODO ESP code:
- write interface for listening to the Adafruit for mood status (serial interface needs a logic level shifter, maybe an analog value would be easier to implement)
- clean up and make failsafer (currently the program crashes as soon as serial data is received
- implement password authentication (is this really required?)
- Stretch goal: Implement "multithreaded" page loading (currently every website page has to be ONE document, because modern browser request several files at once and the webserver code is unable to account for that. Real multithreaded loading is impossible since the ESP only has one processing core)

TODO Website:
- Include animatable dramabar logo (which can be set to the actual color and mood values of the dramabar)
- Set up so that other values than default values can be loaded (would also need some ESP Code additions)
