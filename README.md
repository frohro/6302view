# 6302view

A setup that allows researchers and learners to interact directly with their microcontroller in a web browser. Used in MIT subject 6.302 (Feedback system design). This software lets the user control and observe their microcontroller in real-time.

[![(system setup)](https://i.imgur.com/djGt0lU.jpg "6302view with Teensy setup")](https://www.youtube.com/watch?v=AaNXcUNaw-I)

## Supported microcontrollers

* Teensy
* ESP8266
* ESP32
* Arduino Uno

## Quick example

```cpp

#include <Six302.h>

// microseconds
#define STEP_TIME 5000
#define REPORT_TIME 50000

CommManager cm(STEP_TIME, REPORT_TIME);

float input;
float output;

void setup() {
   /* Add modules */
   cm.addSlider(&input, "Input", 0, 5, 0.01);
   cm.addPlot(&output, "Output", -1, 26);

   /* Ready to communicate over serial */
   cm.connect(&Serial, 115200);
}

void loop() {
   output = input * input;
   cm.step();
}
```

The above creates a **slider** called "Input" from 0 to 5 (at a step size of 0.1) and a **plot** called "Output" from -1 to 26, illustrating the square of the slider input:

![(gif of resulting modules)](https://i.imgur.com/THO1Me1.gif)

The system loops at once per 5000 µs (5 ms), and data is reported once per 50000 µs (50 ms).

## Goals

* To be elegant -- just include the library, no need to modify anything.
* To support the following microcontrollers:
   * Teensy
   * ESP32
   * ESP8266
   * Arduino Uno
* To support the following communication methods:
   * Over port (pass in your `Serial` and BAUD rate)
   * Websockets (pass in SSID and p/w)

### Backburner

* Potentially to support the following communication methods:
   * MQTT instead of websockets
   * Bluetooth
   * HTTP (low frequency)

## Dependencies

### Arduino libraries used

Only required if you want to use the WiFi modules of ESP8266 or ESP32 as opposed to communication over Serial.

Go to `Manage libraries...` and search for `WebSockets`. It's [the one at the bottom by Markus Sattler](https://github.com/Links2004/arduinoWebSockets).

### Python modules used

```plaintext
websockets
pyserial
```
