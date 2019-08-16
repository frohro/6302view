# 6302view

A setup that allows researchers and learners to interact directly with their microcontroller in a web browser. Used in MIT subject 6.302 (Feedback system design).

## Supported microcontrollers

* Arduino Uno
* Teensy
* ESP8266
* ESP32

## Quick example

```cpp

#include <Six302.h>

// microseconds
#define STEP_TIME 10000
#define REPORT_TIME 50000

CommManager cm(STEP_TIME, REPORT_TIME);

float input;
float output;

void setup() {
   /* Add modules */
   cm.addSlider(&input, "Input", 0, 5, 0.1);
   cm.addPlot(&output, "Output", -1, 26);

   /* Ready to communicate over serial */
   cm.connect(&Serial, 115200);
}

void loop() {
   output = input * input;
   cm.step();
}
```

In the webpage, the above creates a slider called "Input" from 0 to 5 (at a step size of 0.1) and a plot called "Output" from -1 to 26, illustrating the square of the slider input.

The system loops at once per 10000 µs (10 ms), and data is reported once per 50000 µs (50 ms). The system uses the Serial port to communicate with the webpage.

*(Pictures of the physical setup and the resulting webpage to be added.)*

## Desired goals

* To be elegant -- just include the library, no need to modify anything.
* To support the following microcontrollers:
   * Teensy
   * ESP32
   * ESP8266
   * Arduino Uno
* To support the following communication methods:
   * Over port (pass in your `Serial` and BAUD rate)
   * Websockets (pass in SSID and p/w)

[WebSocket library used](https://github.com/Links2004/arduinoWebSockets)

## Backburner

* Potentially to support the following communication methods:
   * MQTT instead of websockets
   * Bluetooth
   * HTTP (low frequency)
