# 6302view

Used in MIT subject 6.302

Documentation found here TB prettified.

## Quick example

```cpp

#include "Six302.h"

// microseconds
#define STEP_TIME 100000
#define REPORT_TIME 500000

CommManager cm(STEP_TIME, REPORT_TIME);

float input;
float output;

void setup() {
   /* Add modules */
   cm.addSlider(&input, "Input", -5, 5, 0.1);
   cm.addPlot(&output, "Output", -1, 30);

   /* Ready to communicate over serial */
   cm.connect(&Serial, 115200);
}

void loop() {
   output = input * input;
   cm.step();
}
```

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
* And potentially to support the following communication methods:
   * MQTT instead of websockets
   * Bluetooth
